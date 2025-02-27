#ifdef ARDUINO_ARCH_ESP32  // Skip BLE tests on Windows

#include <Arduino.h>
#include <unity.h>
#include "ble_ftms.h"

BLEFTMS bleFTMS;

void test_ble_data_format() {
    uint8_t data[8];
    bleFTMS.prepareFTMSData(data, 200, 30, 80);
    
    TEST_ASSERT_EQUAL_UINT8(0x04 | 0x02, data[0]);  // Check FTMS flags
    TEST_ASSERT_EQUAL_UINT8(200 & 0xFF, data[1]);   // Power LSB
    TEST_ASSERT_EQUAL_UINT8((200 >> 8) & 0xFF, data[2]);  // Power MSB
    TEST_ASSERT_EQUAL_UINT8(30 & 0xFF, data[3]);    // Speed LSB
    TEST_ASSERT_EQUAL_UINT8((30 >> 8) & 0xFF, data[4]); // Speed MSB
    TEST_ASSERT_EQUAL_UINT8(80, data[5]);           // Cadence
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_ble_data_format);
    UNITY_END();
}

void loop() {}

#endif  // ARDUINO_ARCH_ESP32
