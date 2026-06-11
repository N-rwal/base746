#include "LD2450.h"

LD2450::LD2450(HardwareSerial* serial) {
    _serial = serial;
    _bufferLen = 0;
    memset(_latestTargets, 0, sizeof(_latestTargets));
    _ackReceived = false;
    _lastAckCmd = 0;
    _ackStatus = 0;
}

int16_t LD2450::_decodeValue(uint8_t low, uint8_t high) {
    uint16_t raw = low | (high << 8);
    if (high & 0x80) {
        return (int16_t)(raw & 0x7FFF);
    } else {
        return -(int16_t)(raw & 0x7FFF);
    }
}

void LD2450::_parseRadarData(uint8_t *buf, int len) {
    for (int i = 0; i <= len - 30; i++) {
        if (buf[i] == 0xAA && buf[i+1] == 0xFF && buf[i+2] == 0x03 && buf[i+3] == 0x00) {
            LD2450_Target targets[3];
            
            for (int t = 0; t < 3; t++) {
                int offset = i + 4 + t * 8;
                uint8_t xl = buf[offset];
                uint8_t xh = buf[offset+1];
                uint8_t yl = buf[offset+2];
                uint8_t yh = buf[offset+3];
                uint8_t sl = buf[offset+4];
                uint8_t sh = buf[offset+5];
                uint8_t rl = buf[offset+6];
                uint8_t rh = buf[offset+7];
                
                targets[t].x = _decodeValue(xl, xh);
                targets[t].y = _decodeValue(yl, yh);
                targets[t].speed = _decodeValue(sl, sh);
                targets[t].distRes = (rl | (rh << 8));
            }
            
            // Copy to latest targets buffer
            memcpy(_latestTargets, targets, sizeof(targets));
            
            i += 29;
        }
    }
}

void LD2450::_parseAckFrame(uint8_t *buf, int len) {
    if (len < 10) return;
    if (buf[0] != 0xFD || buf[1] != 0xFC || buf[2] != 0xFB || buf[3] != 0xFA) return;
    
    uint16_t dataLen = buf[4] | (buf[5] << 8);
    _lastAckCmd = buf[6] | (buf[7] << 8);
    _ackStatus = buf[8];
    _ackReceived = true;
}

bool LD2450::_sendCommand(uint16_t cmdWord, uint8_t* cmdValue, uint8_t cmdValueLen) {
    if (!_serial) return false;
    
    uint8_t frame[32];
    int idx = 0;
    
    frame[idx++] = 0xFD; frame[idx++] = 0xFC;
    frame[idx++] = 0xFB; frame[idx++] = 0xFA;
    
    uint16_t dataLen = 2 + cmdValueLen;
    frame[idx++] = dataLen & 0xFF;
    frame[idx++] = (dataLen >> 8) & 0xFF;
    
    frame[idx++] = cmdWord & 0xFF;
    frame[idx++] = (cmdWord >> 8) & 0xFF;
    
    if (cmdValue && cmdValueLen) {
        memcpy(&frame[idx], cmdValue, cmdValueLen);
        idx += cmdValueLen;
    }
    
    frame[idx++] = 0x04; frame[idx++] = 0x03;
    frame[idx++] = 0x02; frame[idx++] = 0x01;
    
    _ackReceived = false;
    _serial->write(frame, idx);
    _serial->flush();
    
    uint32_t start = millis();
    while (millis() - start < 200) {
        if (_ackReceived && _lastAckCmd == cmdWord) {
            return _ackStatus == 0;
        }
        delay(1);
    }
    return false;
}

bool LD2450::_enableConfig() {
    uint16_t val = 0x0001;
    return _sendCommand(0x00FF, (uint8_t*)&val, 2);
}

bool LD2450::_endConfig() {
    return _sendCommand(0x00FE, nullptr, 0);
}

void LD2450::processByte(uint8_t c) {
    if (_bufferLen < (int)sizeof(_buffer)) {
        _buffer[_bufferLen++] = c;
    } else {
        _bufferLen = 0;
    }
    
    if (_bufferLen >= 8) {
        _parseRadarData(_buffer, _bufferLen);
    }
}

void LD2450::getTargets(LD2450_Target targets[3]) {
    memcpy(targets, _latestTargets, sizeof(_latestTargets));
}

bool LD2450::setSingleTargetMode() {
    bool ok = false;
    if (_enableConfig()) {
        delay(10);
        ok = _sendCommand(0x0080, nullptr, 0);
        delay(10);
        _endConfig();
    }
    return ok;
}

bool LD2450::setMultiTargetMode() {
    bool ok = false;
    if (_enableConfig()) {
        delay(10);
        ok = _sendCommand(0x0090, nullptr, 0);
        delay(10);
        _endConfig();
    }
    return ok;
}

bool LD2450::restart() {
    return _sendCommand(0x00A3, nullptr, 0);
}

bool LD2450::factoryReset() {
    return _sendCommand(0x00A2, nullptr, 0);
}