#ifndef ANT_PARSER_H
#define ANT_PARSER_H

#include <Arduino.h>

struct BLEData {
    uint16_t power;
    uint16_t speed;
    uint8_t cadence;
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
        bool validateCRC(uint8_t *payload, uint8_t length, uint8_t crc);  // ✅ Declare function inside class
        void processSerialCommand(uint8_t *data, uint8_t length);
};

#endif  // ANT_PARSER_H
