#ifndef BLE_FTMS_H
#define BLE_FTMS_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "ant_parser.h"

class BLEFTMS {
public:
    BLEFTMS();
    void begin();
    void sendIndoorBikeData(const FTMSDataStorage &ftmsData);
    uint8_t determineTrainingStatus(const FTMSDataStorage &ftmsData);
    uint8_t determineEventID(const FTMSDataStorage &ftmsData);
    void sendFitnessMachineStatus(const FTMSDataStorage &ftmsData);
    void sendTrainingStatus(const FTMSDataStorage &ftmsData);
    void prepareFTMSData(uint8_t* data, const FTMSDataStorage& ftmsData);
    void updateTrainingStatus(const FTMSDataStorage &ftmsData);
    void updateFitnessMachineStatus(const FTMSDataStorage &ftmsData);
    String getDeviceMAC();
    bool deviceSupportsControl();
    void setConnectCallback(void (*callback)());
    void setDisconnectCallback(void (*callback)());

private:
    void setupFTMS();  // ✅ Ensure it's declared in the class

    void setupFTMSFeatures();

    NimBLECharacteristic *indoorBikeChar;
    NimBLECharacteristic *fitnessMachineFeatureChar;  // ✅ New characteristic
    NimBLECharacteristic *fitnessMachineStatusChar;  // ✅ New characteristic
    NimBLECharacteristic *trainingStatusChar;  // ✅ New characteristic
};

#endif  // BLE_FTMS_H
