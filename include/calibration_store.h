#pragma once

#include <ArduinoJson.h>

namespace calibration_store {

constexpr const char* kTestFilePath = "/test_cali_123456789.json";

bool begin();
bool saveTestCalibration();
bool loadTestCalibration(JsonDocument& document);
bool loadOrCreateTestCalibration(JsonDocument& document, bool& created);

}  // namespace calibration_store
