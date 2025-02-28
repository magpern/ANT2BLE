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

    // Read stored BLE name, fallback to default if not set
    String bleName = preferences.getString("ble_name", "ESP32-S3 FTMS");

    LOGF("[DEBUG] BLE Device Name: %s", bleName.c_str());

    // ✅ Ensure BLE is initialized BEFORE advertising
    NimBLEDevice::init(bleName.c_str());

    // ✅ Create BLE services
    setupFTMS();

    // ✅ Configure BLE Advertising
    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();

    // ✅ Create advertisement data
    NimBLEAdvertisementData advertisementData;
    advertisementData.setFlags(0x06); // LE General Discoverable, BR/EDR Not Supported
    advertisementData.setAppearance(0x0484); // Cycling Power Sensor

    // ✅ Explicitly add Complete List of 16-bit Service UUIDs (FTMS + Cycling Power)
    std::vector<uint8_t> serviceUUIDs = { 0x05, 0x03, 0x26, 0x18, 0x18, 0x18 };
    advertisementData.addData(serviceUUIDs);

    // ✅ Apply advertisement data
    adv->setAdvertisementData(advertisementData);

    // ✅ Add Scan Response Data (For the Device Name)
    NimBLEAdvertisementData scanResponseData;
    scanResponseData.setName(bleName.c_str());  // ✅ Force Name into Scan Response

    // ✅ Manufacturer Data (Placeholder for Reserved ID <0x2502>)
    uint8_t manufacturerData[] = {0x21, 0x48}; 
    scanResponseData.setManufacturerData(manufacturerData, sizeof(manufacturerData));

    // ✅ Apply Scan Response Data
    adv->setScanResponseData(scanResponseData);

    // ✅ Start advertising
    adv->start(0);  // ✅ Start advertising indefinitely
    LOG("[DEBUG] BLE Advertising Started...");
}

void BLEFTMS::setupFTMS() {
    NimBLEServer *server = NimBLEDevice::createServer();

    // ✅ Register BLE connection & disconnection event handlers
    class MyServerCallbacks : public NimBLEServerCallbacks {
        void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
            LOG("[INFO] BLE Device Connected!");
            if (onConnectCallback) {
                onConnectCallback();  // ✅ Notify `main.cpp` about connection
            }
        }

        void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
            LOGF("[INFO] BLE Device Disconnected! Reason: %d", reason);
            if (onDisconnectCallback) {
                onDisconnectCallback();  // ✅ Notify `main.cpp` about disconnection
            }

            // ✅ Restart advertising when a device disconnects
            LOG("[INFO] Restarting BLE Advertising...");
            NimBLEDevice::getAdvertising()->start(0);
        }
    };

    server->setCallbacks(new MyServerCallbacks());  // ✅ Assign Callbacks

    NimBLEService *ftmsService = server->createService(NimBLEUUID((uint16_t) 0x1826)); // FTMS Service UUID

    // ✅ Indoor Bike Data (Notify)
    indoorBikeChar = ftmsService->createCharacteristic(
        NimBLEUUID((uint16_t) 0x2AD2), // Indoor Bike Data UUID
        NIMBLE_PROPERTY::NOTIFY
    );

    // ✅ Fitness Machine Feature (Read)
    fitnessMachineFeatureChar = ftmsService->createCharacteristic(
        NimBLEUUID((uint16_t) 0x2ACC), // Fitness Machine Feature UUID
        NIMBLE_PROPERTY::READ
    );

    // ✅ Fitness Machine Status (Notify)
    fitnessMachineStatusChar = ftmsService->createCharacteristic(
        NimBLEUUID((uint16_t) 0x2ADA), // Fitness Machine Status UUID
        NIMBLE_PROPERTY::NOTIFY
    );

    // ✅ Training Status (Notify)
    trainingStatusChar = ftmsService->createCharacteristic(
        NimBLEUUID((uint16_t) 0x2AD3), // Training Status UUID
        NIMBLE_PROPERTY::NOTIFY
    );

    ftmsService->start();
}

