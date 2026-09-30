#include <Arduino.h>
#include <ArduinoJson.h>

#include "calibration_store.h"

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
}

void loop() {
  static uint32_t lastTick = 0;
  if (millis() - lastTick >= 1000) {
    lastTick = millis();
    Serial.printf("heartbeat %lu ms\n", lastTick);
  }
}
