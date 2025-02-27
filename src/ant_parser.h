#ifndef ANT_PARSER_H
#define ANT_PARSER_H

#include <Arduino.h>

struct BLEData {
    uint16_t power;
    uint16_t speed;
    uint8_t cadence;
};

struct AntFeData {
    uint8_t page;
    uint16_t elapsed_time;
    uint16_t distance;
    uint16_t speed;
    uint16_t power;
    uint8_t cadence;
    uint8_t resistance;
};

class ANTParser {
    public:
        ANTParser();
        void processANTMessage(uint8_t *data, uint8_t length);
        BLEData getBLEData();
        bool hasNewData();
        void readSerial();
        
    private:
        BLEData bleData;
        bool newData;
        bool validateCRC(uint8_t *payload, uint8_t length, uint8_t crc);
        void processSerialCommand(uint8_t *data, uint8_t length);

        // ✅ Only Declare Here, Define in `ant_parser.cpp`
        void parseGeneralFeData(const uint8_t* data, AntFeData& feData);
        void parseTrainerData(const uint8_t* data, AntFeData& feData);
        void parseTrainerStatus(const uint8_t* data, AntFeData& feData);
        void parseManufacturerID(const uint8_t *data);
        void parseProductInfo(const uint8_t *data);
};

#endif  // ANT_PARSER_H