// 🔹 Prepare FTMS Indoor Bike Data for BLE notification
void BLEFTMS::prepareFTMSData(uint8_t* data, const FTMSData& ftmsData) {
    // ✅ 1. Set Flags: Speed, Cadence, Distance, Resistance, Power, Elapsed Time
    data[0] = 0x75;  // ✅ Ensure Speed, Cadence, Distance (24-bit), Resistance, Power, Time are present
    data[1] = 0x08;  // ✅ Ensure correct feature presence

    // ✅ 2. Convert Speed from km/h → m/s (0.01 m/s resolution)
    uint16_t speed_mps = static_cast<uint16_t>((ftmsData.speed * 1000) / 36);  // Convert km/h → 0.01 m/s
    data[2] = speed_mps & 0xFF;
    data[3] = (speed_mps >> 8) & 0xFF;

    // ✅ 3. Instantaneous Cadence (Little-Endian)
    uint16_t cadence = static_cast<uint16_t>(ftmsData.cadence);
    data[4] = cadence & 0xFF;
    data[5] = (cadence >> 8) & 0xFF;

    // ✅ 4. Total Distance (24-bit Little-Endian)
    uint32_t distance = ftmsData.distance;
    data[6] = distance & 0xFF;
    data[7] = (distance >> 8) & 0xFF;
    data[8] = (distance >> 16) & 0xFF;

    // ✅ 5. Resistance Level
    uint8_t resistance = static_cast<uint8_t>(ftmsData.resistance);
    data[9] = resistance;

    // ✅ 6. Instantaneous Power (Little-Endian)
    int16_t power = static_cast<int16_t>(ftmsData.instantaneousPower);
    data[10] = power & 0xFF;
    data[11] = (power >> 8) & 0xFF;

    // ✅ 7. Elapsed Time (Little-Endian)
    uint16_t elapsedTime = static_cast<uint16_t>(ftmsData.elapsedTime);
    data[12] = elapsedTime & 0xFF;
    data[13] = (elapsedTime >> 8) & 0xFF;
}


// 🔹 Send FTMS BLE Notification
void BLEFTMS::sendIndoorBikeData(const FTMSData& ftmsData) {
    uint8_t data[14] = {0};  // ✅ Ensure Correct Buffer Size
    prepareFTMSData(data, ftmsData);

    LOGF("[DEBUG] Sending BLE FTMS: Power=%dW, Speed=%d km/h (0x%02X%02X), Cadence=%d rpm (0x%02X%02X), Distance=%d m (0x%02X%02X%02X), Resistance=%d, Elapsed Time=%d s (0x%02X%02X)",
        ftmsData.power, ftmsData.speed, data[3], data[2], 
        ftmsData.cadence, data[5], data[4], 
        ftmsData.distance, data[8], data[7], data[6], 
        ftmsData.resistance, ftmsData.elapsedTime, data[13], data[12]);

    if (indoorBikeChar) {
        indoorBikeChar->notify((const uint8_t*)data, sizeof(data));
    } else {
        LOG("[ERROR] BLE Indoor Bike Characteristic is NULL!");
    }

    // ✅ Notify Fitness Machine Status (if applicable)
    sendFitnessMachineStatus(ftmsData);

    // ✅ Send Training Status (0x2AD3)
    sendTrainingStatus(ftmsData);
}

void BLEFTMS::sendFitnessMachineStatus(const FTMSData& ftmsData) {
    if (!fitnessMachineStatusChar) {
        LOG("[ERROR] Fitness Machine Status characteristic is NULL!");
        return;
    }

    uint8_t eventID = 0x02;  // Default: Stopped
    uint8_t eventParam = 0x00;

    // ✅ Map ANT+ `fe_state` to FTMS Event IDs
    switch (ftmsData.feState) {
        case 1:  // ASLEEP
            eventID = 0x02;  // Stopped
            break;
        case 2:  // READY
            eventID = 0x03;  // Idle
            break;
        case 3:  // IN USE
            eventID = 0x01;  // Started
            break;
        case 4:  // FINISHED (Paused)
            eventID = 0x04;  // Paused
            break;
        default:
            eventID = 0x02;  // Default to Stopped
    }

    uint8_t statusData[2] = { eventID, eventParam };

    LOGF("[DEBUG] Sending FTMS Status: Event=0x%02X, Parameter=0x%02X", eventID, eventParam);
    fitnessMachineStatusChar->notify(reinterpret_cast<const uint8_t*>(statusData), sizeof(statusData));
}


void BLEFTMS::sendTrainingStatus(const FTMSData& ftmsData) {
    if (!trainingStatusChar) {
        LOG("[ERROR] Training Status characteristic is NULL!");
        return;
    }

    uint8_t statusID = 0x01;  // Default: Not in Training
    uint8_t additionalInfo = 0x00;

    // ✅ Map ANT+ `trainer_status` to FTMS Training Status IDs
    switch (ftmsData.trainerStatus) {
        case 1:  // NO ACTIVITY
            statusID = 0x01;  // Not in Training
            break;
        case 2:  // WARMING UP
            statusID = 0x06;  // Warming Up
            break;
        case 3:  // ACTIVE TRAINING
            statusID = 0x02;  // Active Training
            break;
        case 4:  // PAUSED / RESTING
            statusID = 0x05;  // Paused
            break;
        case 5:  // COOLING DOWN
            statusID = 0x07;  // Cooling Down
            break;
        default:
            statusID = 0x01;  // Default to Not in Training
    }

    uint8_t trainingData[2] = { statusID, additionalInfo };

    LOGF("[DEBUG] Sending Training Status: Status=0x%02X, Info=0x%02X", statusID, additionalInfo);
    trainingStatusChar->notify(reinterpret_cast<const uint8_t*>(trainingData), sizeof(trainingData));
}


String BLEFTMS::getDeviceMAC() {
    return NimBLEDevice::getAddress().toString().c_str();
}
