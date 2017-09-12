#include <string.h>
#include "DS1820.h"

uint8_t DS1820::find() {
  uint8_t addr[8];
  uint8_t result = 0;

  memset(_addr, 0, sizeof(_addr));
  while (_ow.search(addr)) {
    if ((OneWire::crc8(addr, 7) == addr[7]) && ((addr[0] == 0x10) || (addr[0] == 0x22) || (addr[0] == 0x28))) {
      memcpy(_addr[result], addr, sizeof(addr));
      if (++result >= MAX_DS1820)
        break;
    }
  }
  _ow.reset_search();

  return result;
}

void DS1820::update(uint8_t id) {
  if (id == 0xFF) {
    _ow.reset();
    for (id = 0; id < MAX_DS1820; ++id) {
      if (! isValid(id))
        break;
      _ow.select(_addr[id]);
      _ow.write(0x44, 1); // start conversion, with parasite power on at the end
    }
  } else {
    if (isValid(id)) {
      _ow.reset();
      _ow.select(_addr[id]);
      _ow.write(0x44, 1); // start conversion, with parasite power on at the end
    }
  }
}

float DS1820::readTemperature(uint8_t id) {
  if (! isValid(id))
    return NAN;

  uint8_t data[9];

  _ow.reset();
  _ow.select(_addr[id]);
  _ow.write(0xBE); // Read Scratchpad

  for (uint8_t i = 0; i < 9; ++i) { // we need 9 bytes
    data[i] = _ow.read();
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (_addr[id][0] == 0x10) { // old DS1820
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    uint8_t cfg = data[4] & 0x60;
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    // default is 12 bit resolution, 750 ms conversion time
  }

  return ((float)raw / 16.0);
}
