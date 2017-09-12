#ifndef __AT24C32_H
#define __AT24C32_H

#include <Arduino.h>

class AT24C32 {
public:
  static void init(bool fast = false);
#ifdef ESP8266
  static void init(int8_t pinSDA, int8_t pinSCL, bool fast = false);
#endif
  static bool begin();
  static uint8_t read(uint16_t index);
  static void read(uint16_t index, uint8_t* buf, uint16_t len);
  static void write(uint16_t index, uint8_t data);
  static void write(uint16_t index, const uint8_t* buf, uint16_t len);
  static void update(uint16_t index, uint8_t data) {
    if (read(index) != data)
      write(index, data);
  }
  template<typename T> static T& get(uint16_t index, T& t) {
    read(index, (uint8_t*)&t, sizeof(T));
    return t;
  }
  template<typename T> static const T& put(uint16_t index, const T& t) {
    write(index, (const uint8_t*)&t, sizeof(T));
    return t;
  }
};

extern AT24C32 at24c32;

#endif
