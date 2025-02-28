#include "ant_parser.h"
#include "logger.h"
#include "global.h"
#include <NimBLEDevice.h>

// ANT+ Fitness Equipment Data Pages
#define PAGE_GENERAL_FE_DATA 0x10
#define PAGE_TRAINER_DATA 0x19
#define PAGE_TRAINER_STATUS 0x11
#define PAGE_MANUFACTURER_ID 0x50  // Page 80
#define PAGE_PRODUCT_INFO 0x51     // Page 81

ANTParser::ANTParser() {
    ftmsData = {};  // Initialize all values to defaults
    newData = false;
}

void ANTParser::processANTMessage(uint8_t *data, uint8_t length) {

    uint8_t page = data[0];
    switch (page) {
        case PAGE_GENERAL_FE_DATA:
            parseGeneralFeData(data);
            break;
        case PAGE_TRAINER_DATA:
            parseTrainerData(data);
            break;
        case PAGE_TRAINER_STATUS:
            parseTrainerStatus(data);
            break;
        case PAGE_MANUFACTURER_ID:
            parseManufacturerID(data);
            break;
        case PAGE_PRODUCT_INFO:
            parseProductInfo(data);
            break;
        default:
            LOGF("[WARN] Unhandled ANT+ Page: 0x%02X", page);
            return;
    }

    // ✅ Mark data as valid once any valid ANT+ message is received
    ftmsData.hasData = true;
    newData = true;
}

FTMSDataStorage ANTParser::getFTMSData() {
    return ftmsData;
}

void ANTParser::parseGeneralFeData(const uint8_t* data) {
    ftmsData.elapsed_time = 0;  // ✅ Always 0
    ftmsData.distance = 0;  // ✅ Always 0
    ftmsData.speed = ((data[4] | (data[5] << 8)) * 0.001) * 3.6;  // ✅ Convert to km/h
    ftmsData.heart_rate = 0xFF;  // ✅ Always invalid
    ftmsData.fe_state = (data[7] >> 4);  // ✅ Extract FE State

    LOGF("[ANT+] General FE Data - Speed: %.2f km/h, HR: 0x%X, State: %d",
         ftmsData.speed, ftmsData.heart_rate, ftmsData.fe_state);
}



void ANTParser::parseTrainerData(const uint8_t* data) {
    ftmsData.cadence = data[5];

    LOGF("[ANT+] Trainer Data - Cadence: %d rpm", ftmsData.cadence);
}

void ANTParser::parseTrainerStatus(const uint8_t* data) {
    ftmsData.cycle_length = data[3] * 0.01;  // ✅ Convert to meters
    ftmsData.incline = (int16_t)(data[4] | (data[5] << 8)) * 0.01;  // ✅ Convert to percentage
    ftmsData.resistance = data[6] * 0.5;  // ✅ Convert to % of max resistance
    ftmsData.fe_state = (data[7] >> 4);  // ✅ Extract FE State

    LOGF("[ANT+] Trainer Status - Cycle Length: %.2fm, Incline: %.2f%%, Resistance: %.1f%%, State: %d",
         ftmsData.cycle_length, ftmsData.incline, ftmsData.resistance, ftmsData.fe_state);
}


void ANTParser::parseManufacturerID(const uint8_t* data) {
    ftmsData.manufacturerID = data[2] | (data[3] << 8);
    ftmsData.serialNumber = data[4] | (data[5] << 8);

    LOGF("[ANT+] Manufacturer ID - Manufacturer: %d, Serial: %d",
         ftmsData.manufacturerID, ftmsData.serialNumber);
}

void ANTParser::parseProductInfo(const uint8_t* data) {
    ftmsData.softwareVersion = (data[3] * 100) + data[2];  // ✅ Correct SW Version encoding
    ftmsData.serialNumber = (data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24));  // ✅ 4-byte Serial Number

    LOGF("[ANT+] Product Info - Software Version: %d.%d, Serial Number: %u",
         ftmsData.softwareVersion / 100, ftmsData.softwareVersion % 100, ftmsData.serialNumber);
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
            if (byteReceived == 0xA4 || byteReceived == 0xF0) {  // ANT+ or Custom Sync Byte
                index = 0;
                receiving = true;
                expectedLength = 0;
                buffer[index++] = byteReceived;
                LOGF("[DEBUG] Start-of-Message (0x%02X) Detected", byteReceived);
            }
            continue;
        }

        buffer[index++] = byteReceived;

        // ✅ Set expected message length correctly
        if (index == 2) {
            expectedLength = buffer[1] + 2;  // Length + Payload + CRC
            LOGF("[DEBUG] Corrected Packet Length: %d bytes", expectedLength);
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

            // ✅ Corrected Payload Extraction
            uint8_t payloadLength = expectedLength - 2;  // ✅ Remove Sync Byte & CRC
            uint8_t processedMessage[payloadLength];

            for (uint8_t i = 0; i < payloadLength; i++) {
                processedMessage[i] = buffer[i + 2];  // ✅ Corrected Start Index
            }

            // ✅ Detect Custom or ANT+ Message
            if (buffer[0] == 0xF0) {
                processSerialCommand(processedMessage, payloadLength);
            } else {
                processANTMessage(processedMessage, payloadLength);
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
    if (length < 2) {  // Ensure there's a valid payload
        LOG("[ERROR] Invalid Serial Command: Too Short");
        return;
    }

    // ✅ Extract command as a string
    String command = "";
    for (int i = 0; i < length; i++) {  
        command += (char)data[i];
    }

    LOGF("[DEBUG] Received Serial Command: %s", command.c_str());

    if (command.startsWith("SETNAME ")) {
        String newName = command.substring(8);  
        if (newName.length() < 3 || newName.length() > 20) {
            LOG("[ERROR] Invalid BLE Name: Must be 3-20 characters");
            return;
        }

        preferences.begin("ble_ftms", false);
        preferences.putString("ble_name", newName);
        preferences.end();

        LOGF("[INFO] BLE Name Set: %s", newName.c_str());
        LOG("[INFO] Rebooting to apply changes...");
        delay(500);
        esp_restart();
    } else if (command == "REBOOT") {
        LOG("[INFO] Reboot command received! Restarting ESP32...");
        delay(500);
        esp_restart();
    } else {
        LOGF("[ERROR] Unknown Command: %s", command.c_str());
    }
}
