#ifndef LD2450_H
#define LD2450_H

#include <Arduino.h>

struct LD2450_Target {
    int16_t x;        // mm
    int16_t y;        // mm
    int16_t speed;    // cm/s
    uint16_t distRes; // mm
};

class LD2450 {
public:
    LD2450(HardwareSerial* serial);
    void processByte(uint8_t c);
    void getTargets(LD2450_Target targets[3]);
    bool setSingleTargetMode();
    bool setMultiTargetMode();
    bool restart();
    bool factoryReset();

private:
    HardwareSerial* _serial;
    uint8_t _buffer[512];
    int _bufferLen;
    LD2450_Target _latestTargets[3];
    
    volatile bool _ackReceived;
    volatile uint16_t _lastAckCmd;
    volatile uint8_t _ackStatus;
    
    int16_t _decodeValue(uint8_t low, uint8_t high);
    void _parseRadarData(uint8_t *buf, int len);
    void _parseAckFrame(uint8_t *buf, int len);
    bool _sendCommand(uint16_t cmdWord, uint8_t* cmdValue, uint8_t cmdValueLen);
    bool _enableConfig();
    bool _endConfig();
};

#endif