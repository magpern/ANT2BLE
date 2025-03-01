#include "ble_ftms.h"
#include "logger.h"
#include "global.h"

BLEFTMS::BLEFTMS() {}
static void (*onConnectCallback)() = nullptr;
static void (*onDisconnectCallback)() = nullptr;

void BLEFTMS::setConnectCallback(void (*callback)()) {
    onConnectCallback = callback;
}

void BLEFTMS::setDisconnectCallback(void (*callback)()) {
    onDisconnectCallback = callback;
}

void BLEFTMS::begin() {
    preferences.begin("ble_ftms", false);  // Open storage
    String bleName = preferences.getString("ble_name", "ESP32-S3 FTMS");

    LOGF("[DEBUG] BLE Device Name: %s", bleName.c_str());
    NimBLEDevice::init(bleName.c_str());

    setupFTMS();  // ✅ Setup BLE services

    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
    NimBLEAdvertisementData advertisementData;
    advertisementData.setFlags(0x06);
    advertisementData.setAppearance(0x0484); // Cycling Power Sensor

    std::vector<uint8_t> serviceUUIDs = { 0x05, 0x03, 0x26, 0x18, 0x18, 0x18 };
    advertisementData.addData(serviceUUIDs);
    adv->setAdvertisementData(advertisementData);

    NimBLEAdvertisementData scanResponseData;
    scanResponseData.setName(bleName.c_str());
    uint8_t manufacturerData[] = {0x21, 0x48};
    scanResponseData.setManufacturerData(manufacturerData, sizeof(manufacturerData));
    adv->setScanResponseData(scanResponseData);
    adv->start(0);

    LOG("[DEBUG] BLE Advertising Started...");
}

void BLEFTMS::setupFTMS() {
    NimBLEServer *server = NimBLEDevice::createServer();

    class MyServerCallbacks : public NimBLEServerCallbacks {
        void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
            LOG("[INFO] BLE Device Connected!");
            if (onConnectCallback) onConnectCallback();
        }

        void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
            LOGF("[INFO] BLE Device Disconnected! Reason: %d", reason);
            if (onDisconnectCallback) onDisconnectCallback();
            LOG("[INFO] Restarting BLE Advertising...");
            NimBLEDevice::getAdvertising()->start(0);
        }
    };

    server->setCallbacks(new MyServerCallbacks());
    NimBLEService *ftmsService = server->createService(NimBLEUUID((uint16_t) 0x1826)); // FTMS Service UUID

    indoorBikeChar = ftmsService->createCharacteristic(
        NimBLEUUID((uint16_t) 0x2AD2), NIMBLE_PROPERTY::NOTIFY);

    fitnessMachineFeatureChar = ftmsService->createCharacteristic(
        NimBLEUUID((uint16_t) 0x2ACC), NIMBLE_PROPERTY::READ);

    setupFTMSFeatures();
    
    fitnessMachineStatusChar = ftmsService->createCharacteristic(
        NimBLEUUID((uint16_t) 0x2ADA), NIMBLE_PROPERTY::NOTIFY);

    trainingStatusChar = ftmsService->createCharacteristic(
        NimBLEUUID((uint16_t) 0x2AD3), NIMBLE_PROPERTY::NOTIFY);

    ftmsService->start();
}

void BLEFTMS::setupFTMSFeatures() {
    uint32_t features = 0x00005086; // Speed, Cadence, Power, Resistance Level
    uint32_t targetSettings = 0x00000000; // No target settings

    // Convert to little-endian byte array (BLE requires LSB first)
    uint8_t featureData[8];
    memcpy(featureData, &features, sizeof(features));
    memcpy(featureData + 4, &targetSettings, sizeof(targetSettings));

    // Set the FTMS Features characteristic value
    fitnessMachineFeatureChar->setValue(featureData, sizeof(featureData));

    // Debug logging
    LOGF("[DEBUG] FTMS Features Set: %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X",
         featureData[0], featureData[1], featureData[2], featureData[3],
         featureData[4], featureData[5], featureData[6], featureData[7]);
}



