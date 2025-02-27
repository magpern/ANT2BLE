#include "ant_parser.h"
#include "logger.h"
#include "global.h"

ANTParser::ANTParser() {
    bleData = {0, 0, 0};
    newData = false;
}

void ANTParser::processANTMessage(uint8_t *data, uint8_t length) {
    if (length < 4) return;  // Ensure valid message length

    uint8_t messageID = data[2];  // Third byte is Message ID
    uint8_t *payload = &data[3];  // Start of actual data

    LOGF("[DEBUG] Processing ANT+ Message ID: 0x%02X", messageID);

    switch (messageID) {
        case 0x10:  // Power Data Page
            bleData.power = (payload[3] << 8) | payload[2];  // Extract power
            break;

        case 0x1E: {  // ✅ Wrap variables in braces
            uint16_t rawSpeed = (payload[3] << 8) | payload[2];  // Extract raw speed
            bleData.cadence = payload[4];  // Extract cadence
            
            // ✅ Corrected Speed Conversion Formula
            float wheelCircumference = 2.105;  // Standard 700x25c bike (meters)
            float speedKmH = (rawSpeed * wheelCircumference * 3.6) / 16;  // Convert to km/h
            bleData.speed = (uint16_t)speedKmH;

            LOGF("[DEBUG] Raw Speed: %d, Corrected Speed in km/h: %.2f",
                 rawSpeed, speedKmH);
            break;
        }

        default:
            LOGF("[DEBUG] Unknown ANT+ Message ID: 0x%02X", messageID);
            return;
    }

    LOGF("[DEBUG] Parsed ANT+ Data: Power=%dW, Speed=%d km/h, Cadence=%d rpm",
         bleData.power, bleData.speed, bleData.cadence);

    newData = true;  // ✅ Set flag so `hasNewData()` returns true
}


BLEData ANTParser::getBLEData() {
    return bleData;
}

bool ANTParser::hasNewData() {
    if (newData) {
        newData = false;  // ✅ Reset flag after processing
        return true;
    }
    return false;
}


void ANTParser::readSerial() {
    static uint8_t buffer[32];  // Buffer for ANT+ messages and serial commands
    static uint8_t index = 0;
    static uint8_t expectedLength = 0;
    static bool receiving = false;

    while (Serial.available()) {
        uint8_t byteReceived = Serial.read();
        LOGF("[DEBUG] Received Byte: 0x%02X", byteReceived);

        if (!receiving) {
            if (byteReceived == 0xA4) {  // ANT+ Sync Byte
                index = 0;
                receiving = true;
                expectedLength = 0;
                buffer[index++] = byteReceived;
                LOG("[DEBUG] Start-of-Message (0xA4) Detected");
            }
            continue;
        }

        buffer[index++] = byteReceived;

        // ✅ Set expected message length correctly
        if (index == 2) {
            expectedLength = buffer[1] + 2;  // Sync (1) + Length (1) + Payload + CRC (1)
            LOGF("[DEBUG] Corrected ANT+ Packet Length: %d bytes", expectedLength);
        }

        // ✅ Process message only when full length is received
        if (index == expectedLength) {  
            LOG("[DEBUG] Full Message Received:");
            for (uint8_t i = 0; i < index; i++) {
                LOGF("0x%02X ", buffer[i]);
            }

            // ✅ Validate CRC before processing
            if (!validateCRC(buffer, index - 1, buffer[index - 1])) {
                LOG("[ERROR] CRC Mismatch! Message Discarded.");
                index = 0;
                receiving = false;
                return;  // Ignore corrupted message
            }

            LOG("[DEBUG] CRC Check Passed ✅");

            // ✅ Detect Custom Command
            if (buffer[2] == 0xF0) {  // Custom command flag
                processSerialCommand(buffer, index - 1);
            } else {
                processANTMessage(buffer, index - 1);  // Process ANT+ message
            }

            index = 0;
            receiving = false;
        }
    }
}


bool ANTParser::validateCRC(uint8_t *payload, uint8_t length, uint8_t expectedCRC) {
    uint8_t calculatedCRC = 0;
    for (uint8_t i = 1; i < length; i++) {  // Skip Sync Byte (Byte 0)
        calculatedCRC ^= payload[i];  // Simple XOR-based CRC
    }

    LOGF("[DEBUG] Calculated CRC: 0x%02X, Expected CRC: 0x%02X", calculatedCRC, expectedCRC);
    
    return (calculatedCRC == expectedCRC);
}

void ANTParser::processSerialCommand(uint8_t *data, uint8_t length) {
    String command = "";
    for (int i = 3; i < length; i++) {  // Extract payload (skip 0xA4, length, and 0xF0)
        command += (char)data[i];
    }

    LOGF("[DEBUG] Received Serial Command: %s", command.c_str());

    if (command.startsWith("SETNAME ")) {
        String newName = command.substring(8);  // Extract name after "SETNAME "
        preferences.begin("ble_ftms", false);
        preferences.putString("ble_name", newName);
        preferences.end();

        LOGF("[INFO] BLE Name Set: %s", newName.c_str());
        LOG("[INFO] Rebooting to apply changes...");
        delay(500);
        esp_restart();
    } else {
        LOGF("[ERROR] Unknown Command: %s", command.c_str());
    }
}



