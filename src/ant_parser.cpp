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
#define PAGE_FE_Capabilities 0x54  // Page 84

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
        case PAGE_FE_Capabilities:
            parseFECapabilities(data);
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
    // ✅ Extract Equipment Type
    uint8_t equipmentType = data[1];  

    // ✅ Convert Elapsed Time
    ftmsData.elapsed_time = (data[2] == 0xFF) ? 0 : data[2] * 0.25;  

    // ✅ Convert Distance
    ftmsData.distance = (data[3] == 0xFF) ? 0 : data[3];

    // ✅ Correct Speed Extraction (Little-Endian)
    uint16_t rawSpeed = data[4] | (data[5] << 8);
    ftmsData.speed = (rawSpeed == 0xFFFF) ? 0.0 : (rawSpeed * 0.001 * 3.6);

    // ✅ Extract Heart Rate
    ftmsData.heart_rate = (data[6] == 0xFF) ? 0 : data[6];

    // ✅ Extract Capabilities and FE State
    uint8_t capabilities = data[7] & 0x0F;  // Bits 0-3
    ftmsData.fe_state = (data[7] >> 4);  // Bits 4-7

    // ✅ Debug Output
    LOGF("[ANT+] General FE Data - Equipment: %d, Speed: %.2f km/h, Distance: %d m, HR: %d, State: %d",
         equipmentType, ftmsData.speed, ftmsData.distance, ftmsData.heart_rate, ftmsData.fe_state);
}



void ANTParser::parseTrainerData(const uint8_t* data) {
    // ✅ Extract Instantaneous Cadence (Byte 2)
    ftmsData.cadence = (data[2] == 0xFF) ? 0 : data[2];

    // ✅ Extract Accumulated Power (Bytes 3-4, Little-Endian)
    uint16_t accumulatedPower = data[3] | (data[4] << 8);
    ftmsData.accumulated_power = (accumulatedPower == 0xFFFF) ? 0 : accumulatedPower;

    // ✅ Extract Instantaneous Power (12-bit, LSB in Byte 5, MSB upper 4 bits of Byte 6)
    uint16_t instantaneousPower = (data[5] | ((data[6] & 0x0F) << 8));
    ftmsData.instantaneous_power = (instantaneousPower == 0xFFF) ? 0 : instantaneousPower;

    // ✅ Extract Trainer Status (Bits 4-7 of Byte 6)
    ftmsData.trainer_status = (data[6] >> 4);

    // ✅ Extract Flags (Bits 0-3 of Byte 7)
    ftmsData.virtual_speed = data[7] & 0x01;  // ✅ Bit 0: 1 = Virtual, 0 = Real

    // ✅ Extract FE State (Bits 4-7 of Byte 7)
    ftmsData.fe_state = (data[7] >> 4);

    LOGF("[ANT+] Trainer Data - Power: %d W, Cadence: %d rpm, Accumulated Power: %d W, Status: %d, Virtual Speed: %d, FE State: %d",
         ftmsData.instantaneous_power, ftmsData.cadence, ftmsData.accumulated_power,
         ftmsData.trainer_status, ftmsData.virtual_speed, ftmsData.fe_state);
}

void ANTParser::parseTrainerStatus(const uint8_t* data) {
    // ✅ Reserved Fields (Ignore)
    
    // ✅ Extract Cycle Length (meters)
    ftmsData.cycle_length = (data[3] == 0xFF) ? 0.0 : data[3] * 0.01;

    // ✅ Extract Incline (Signed, Little-Endian)
    int16_t rawIncline = (int16_t)(data[4] | (data[5] << 8));
    ftmsData.incline = (rawIncline == 0x7FFF) ? 0.0 : rawIncline * 0.01;

    // ✅ Extract Resistance Level (%)
    ftmsData.resistance = (data[6] == 0xFF) ? 0.0 : data[6] * 0.5;

    // ✅ Extract Capabilities and FE State
    uint8_t capabilities = data[7] & 0x0F;  // Bits 0-3
    ftmsData.fe_state = (data[7] >> 4);  // Bits 4-7

    // ✅ Debug Output
    LOGF("[ANT+] Trainer Status - Cycle Length: %.2fm, Incline: %.2f%%, Resistance: %.1f%%, FE State: %d",
         ftmsData.cycle_length, ftmsData.incline, ftmsData.resistance, ftmsData.fe_state);
}

void ANTParser::parseFECapabilities(const uint8_t* data) {
    // ✅ Corrected Maximum Resistance Extraction (Little-Endian)
    uint16_t maxResistance = data[4] | (data[5] << 8);
    ftmsData.maxResistance = (maxResistance == 0xFFFF) ? 0 : maxResistance;

    // ✅ Extract Capabilities Bit Field (Byte 6)
    uint8_t capabilities = data[6];  // <-- Corrected to read **1 byte** only!

    // ✅ Decode Capabilities
    bool simulationMode = capabilities & 0x01;
    bool ergMode = capabilities & 0x02;
    bool resistanceMode = capabilities & 0x04;
    bool windMode = capabilities & 0x08;
    bool trackMode = capabilities & 0x10;

    // ✅ Debug Output
    LOGF("[ANT+] FE Capabilities - Max Resistance: %d N, Simulation Mode: %d, ERG Mode: %d, Resistance Mode: %d, Wind Mode: %d, Track Mode: %d",
         ftmsData.maxResistance, simulationMode, ergMode, resistanceMode, windMode, trackMode);
}

