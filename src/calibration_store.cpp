#include "calibration_store.h"

#include <Arduino.h>
#include <LittleFS.h>

namespace calibration_store {
namespace {
constexpr const char* kMagic = "PPC1_REF_V1";
constexpr size_t kPointCount = 24;

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

bool begin() {
  return LittleFS.begin(true);
}

bool saveTestCalibration() {
  DynamicJsonDocument document(4096);
  makeTestCalibration(document);

  File file = LittleFS.open(kTestFilePath, "w");
  if (!file) {
    return false;
  }

  const size_t expectedBytes = measureJson(document);
  const size_t writtenBytes = serializeJson(document, file);
  file.close();
  return writtenBytes == expectedBytes;
}

bool loadTestCalibration(JsonDocument& document) {
  File file = LittleFS.open(kTestFilePath, "r");
  if (!file) {
    return false;
  }

  const DeserializationError error = deserializeJson(document, file);
  file.close();
  return !error && hasExpectedSchema(document);
}

bool loadOrCreateTestCalibration(JsonDocument& document, bool& created) {
  created = false;
  if (LittleFS.exists(kTestFilePath)) {
    return loadTestCalibration(document);
  }

  if (!saveTestCalibration()) {
    return false;
  }

  created = true;
  return loadTestCalibration(document);
}

}  // namespace calibration_store
