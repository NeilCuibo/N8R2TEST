#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>

#include "calibration_store.h"

namespace {
void waitForFormatCommand() {
  Serial.println("LittleFS mount failed or is not readable.");
  Serial.println("Press '1' and then Enter to format LittleFS.");
  Serial.println("Press any other key to keep current state.");

  while (true) {
    if (Serial.available()) {
      const char ch = Serial.read();
      if (ch == '1') {
        Serial.println("Formatting requested by user.");
        if (!calibration_store::formatFilesystem()) {
          Serial.println("Format failed.");
          return;
        }
        return;
      }

      while (Serial.available()) {
        Serial.read();
      }
      Serial.println("Format cancelled. System will remain unchanged.");
      return;
    }
    delay(50);
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);

  if (!calibration_store::begin()) {
    waitForFormatCommand();
    if (!calibration_store::begin()) {
      Serial.println("LittleFS still unavailable after user action.");
      return;
    }
  }

  DynamicJsonDocument calibration(4096);
  bool created = false;
  if (!calibration_store::loadOrCreateTestCalibration(calibration, created)) {
    Serial.println("Calibration JSON load/create failed.");
    Serial.println("Press '1' and then Enter to format LittleFS.");
    while (true) {
      if (Serial.available()) {
        const char ch = Serial.read();
        if (ch == '1') {
          Serial.println("Formatting requested by user.");
          if (!calibration_store::formatFilesystem()) {
            Serial.println("Format failed.");
            return;
          }
          if (!calibration_store::begin()) {
            Serial.println("LittleFS still unavailable after format.");
            return;
          }
          break;
        }
        while (Serial.available()) {
          Serial.read();
        }
        Serial.println("Format cancelled.");
        return;
      }
      delay(50);
    }
  }

  Serial.println(created ? "Created and saved test calibration JSON"
                         : "Loaded saved test calibration JSON");
  Serial.print("Active file: ");
  Serial.println(calibration_store::getCurrentTestFilePath());
  Serial.println("==================================================");
  calibration_store::printCalibrationFileList();
  Serial.println("==================================================");
}

void loop() {
  if (Serial.available()) {
    const char ch = Serial.read();
    if (ch == '1') {
      Serial.println("Manual format requested. Formatting LittleFS...");
      if (calibration_store::formatFilesystem()) {
        Serial.println("Format successful. Restarting...");
        ESP.restart();
      }
      Serial.println("Format failed or cancelled.");
    }

    while (Serial.available()) {
      Serial.read();
    }
  }

  static uint32_t lastTick = 0;
  if (millis() - lastTick >= 1000) {
    lastTick = millis();
    Serial.printf("heartbeat %lu ms\n", lastTick);
  }
}
