#include "ble_ftms.h"
#include "logger.h"
#include "global.h"

BLEFTMS::BLEFTMS() {}

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
    adv->start();
    LOG("[DEBUG] BLE Advertising Started...");
}


void BLEFTMS::setupFTMS() {
    NimBLEServer *server = NimBLEDevice::createServer();
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

    // ✅ Set Read Callback for Fitness Machine Feature
    // ✅ Correctly store the value in a uint32_t buffer
    uint8_t ftmsFeatureValue[8] = { 0x02, 0x40, 0x00, 0x00,  // Fitness Machine Features
        0x00, 0x00, 0x00, 0x00 }; // Target Setting Features
    fitnessMachineFeatureChar->setValue((uint8_t*)&ftmsFeatureValue, sizeof(ftmsFeatureValue));

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
    data[0] = 0x74;  // ✅ Flags: Speed, Cadence, Distance, Resistance, Power, Elapsed Time present
    data[1] = 0x08;

    // ✅ Instantaneous Speed (Little-Endian)
    data[2] = ftmsData.speed & 0xFF;
    data[3] = (ftmsData.speed >> 8) & 0xFF;

    // ✅ Instantaneous Cadence (Little-Endian)
    data[4] = ftmsData.cadence & 0xFF;
    data[5] = (ftmsData.cadence >> 8) & 0xFF;

    // ✅ Total Distance (Little-Endian)
    data[6] = ftmsData.distance & 0xFF;
    data[7] = (ftmsData.distance >> 8) & 0xFF;

    // ✅ Resistance Level
    data[8] = ftmsData.resistance;

    // ✅ Instantaneous Power (Little-Endian)
    data[9] = ftmsData.power & 0xFF;
    data[10] = (ftmsData.power >> 8) & 0xFF;

    // ✅ Elapsed Time (Little-Endian)
    data[11] = ftmsData.elapsedTime & 0xFF;
    data[12] = (ftmsData.elapsedTime >> 8) & 0xFF;
}


// 🔹 Send FTMS BLE Notification
void BLEFTMS::sendIndoorBikeData(const FTMSData& ftmsData) {
    uint8_t data[14] = {0};  // ✅ Increased buffer size for full FTMS message
    prepareFTMSData(data, ftmsData);

    LOGF("[DEBUG] Sending BLE FTMS: Power=%dW, Speed=%d km/h, Cadence=%d rpm, Distance=%d m, Resistance=%d, Elapsed Time=%d s",
        ftmsData.power, ftmsData.speed, ftmsData.cadence, ftmsData.distance, ftmsData.resistance, ftmsData.elapsedTime);

    if (indoorBikeChar) {
        indoorBikeChar->notify((const uint8_t*)data, sizeof(data));
    } else {
        LOG("[ERROR] BLE Indoor Bike Characteristic is NULL!");
    }

    // ✅ Notify Fitness Machine Status (if applicable)
    if (ftmsData.power > 0 || ftmsData.speed > 0) {
        sendFitnessMachineStatus(0x01, 0x00);  // Event: Started
        sendTrainingStatus(0x02, 0x00);  // Training Status: Active Training
    } else {
        sendFitnessMachineStatus(0x02, 0x00);  // Event: Stopped
        sendTrainingStatus(0x05, 0x00);  // Training Status: Cooling Down
    }
}



void BLEFTMS::sendFitnessMachineStatus(uint8_t event, uint8_t parameter) {
    if (!fitnessMachineStatusChar) {
        LOG("[ERROR] Fitness Machine Status characteristic is NULL!");
        return;
    }

    uint8_t statusData[2] = { event, parameter };  // FTMS Status format

    LOGF("[DEBUG] Sending FTMS Status: Event=0x%02X, Parameter=0x%02X", event, parameter);
    fitnessMachineStatusChar->notify(reinterpret_cast<const uint8_t*>(statusData), sizeof(statusData));
}

void BLEFTMS::sendTrainingStatus(uint8_t status, uint8_t additionalInfo) {
    if (!trainingStatusChar) {
        LOG("[ERROR] Training Status characteristic is NULL!");
        return;
    }

    uint8_t trainingData[2] = { status, additionalInfo };  // FTMS Training Status format

    LOGF("[DEBUG] Sending Training Status: Status=0x%02X, Info=0x%02X", status, additionalInfo);
    trainingStatusChar->notify(reinterpret_cast<const uint8_t*>(trainingData), sizeof(trainingData));
}


String BLEFTMS::getDeviceMAC() {
    return NimBLEDevice::getAddress().toString().c_str();
}
