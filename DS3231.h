#ifndef __DS3231_H
#define __DS3231_H

#include <Arduino.h>

class DS3231 {
public:
  static void init(bool fast = false);
#ifdef ESP8266
  static void init(int8_t pinSDA, int8_t pinSCL, bool fast = false);
#endif
  static bool begin();
  static uint32_t get();
  static void set(uint32_t time);
};

extern DS3231 ds3231;

#endif
