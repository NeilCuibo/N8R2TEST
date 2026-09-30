#include <Arduino.h>
#include <ArduinoJson.h>
#include <unity.h>

#include "calibration_store.h"

void test_create_save_and_reload_calibration_json() {
  TEST_ASSERT_TRUE(calibration_store::begin());

  DynamicJsonDocument document(4096);
  bool created = false;
  TEST_ASSERT_TRUE(
      calibration_store::loadOrCreateTestCalibration(document, created));

  // Rewriting the current run's file makes this test repeatable.
  TEST_ASSERT_TRUE(calibration_store::saveTestCalibration());
  TEST_ASSERT_TRUE(calibration_store::loadTestCalibration(document));

  TEST_ASSERT_EQUAL_STRING("PPC1_REF_V1", document["magic"] | "");
  TEST_ASSERT_EQUAL_INT(1, document["version"] | 0);
  TEST_ASSERT_TRUE(document["temperature_c"].is<JsonObjectConst>());
  TEST_ASSERT_TRUE(document["config"].is<JsonObjectConst>());
  TEST_ASSERT_TRUE(document["points"].is<JsonObjectConst>());
  TEST_ASSERT_TRUE(document["summary"].is<JsonObjectConst>());
  TEST_ASSERT_TRUE(document["fit_rules"].is<JsonObjectConst>());
  TEST_ASSERT_TRUE(document["approval"].is<JsonObjectConst>());

  TEST_ASSERT_EQUAL_UINT(6,
      document["config"]["target_rpms"].as<JsonArrayConst>().size());
  TEST_ASSERT_EQUAL_UINT(24,
      document["points"]["load_index"].as<JsonArrayConst>().size());
  TEST_ASSERT_EQUAL_UINT(24,
      document["points"]["actual_rpm_avg"].as<JsonArrayConst>().size());
  TEST_ASSERT_EQUAL_UINT(24,
      document["points"]["delta_pressure_bar"].as<JsonArrayConst>().size());
  TEST_ASSERT_EQUAL_UINT(24,
      document["points"]["flow_mlmin"].as<JsonArrayConst>().size());

  // A second load reads the already-saved file again.
  DynamicJsonDocument reloaded(4096);
  TEST_ASSERT_TRUE(calibration_store::loadTestCalibration(reloaded));
  TEST_ASSERT_EQUAL_STRING("PPC1_REF_V1", reloaded["magic"] | "");
  TEST_ASSERT_EQUAL_INT(24,
      reloaded["points"]["flow_mlmin"].as<JsonArrayConst>().size());
}

void setup() {
  delay(200);
  UNITY_BEGIN();
  RUN_TEST(test_create_save_and_reload_calibration_json);
  UNITY_END();
}

void loop() {}
