#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>

#include "calibration_store.h"

namespace {
void printLittleFSRootFiles() {
  File root = LittleFS.open("/");
  if (!root || !root.isDirectory()) {
    Serial.println("LittleFS root scan failed");
    return;
  }

  Serial.println("LittleFS root files:");
  while (true) {
    File entry = root.openNextFile();
    if (!entry) {
      break;
    }

    if (!entry.isDirectory()) {
      Serial.print(entry.name());
      Serial.print(" | ");
      Serial.print(entry.size());
      Serial.println(" bytes");
    }
    entry.close();
  }
  root.close();
  Serial.println("End of LittleFS root files");
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);

  if (!calibration_store::begin()) {
    Serial.println("LittleFS mount failed");
    return;
  }

  DynamicJsonDocument calibration(4096);
  bool created = false;
  if (!calibration_store::loadOrCreateTestCalibration(calibration, created)) {
    Serial.println("Calibration JSON load/create failed");
    return;
  }

  Serial.println(created ? "Created and saved test calibration JSON"
                         : "Loaded saved test calibration JSON");
  Serial.print("File: ");
  Serial.println(calibration_store::getCurrentTestFilePath());
  printLittleFSRootFiles();
}

void loop() {
  static uint32_t lastTick = 0;
  if (millis() - lastTick >= 1000) {
    lastTick = millis();
    Serial.printf("heartbeat %lu ms\n", lastTick);
  }
}
