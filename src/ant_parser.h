#ifndef ANT_PARSER_H
#define ANT_PARSER_H

#include <Arduino.h>

// ✅ Struct to store persistent FTMS data
struct FTMSDataStorage {
    uint16_t elapsed_time;
    uint32_t distance; // Updated to 24-bit equivalent
    float speed;
    uint8_t heart_rate;
    uint16_t power;
    uint8_t virtual_speed; 
    uint16_t accumulated_power;
    uint16_t instantaneous_power;
    uint8_t cadence;
    float cycle_length;
    float incline;
    float resistance;
    uint8_t fe_state;
    uint16_t manufacturerID;
    uint32_t serialNumber;
    uint16_t softwareVersion;
    uint16_t modelNumber;
    uint8_t hardware_revision; 
    uint8_t trainer_status;
    uint16_t maxResistance;
    bool hasData;

    FTMSDataStorage() : elapsed_time(0), distance(0), speed(0), heart_rate(0), power(0),
                        accumulated_power(0), instantaneous_power(0), cadence(0), cycle_length(0),
                        incline(0), resistance(0), fe_state(0), manufacturerID(0), serialNumber(0),
                        softwareVersion(0), modelNumber(0), virtual_speed(0), hardware_revision(0), trainer_status(0), maxResistance(0), hasData(false) {}
};
class ANTParser {
    public:
        ANTParser();
        void processANTMessage(uint8_t *data, uint8_t length);
        FTMSDataStorage getFTMSData();  // ✅ Return collected FTMS data
        bool hasNewData();
        void readSerial();

    private:
        FTMSDataStorage ftmsData;  // ✅ Store collected data
        bool newData;
        bool validateCRC(uint8_t *payload, uint8_t length, uint8_t crc);
        void processSerialCommand(uint8_t *data, uint8_t length);

        // ✅ Parsing functions now update only specific fields
        void parseGeneralFeData(const uint8_t* data);
        void parseTrainerData(const uint8_t* data);
        void parseTrainerStatus(const uint8_t* data);
        void parseFECapabilities(const uint8_t *data);
        void parseManufacturerID(const uint8_t *data);
        void parseProductInfo(const uint8_t* data);
};

#endif  // ANT_PARSER_H
