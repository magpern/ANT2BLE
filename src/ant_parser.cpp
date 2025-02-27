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
    bleData = {0, 0, 0};
    newData = false;
}

void ANTParser::processANTMessage(uint8_t *data, uint8_t length) {
    if (length != 8) {
        LOG("[ERROR] Invalid ANT+ message length");
        return;
    }

    AntFeData feData = {};
    feData.page = data[0];

    switch (feData.page) {
        case PAGE_GENERAL_FE_DATA:
            parseGeneralFeData(data, feData);
            break;
        case PAGE_TRAINER_DATA:
            parseTrainerData(data, feData);
            break;
        case PAGE_TRAINER_STATUS:
            parseTrainerStatus(data, feData);
            break;
        case PAGE_MANUFACTURER_ID:
            parseManufacturerID(data);
            break;
        case PAGE_PRODUCT_INFO:
            parseProductInfo(data);
            break;
        default:
            LOGF("[WARN] Unhandled ANT+ Page: 0x%02X", feData.page);
            return;
    }

    // Convert ANT+ parsed data to BLEData format
    bleData.power = feData.power;
    bleData.speed = feData.speed;
    bleData.cadence = feData.cadence;
    newData = true;

    LOGF("[DEBUG] Parsed ANT+ Data: Power=%dW, Speed=%d km/h, Cadence=%d rpm",
         bleData.power, bleData.speed, bleData.cadence);
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

            // ✅ Strip Sync Byte and CRC Before Processing
            uint8_t payloadLength = expectedLength - 3;  // Remove Sync Byte & CRC
            uint8_t processedMessage[payloadLength];

            for (uint8_t i = 0; i < payloadLength; i++) { // 
                processedMessage[i] = buffer[i + 2];  // Shift left to remove Sync Byte and length
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
    if (length < 4) {  // Ensure there's a valid payload
        LOG("[ERROR] Invalid Serial Command: Too Short");
        return;
    }

    // ✅ Extract command as a string (excluding sync byte, length, and CRC)
    String command = "";
    for (int i = 1; i < length; i++) {  
        command += (char)data[i];
    }

    LOGF("[DEBUG] Received Serial Command: %s", command.c_str());

    // ✅ Handle "SETNAME" Command
    if (command.startsWith("SETNAME ")) {
        String newName = command.substring(8);  // Extract name after "SETNAME "
        if (newName.length() < 3 || newName.length() > 20) {  // Validate name length
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
    }
    // ✅ Handle "REBOOT" Command
    else if (command == "REBOOT") {
        LOG("[INFO] Reboot command received! Restarting ESP32...");
        delay(500);
        esp_restart();
    }
    // ✅ Handle "STATUS" Command
    else if (command == "STATUS") {
        LOG("[INFO] Sending System Status...");
        LOGF("[INFO] BLE Device Name: %s", preferences.getString("ble_name", "ESP32-S3 FTMS").c_str());
        LOGF("[INFO] Device MAC: %s", NimBLEDevice::getAddress().toString().c_str());
    }
    // ✅ Handle "HELP" Command
    else if (command == "HELP") {
        LOG("[INFO] Available Commands:");
        LOG("[INFO]  - SETNAME <new_name> : Change BLE device name");
        LOG("[INFO]  - REBOOT            : Restart ESP32");
        LOG("[INFO]  - STATUS            : Display system status");
        LOG("[INFO]  - HELP              : Show this help message");
    }
    else {
        LOGF("[ERROR] Unknown Command: %s", command.c_str());
    }
}


// Parse General Fitness Equipment Data (Page 16 / 0x10)
void ANTParser::parseGeneralFeData(const uint8_t* data, AntFeData& feData) {
    feData.elapsed_time = data[1];
    feData.distance = data[2];
    feData.speed = (data[3] | (data[4] << 8)) / 100; // Convert to km/h
    feData.power = (data[6] | (data[7] << 8));

    LOGF("[ANT+] General FE Data - Time: %d s, Distance: %d m, Speed: %d km/h, Power: %d W",
         feData.elapsed_time, feData.distance, feData.speed, feData.power);
}

void ANTParser::parseTrainerData(const uint8_t* data, AntFeData& feData) {
    feData.elapsed_time = data[1];
    feData.distance = data[2];
    feData.speed = (data[3] | (data[4] << 8)) / 100; // Convert to km/h
    feData.cadence = data[5];
    feData.power = (data[6] | (data[7] << 8));

    LOGF("[ANT+] Trainer Data - Time: %d s, Distance: %d m, Speed: %d km/h, Cadence: %d rpm, Power: %d W",
         feData.elapsed_time, feData.distance, feData.speed, feData.cadence, feData.power);
}

void ANTParser::parseTrainerStatus(const uint8_t* data, AntFeData& feData) {
    feData.resistance = data[4];

    LOGF("[ANT+] Trainer Status - Resistance: %d", feData.resistance);
}

void ANTParser::parseManufacturerID(const uint8_t* data) {
    uint8_t hardwareRev = data[1];
    uint16_t manufacturerID = data[2] | (data[3] << 8);
    uint16_t serialNumber = data[4] | (data[5] << 8);

    LOGF("[ANT+] Manufacturer ID - Manufacturer: %d, Hardware Rev: %d, Serial: %d",
         manufacturerID, hardwareRev, serialNumber);
}

void ANTParser::parseProductInfo(const uint8_t* data) {
    uint8_t softwareVersion = data[1];
    uint8_t modelNumber = data[2];

    LOGF("[ANT+] Product Info - Software Version: %d, Model Number: %d",
         softwareVersion, modelNumber);
}
