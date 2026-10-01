#include "calibration_store.h"

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

namespace calibration_store {
namespace {
constexpr const char* kMagic = "PPC1_REF_V1";
constexpr size_t kPointCount = 24;
String gCurrentFilePath;
String gCurrentDisplayPath;
bool gInitialized = false;

String buildRandomTestFilePath() {
  char buffer[32];
  const uint32_t randomNumber = random(100000000u, 1000000000u);
  snprintf(buffer, sizeof(buffer), "/cali_%09u.json", randomNumber);
  return String(buffer);
}

void dumpCalibrationFileListImpl() {
  File root = LittleFS.open("/");
  if (!root || !root.isDirectory()) {
    Serial.println("LittleFS root is not available");
    return;
  }

  bool foundAny = false;
  Serial.println("All calibration files currently stored in LittleFS:");

  while (true) {
    File entry = root.openNextFile();
    if (!entry) {
      break;
    }

    String fileName = entry.name();
    String fullPath = fileName.startsWith("/") ? fileName : "/" + fileName;
    if (!entry.isDirectory() &&
        ((fullPath.startsWith("/cali_") && fullPath.endsWith(".json")) ||
         (fileName.startsWith("cali_") && fileName.endsWith(".json")))) {
      foundAny = true;
      Serial.print("  ");
      Serial.print(fullPath);
      Serial.print("  size=");
      Serial.println(entry.size());
    }
    entry.close();
  }

  if (!foundAny) {
    Serial.println("  <none>");
  }

  root.close();
}

void makeTestCalibration(JsonDocument& document) {
  document.clear();
  JsonObject root = document.to<JsonObject>();
  root["magic"] = kMagic;
  root["version"] = 1;
  root["brand"] = "STD";
  root["model"] = "123456789";
  root["pump_code"] = "123456789";
  root["operator_id"] = "";
  root["created_at"] = "2026-09-30T00:00:00+08:00";

  JsonObject temperature = root.createNestedObject("temperature_c");
  temperature["min"] = 27.84;
  temperature["max"] = 28.01;

  JsonObject config = root.createNestedObject("config");
  config["wait_ms"] = 4000;
  config["sample_count"] = 8;
  config["sample_interval_ms"] = 1000;
  JsonArray targetRpms = config.createNestedArray("target_rpms");
  const uint16_t targets[] = {900, 1300, 1700, 2100, 2500, 2700};
  for (uint8_t i = 0; i < sizeof(targets) / sizeof(targets[0]); ++i) {
    targetRpms.add(targets[i]);
  }

  JsonObject points = root.createNestedObject("points");
  JsonArray loadIndex = points.createNestedArray("load_index");
  JsonArray actualRpm = points.createNestedArray("actual_rpm_avg");
  JsonArray deltaPressure = points.createNestedArray("delta_pressure_bar");
  JsonArray flow = points.createNestedArray("flow_mlmin");

  for (uint8_t load = 1; load <= 4; ++load) {
    for (uint8_t i = 0; i < sizeof(targets) / sizeof(targets[0]); ++i) {
      const float target = static_cast<float>(targets[i]);
      const float speedFactor = target / 900.0f;
      loadIndex.add(load);
      actualRpm.add(target - load * 1.5f - i * 0.2f);
      deltaPressure.add(0.18f * load * speedFactor * speedFactor);
      flow.add(target * 0.31f - load * 2.0f);
    }
  }

  JsonObject summary = root.createNestedObject("summary");
  summary["a_standard_ml_per_rev"] = 0.3163;
  summary["a_speed_sd_ml_per_rev"] = 0.0003;
  summary["a_speed_cv_percent"] = 0.09;
  summary["b_standard_ml_per_min_per_bar"] = 8.94;
  summary["b_speed_sd_ml_per_min_per_bar"] = 0.61;
  summary["b_speed_cv_percent"] = 6.83;
  summary["r_squared_standard_mean"] = 0.9995;

  JsonObject fitRules = root.createNestedObject("fit_rules");
  fitRules["minimum_r_squared"] = 0.95;
  fitRules["minimum_pressure_span_bar"] = 0.8;

  JsonObject approval = root.createNestedObject("approval");
  approval["all_requirements_passed"] = true;
}

bool hasExpectedSchema(const JsonDocument& document) {
  if (document["magic"] != kMagic || document["version"] != 1 ||
      !document["temperature_c"].is<JsonObjectConst>() ||
      !document["config"].is<JsonObjectConst>() ||
      !document["points"].is<JsonObjectConst>() ||
      !document["summary"].is<JsonObjectConst>() ||
      !document["fit_rules"].is<JsonObjectConst>() ||
      !document["approval"].is<JsonObjectConst>()) {
    return false;
  }

  const JsonArrayConst loadIndex =
      document["points"]["load_index"].as<JsonArrayConst>();
  const JsonArrayConst actualRpm =
      document["points"]["actual_rpm_avg"].as<JsonArrayConst>();
  const JsonArrayConst deltaPressure =
      document["points"]["delta_pressure_bar"].as<JsonArrayConst>();
  const JsonArrayConst flow = document["points"]["flow_mlmin"].as<JsonArrayConst>();
  const JsonArrayConst targetRpms =
      document["config"]["target_rpms"].as<JsonArrayConst>();

  return loadIndex.size() == kPointCount && actualRpm.size() == kPointCount &&
         deltaPressure.size() == kPointCount && flow.size() == kPointCount &&
         targetRpms.size() == 6;
}
}  // namespace

const char* getCurrentTestFilePath() {
  return gCurrentFilePath.c_str();
}

void resetCurrentFile() {
  gCurrentFilePath = "";
  gInitialized = false;
}

void printCalibrationFileList() {
  dumpCalibrationFileListImpl();
}

bool formatFilesystem() {
  Serial.println("Formatting LittleFS...");
  if (!LittleFS.format()) {
    Serial.println("LittleFS format failed");
    return false;
  }

  if (!LittleFS.begin(false)) {
    Serial.println("LittleFS mount after formatting failed");
    return false;
  }

  gCurrentFilePath = "";
  gInitialized = false;
  return true;
}

bool begin(bool forceFormat) {
  if (forceFormat) {
    if (!formatFilesystem()) {
      return false;
    }
  }

  if (!LittleFS.begin(false)) {
    return false;
  }

  if (!gInitialized || gCurrentFilePath.length() == 0) {
    do {
      gCurrentFilePath = buildRandomTestFilePath();
    } while (LittleFS.exists(gCurrentFilePath.c_str()));
    gInitialized = true;
    Serial.print("Boot: selecting a new active file -> ");
    Serial.println(gCurrentFilePath);
    Serial.println("Current LittleFS root before create:");
    dumpCalibrationFileListImpl();
  }

  return true;
}

bool saveTestCalibration() {
  if (!gInitialized || gCurrentFilePath.length() == 0) {
    if (!begin()) {
      return false;
    }
  }

  DynamicJsonDocument document(4096);
  makeTestCalibration(document);

  File file = LittleFS.open(gCurrentFilePath.c_str(), "w");
  if (!file) {
    return false;
  }

  const size_t expectedBytes = measureJson(document);
  const size_t writtenBytes = serializeJson(document, file);
  file.close();

  if (writtenBytes == expectedBytes) {
    Serial.print("Write OK: ");
    Serial.println(gCurrentFilePath);
    Serial.println("Current LittleFS root after save:");
    dumpCalibrationFileListImpl();
    return true;
  }

  return false;
}

bool loadTestCalibration(JsonDocument& document) {
  if (!gInitialized || gCurrentFilePath.length() == 0) {
    if (!begin()) {
      return false;
    }
  }

  File file = LittleFS.open(gCurrentFilePath.c_str(), "r");
  if (!file) {
    return false;
  }

  const DeserializationError error = deserializeJson(document, file);
  file.close();
  return !error && hasExpectedSchema(document);
}

bool loadOrCreateTestCalibration(JsonDocument& document, bool& created) {
  created = true;
  if (!gInitialized || gCurrentFilePath.length() == 0) {
    if (!begin()) {
      return false;
    }
  }

  if (!saveTestCalibration()) {
    return false;
  }

  Serial.print("Verify active file: ");
  Serial.println(gCurrentFilePath);
  return loadTestCalibration(document);
}

}  // namespace calibration_store
