#ifndef BLE_FTMS_H
#define BLE_FTMS_H

#include <Arduino.h>
#include <NimBLEDevice.h>


struct FTMSData {
    uint16_t elapsedTime;
    uint32_t distance;
    uint16_t speed;
    uint8_t heartRate;
    uint16_t power;
    uint8_t virtualSpeed;
    uint16_t accumulatedPower;
    uint16_t instantaneousPower;
    uint8_t cadence;
    float cycleLength;
    float incline;
    float resistance;
    uint8_t feState;
    uint16_t manufacturerID;
    uint32_t serialNumber;
    uint16_t softwareVersion;
    uint16_t modelNumber;
    uint8_t hardwareRevision;
    uint8_t trainerStatus;
    uint16_t maxResistance;
};

class BLEFTMS {
public:
    BLEFTMS();
    void begin();
    void sendIndoorBikeData(const FTMSData &ftmsData);
    void sendFitnessMachineStatus(const FTMSData &ftmsData);
    void sendTrainingStatus(const FTMSData &ftmsData);
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