// 🔹 Prepare FTMS Indoor Bike Data for BLE notification
void BLEFTMS::prepareFTMSData(uint8_t* data, const FTMSDataStorage& ftmsData) {
    // ✅ Ensure buffer is initialized to prevent memory corruption
    memset(data, 0, 15);

    // ✅ 1. Correct Flag Bytes (Bit 0 = 0, Instantaneous Speed is still included)
    data[0] = 0x74;  // Flag Byte 0
    data[1] = 0x08;  // Flag Byte 1

    // ✅ 2. Instantaneous Speed (0.01 km/h resolution, Little-Endian)
    uint16_t speed_kmh = static_cast<uint16_t>(ftmsData.speed * 100);
    data[2] = speed_kmh & 0xFF;
    data[3] = (speed_kmh >> 8) & 0xFF;
    LOGF("[DEBUG] Speed: %.2f km/h -> Encoded: %d (0.01 km/h units)", ftmsData.speed, speed_kmh);

    // ✅ 3. Instantaneous Cadence (Fixed: Multiply by 2)
    uint16_t cadence = static_cast<uint16_t>(ftmsData.cadence * 2);
    data[4] = cadence & 0xFF;
    data[5] = (cadence >> 8) & 0xFF;
    LOGF("[DEBUG] Cadence: %d rpm -> Encoded: %d (0.5 rpm resolution)", ftmsData.cadence, cadence);

    // ✅ 4. Total Distance (uint24, Little-Endian, **3 bytes only**)
    uint32_t distance = ftmsData.distance;
    data[6] = distance & 0xFF;
    data[7] = (distance >> 8) & 0xFF;
    data[8] = (distance >> 16) & 0xFF;  // Only 3 bytes!
    LOGF("[DEBUG] Distance: %d meters (3-byte encoding)", distance);

    // ✅ 5. Resistance Level (Signed 16-bit, Little-Endian)
    int16_t resistance = static_cast<int16_t>(ftmsData.resistance);
    data[9] = resistance & 0xFF;
    data[10] = (resistance >> 8) & 0xFF;
    LOGF("[DEBUG] Resistance Level: %.2f -> Encoded: %d (sint16)", ftmsData.resistance, resistance);

    // ✅ 6. Instantaneous Power (Signed 16-bit, Little-Endian)
    int16_t power = static_cast<int16_t>(ftmsData.instantaneous_power);
    data[11] = power & 0xFF;
    data[12] = (power >> 8) & 0xFF;
    LOGF("[DEBUG] Instantaneous Power: %d W -> Encoded: 0x%02X%02X", power, data[11], data[12]);

    // ✅ 7. Elapsed Time (2-byte uint16, Little-Endian)
    uint16_t elapsedTime = static_cast<uint16_t>(ftmsData.elapsed_time);
    data[13] = elapsedTime & 0xFF;
    data[14] = (elapsedTime >> 8) & 0xFF;
    LOGF("[DEBUG] Elapsed Time: %d sec (2-byte encoding)", elapsedTime);

    // ✅ Debug final buffer output
    LOGF("[DEBUG] BLE FTMS Data (15 bytes): %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X",
         data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], 
         data[9], data[10], data[11], data[12], data[13], data[14]);
}


// 🔹 Send FTMS BLE Notification
void BLEFTMS::sendIndoorBikeData(const FTMSDataStorage& ftmsData) {
    uint8_t data[15] = {0};  

    LOGF("[DEBUG] SSending BLE FTMS: Power=%dW, Speed=%.1f km/h, Cadence=%d rpm, Distance=%u m, Resistance=%.1f, Elapsed Time=%u s",
        ftmsData.power, ftmsData.speed, ftmsData.cadence, static_cast<unsigned int>(ftmsData.distance), ftmsData.resistance, static_cast<unsigned int>(ftmsData.elapsed_time));
    
    prepareFTMSData(data, ftmsData);

    /*
    LOGF("[DEBUG] BLE FTMS Data (15 bytes): %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X",
        data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
        data[8], data[9], data[10], data[11], data[12], data[13], data[14]);
    */  
    if (indoorBikeChar) {
        indoorBikeChar->notify((const uint8_t*)data, sizeof(data));
    } else {
        LOG("[ERROR] BLE Indoor Bike Characteristic is NULL!");
    }

    //updateFitnessMachineStatus(ftmsData);
    //updateTrainingStatus(ftmsData);
}

uint8_t BLEFTMS::determineTrainingStatus(const FTMSDataStorage& ftmsData) {
    switch (ftmsData.trainer_status) {
        case 1: return 0x01;  // No Activity → Not in Training
        case 2: return 0x06;  // Warming Up
        case 3: return 0x02;  // Active Training
        case 4: return 0x05;  // Paused
        case 5: return 0x07;  // Cooling Down
        default: return 0x01; // Default to Not in Training
    }
}

uint8_t BLEFTMS::determineEventID(const FTMSDataStorage& ftmsData) {
    if (ftmsData.speed > 0) return 0x05;  // Target Speed Changed
    if (ftmsData.incline != 0) return 0x06;  // Target Incline Changed
    if (ftmsData.resistance > 0) return 0x07;  // Target Resistance Changed
    if (ftmsData.power > 0) return 0x08;  // Target Power Changed

    switch (ftmsData.fe_state) {
        case 1: return 0x02;  // ASLEEP → Stopped
        case 2: return 0x03;  // READY → Stopped (Safety Key)
        case 3: return 0x04;  // IN USE → Started
        case 4: return 0x05;  // FINISHED → Paused
        default: return 0x02; // Default to Stopped
    }
}


