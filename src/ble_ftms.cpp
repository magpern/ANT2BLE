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

    // ✅ Add services
    adv->addServiceUUID(NimBLEUUID((uint16_t)0x1826)); // Fitness Machine Service
    adv->addServiceUUID(NimBLEUUID((uint16_t)0x1818)); // Cycling Power Service

    // ✅ Create advertisement data
    NimBLEAdvertisementData advertisementData;
    advertisementData.setFlags(0x06); // LE General Discoverable, BR/EDR Not Supported
    advertisementData.setAppearance(0x0484); // Cycling Power Sensor

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

    indoorBikeChar = ftmsService->createCharacteristic(
        NimBLEUUID((uint16_t) 0x2AD2), // Indoor Bike Data UUID
        NIMBLE_PROPERTY::NOTIFY
    );

    ftmsService->start();
}

// 🔹 Prepare FTMS Indoor Bike Data for BLE notification
void BLEFTMS::prepareFTMSData(uint8_t* data, uint16_t power, uint16_t speed, uint8_t cadence) {
    data[0] = 0x04 | 0x02;  // Flags: Power & Cadence present
    data[1] = power & 0xFF;  // Power LSB
    data[2] = (power >> 8) & 0xFF;  // Power MSB
    data[3] = speed & 0xFF;  // Speed LSB
    data[4] = (speed >> 8) & 0xFF;  // Speed MSB
    data[5] = cadence;  // Cadence
}

// 🔹 Send FTMS BLE Notification
void BLEFTMS::sendIndoorBikeData(uint16_t power, uint16_t speed, uint8_t cadence) {
    uint8_t data[8] = {0};
    prepareFTMSData(data, power, speed, cadence);

    LOGF("[DEBUG] Sending BLE FTMS: Power=%dW, Speed=%d km/h, Cadence=%d rpm", 
        power, speed, cadence);
    
    if (indoorBikeChar) {
        indoorBikeChar->notify((const uint8_t*)data, sizeof(data));
    } else {
        LOG("[ERROR] BLE Indoor Bike Characteristic is NULL!");
    }
}


String BLEFTMS::getDeviceMAC() {
    return NimBLEDevice::getAddress().toString().c_str();
}
