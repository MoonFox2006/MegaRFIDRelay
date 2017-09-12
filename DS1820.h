#ifndef __DS1820_H
#define __DS1820_H

#include <OneWire.h>

#define MAX_DS1820 4

class DS1820 {
public:
  static const uint32_t MEASURE_TIME = 750;

  DS1820(int8_t pin) : _ow(OneWire(pin)) {}

  uint8_t find();
  bool isValid(uint8_t id) {
    return (_addr[id][0] != 0);
  }
  void update(uint8_t id = 0xFF);
  float readTemperature(uint8_t id);

protected:
  OneWire _ow;
  uint8_t _addr[MAX_DS1820][8];
};

#endif