void BLEFTMS::updateTrainingStatus(const FTMSDataStorage& ftmsData) {
    static uint8_t lastTrainingStatus = 0xFF;  // Keep track of last training status sent

    uint8_t statusID = determineTrainingStatus(ftmsData);  // Get the correct status

    if (statusID != lastTrainingStatus) {  // Only send if status changed
        sendTrainingStatus(ftmsData);
        lastTrainingStatus = statusID;
    }
}

void BLEFTMS::updateFitnessMachineStatus(const FTMSDataStorage& ftmsData) {
    static uint8_t lastEventID = 0xFF;  // Keep track of last event sent

    uint8_t eventID = determineEventID(ftmsData);  // Get the correct event based on state

    if (eventID != lastEventID) {  // Only send if state changed
        sendFitnessMachineStatus(ftmsData);
        lastEventID = eventID;
    }
}

// ✅ Send FTMS Fitness Machine Status (0x2ADA)
void BLEFTMS::sendFitnessMachineStatus(const FTMSDataStorage& ftmsData) {
    if (!fitnessMachineStatusChar) {
        LOG("[ERROR] Fitness Machine Status characteristic is NULL!");
        return;
    }

    uint8_t eventID = determineEventID(ftmsData);
    uint8_t statusData[3] = { eventID, 0x00, 0x00 };  // Default: No extra parameters

    // ✅ Only send target changes if the device supports control
    if (deviceSupportsControl()) {
        if (eventID == 0x05) {  // Target Speed Changed
            uint16_t newSpeed = static_cast<uint16_t>(ftmsData.speed * 100);  // Convert to 0.01 km/h
            statusData[1] = newSpeed & 0xFF;
            statusData[2] = (newSpeed >> 8) & 0xFF;
        }
        if (eventID == 0x06) {  // Target Incline Changed
            int16_t newIncline = static_cast<int16_t>(ftmsData.incline * 10);  // Convert to 0.1%
            statusData[1] = newIncline & 0xFF;
            statusData[2] = (newIncline >> 8) & 0xFF;
        }
        if (eventID == 0x07) {  // Target Resistance Changed
            statusData[1] = static_cast<uint8_t>(ftmsData.resistance * 10);  // Convert to 0.1 resolution
        }
        if (eventID == 0x08) {  // Target Power Changed
            int16_t newPower = static_cast<int16_t>(ftmsData.power);
            statusData[1] = newPower & 0xFF;
            statusData[2] = (newPower >> 8) & 0xFF;
        }
    } else {
        LOG("[INFO] Skipping control-related FTMS Status updates (Device is not controllable).");
    }

    LOGF("[DEBUG] Sending FTMS Status: Event=0x%02X, Data=0x%02X 0x%02X",
         eventID, statusData[1], statusData[2]);

    fitnessMachineStatusChar->notify(reinterpret_cast<const uint8_t*>(statusData), sizeof(statusData));
}




// ✅ Send FTMS Training Status (0x2AD3)
void BLEFTMS::sendTrainingStatus(const FTMSDataStorage& ftmsData) {
    if (!trainingStatusChar) {
        LOG("[ERROR] Training Status characteristic is NULL!");
        return;
    }

    uint8_t statusID = 0x01;  // Default: Not in Training

    // ✅ Extract FE State Correctly
    switch (ftmsData.trainer_status) {
        case 1: statusID = 0x01; break; // ASLEEP (OFF) → Not in Training
        case 2: statusID = 0x06; break; // READY → Warming Up
        case 3: statusID = 0x02; break; // IN_USE → Active Training
        case 4: statusID = 0x05; break; // FINISHED (PAUSED) → Paused
        default: statusID = 0x01;  // Default to Not in Training
    }

    uint8_t trainingData[2] = { statusID, 0x00 };  // Second byte reserved

    LOGF("[DEBUG] Sending Training Status: Status=0x%02X, Info=0x%02X", statusID, 0x00);
    trainingStatusChar->notify(reinterpret_cast<const uint8_t*>(trainingData), sizeof(trainingData));
}



String BLEFTMS::getDeviceMAC() {
    return NimBLEDevice::getAddress().toString().c_str();
}

bool BLEFTMS::deviceSupportsControl() {
    // Implement the logic to determine if the device supports control
    return false; // Placeholder implementation
}
