#ifndef BLE_FTMS_H
#define BLE_FTMS_H

#include <Arduino.h>
#include <NimBLEDevice.h>

class BLEFTMS {
public:
    BLEFTMS();
    void begin();
    void sendIndoorBikeData(uint16_t power, uint16_t speed, uint8_t cadence);
    void sendFitnessMachineStatus(uint8_t event, uint8_t parameter);
    void sendTrainingStatus(uint8_t status, uint8_t additionalInfo);
    void prepareFTMSData(uint8_t *data, uint16_t power, uint16_t speed, uint8_t cadence);
    String getDeviceMAC();

private:
    void setupFTMS();  // ✅ Ensure it's declared in the class

    NimBLECharacteristic *indoorBikeChar;
    NimBLECharacteristic *fitnessMachineFeatureChar;  // ✅ New characteristic
    NimBLECharacteristic *fitnessMachineStatusChar;  // ✅ New characteristic
    NimBLECharacteristic *trainingStatusChar;  // ✅ New characteristic


};

#endif  // BLE_FTMS_H
