#include <avr/wdt.h>
#include <HardwareSerial.h>
#include "Misc.h"

void halt() {
  Serial.println(F("System halted"));
  Serial.flush();

  while (true);
}

void softReset() {
  Serial.println(F("Rebooting..."));
  Serial.flush();

  wdt_enable(WDTO_1S);
  while (true);
}

uint16_t getFreeMem() {
  extern int __heap_start, *__brkval;
  int v;

  return ((uint16_t)&v - (__brkval ? (uint16_t)__brkval : (uint16_t)&__heap_start));
}
