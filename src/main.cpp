#include <Arduino.h>
#include "ant_parser.h"
#include "ble_ftms.h"
#include "logger.h"
#include "esp_system.h"  // ESP32 system functions

#define SERIAL_BAUDRATE 115200

void checkForReboot();  // Function declaration

ANTParser antParser;
BLEFTMS bleFTMS;

void setup() {
    Serial.begin(SERIAL_BAUDRATE);  // ANT+ Data Input (Raspberry Pi -> ESP32)
    logger.begin(SERIAL_BAUDRATE, SERIAL_8N1, 9, 10);  // Debug Output via GPIO9/10

    LOG("ESP32-S3 ANT+ to BLE FTMS");

    // ✅ Ensure NimBLE logs go through logger
    setupLogger();
    // ✅ Redirect all `printf` calls to use logger
    esp_log_set_vprintf(logger_vprintf);

    // Initialize BLE FTMS
    bleFTMS.begin();

    // Print unique ESP32-S3 MAC address
    LOG("Device MAC: " + bleFTMS.getDeviceMAC());
}

void loop() {
    antParser.readSerial();

    if (antParser.hasNewData()) {
        BLEData data = antParser.getBLEData();
        bleFTMS.sendIndoorBikeData(data.power, data.speed, data.cadence);
        LOGF("[DEBUG] BLE FTMS Update: Power=%dW, Speed=%d km/h, Cadence=%d rpm", 
            data.power, data.speed, data.cadence);
    }

    checkForReboot();  // Check if "reboot" command is received
}

// Function to listen for "reboot" command on Serial1
void checkForReboot() {
    static char inputBuffer[10];  // Small buffer for command
    static uint8_t index = 0;

    while (logger.available()) {
        char c = logger.read();
        logger.print("[DEBUG] Received: ");
        logger.println(c);  // Debug: Show each received character

        if (c == '\n' || c == '\r') {  // End of command
            inputBuffer[index] = '\0';  // Null-terminate string
            logger.print("[DEBUG] Full command: ");
            logger.println(inputBuffer);

            if (strcasecmp(inputBuffer, "reboot") == 0) {  // Case-insensitive compare
                logger.println("[DEBUG] Reboot command received! Restarting ESP32...");
                delay(500);
                logger.flush();  // Ensure log output is sent
                esp_restart();   // Perform software reboot
            }
            index = 0;  // Reset buffer index
        } else if (index < sizeof(inputBuffer) - 1) {
            inputBuffer[index++] = c;  // Store character
        }
    }

    // 🚀 EXTRA FIX: If full "reboot" command is detected *without newline*
    if (strcasecmp(inputBuffer, "reboot") == 0) {
        logger.println("[DEBUG] No newline, but command detected! Restarting...");
        delay(500);
        logger.flush();
        esp_restart();
    }
}


