#include <unity.h>
#include "ant_parser.h"

ANTParser parser;

void test_parse_power() {
    uint8_t test_data[] = {0x10, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00};  // Power: 50W
    parser.processANTMessage(test_data, 8);
    BLEData result = parser.getBLEData();
    TEST_ASSERT_EQUAL_UINT16(50, result.power);
}

void test_parse_speed_cadence() {
    uint8_t test_data[] = {0x1E, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x96};  // Speed: 100, Cadence: 150
    parser.processANTMessage(test_data, 8);
    BLEData result = parser.getBLEData();
    TEST_ASSERT_EQUAL_UINT16(100, result.speed);
    TEST_ASSERT_EQUAL_UINT8(150, result.cadence);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_parse_power);
    RUN_TEST(test_parse_speed_cadence);
    UNITY_END();
}

void loop() {}