void ANTParser::parseManufacturerID(const uint8_t* data) {
    // ✅ Ignore Reserved Bytes (data[1] and data[2])
    
    // ✅ Extract HW Revision (Byte 3)
    ftmsData.hardware_revision = (data[3] == 0xFF) ? 0 : data[3];

    // ✅ Extract Manufacturer ID (Bytes 4-5, Little-Endian)
    uint16_t rawManufacturerID = data[4] | (data[5] << 8);
    ftmsData.manufacturerID = (rawManufacturerID == 0xFFFF) ? 0 : rawManufacturerID;

    // ✅ Extract Model Number (Bytes 6-7, Little-Endian)
    uint16_t rawModelNumber = data[6] | (data[7] << 8);
    ftmsData.modelNumber = (rawModelNumber == 0xFFFF) ? 0 : rawModelNumber;

    LOGF("[ANT+] Manufacturer ID - HW Rev: %d, Manufacturer: %d, Model: %d",
         ftmsData.hardware_revision, ftmsData.manufacturerID, ftmsData.modelNumber);
}

void ANTParser::parseProductInfo(const uint8_t* data) {
    // ✅ Ignore Reserved Byte (data[1])
    
    // ✅ Extract SW Revision (Bytes 2-3)
    uint8_t swRevisionSupplemental = (data[2] == 0xFF) ? 0 : data[2];
    uint8_t swRevisionMain = (data[3] == 0xFF) ? 0 : data[3];

    // ✅ If Supplemental Revision is `0xFF`, use only the Main Revision
    if (swRevisionSupplemental == 0) {
        ftmsData.softwareVersion = swRevisionMain;
    } else {
        // ✅ Otherwise, compute full SW version using Equation 6-3
        ftmsData.softwareVersion = (swRevisionMain * 100) + swRevisionSupplemental;
    }

    // ✅ Extract Serial Number (Bytes 4-7, Little-Endian)
    uint32_t rawSerialNumber = data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24);
    ftmsData.serialNumber = (rawSerialNumber == 0xFFFFFFFF) ? 0 : rawSerialNumber;

    LOGF("[ANT+] Product Info - SW Version: %d, Serial Number: %u",
         ftmsData.softwareVersion, ftmsData.serialNumber);
}

void ANTParser::readSerial() {
    static uint8_t buffer[32];  // Buffer for ANT+ and custom messages
    static uint8_t index = 0;
    static uint8_t expectedLength = 0;
    static bool receiving = false;

    while (Serial.available()) {
        uint8_t byteReceived = Serial.read();

        if (!receiving) {
            if (byteReceived == 0xA4 || byteReceived == 0xF0) {  // ANT+ or Custom Sync Byte
                index = 0;
                receiving = true;
                expectedLength = 0;
                buffer[index++] = byteReceived;
            }
            continue;
        }

        buffer[index++] = byteReceived;

        // ✅ Set expected message length correctly (Payload Length + Sync + CRC)
        if (index == 2) {
            expectedLength = buffer[1] + 3;  // ✅ Corrected length calculation
        }

        // ✅ Process message only when full length is received
        if (index == expectedLength) {
            /*
                logger.print("[DEBUG] Message Buffer Before CRC Check: ");
                for (uint8_t i = 0; i < index; i++) {
                    logger.printf("%02X ", buffer[i]);
                }
                LOG("");  // Newline for formatting
            */
            // ✅ Fix: Pass only the length byte + payload (exclude sync & CRC)
            if (!validateCRC(buffer + 1, index - 2, buffer[index - 1])) {
                LOG("[ERROR] CRC Mismatch! Message Discarded.");
                index = 0;
                receiving = false;
                return;  // Ignore corrupted message
            }

            //LOG("[DEBUG] CRC Check Passed ✅");

            // ✅ Extract payload correctly (excluding Sync & CRC)
            uint8_t payloadLength = buffer[1];  // ✅ Correct payload length
            uint8_t processedMessage[payloadLength];

            for (uint8_t i = 0; i < payloadLength; i++) {
                processedMessage[i] = buffer[i + 2];  // ✅ Skip Sync & Length Bytes
            }

            // ✅ Detect and process ANT+ or Custom Serial Messages
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

    // ✅ XOR all bytes in the payload (starting from `payload[0]`)
    for (uint8_t i = 0; i < length; i++) {  // ✅ Start at 0, stop at last payload byte
        calculatedCRC ^= payload[i];  // XOR each byte
    }

    //LOGF("[DEBUG] Calculated CRC: 0x%02X, Expected CRC: 0x%02X", calculatedCRC, expectedCRC);
    //LOG("");
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
