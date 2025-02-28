#ifndef BLE_FTMS_H
#define BLE_FTMS_H

#include <Arduino.h>
#include <NimBLEDevice.h>

struct FTMSData {
    uint16_t power;
    uint16_t speed;
    uint8_t cadence;
    uint16_t distance;
    uint8_t resistance;
    uint16_t elapsedTime;
};

class BLEFTMS {
public:
    BLEFTMS();
    void begin();
    void sendIndoorBikeData(const FTMSData &ftmsData);
    void sendFitnessMachineStatus(uint8_t event, uint8_t parameter);
    void sendTrainingStatus(uint8_t status, uint8_t additionalInfo);
    void prepareFTMSData(uint8_t* data, const FTMSData& ftmsData);
    String getDeviceMAC();
    void setConnectCallback(void (*callback)());
    void setDisconnectCallback(void (*callback)());

    
private:
    void setupFTMS();  // ✅ Ensure it's declared in the class

    NimBLECharacteristic *indoorBikeChar;
    NimBLECharacteristic *fitnessMachineFeatureChar;  // ✅ New characteristic
    NimBLECharacteristic *fitnessMachineStatusChar;  // ✅ New characteristic
    NimBLECharacteristic *trainingStatusChar;  // ✅ New characteristic


    
};

#endif  // BLE_FTMS_H
