#include <Wire.h>
#include "AT24C32.h"

const uint8_t AT24C32_ADDRESS = 0x57;
const uint8_t EEPROM_PAGE_SIZE = 32;
const uint8_t EEPROM_WORK_SIZE = EEPROM_PAGE_SIZE / 2;
const uint32_t EEPROM_WRITE_TIMEOUT = 10;

void AT24C32::init(bool fast) {
  Wire.begin();
  if (fast)
    Wire.setClock(400000);
}

#ifdef ESP8266
void AT24C32::init(int8_t pinSDA, int8_t pinSCL, bool fast) {
  Wire.begin(pinSDA, pinSCL);
  if (fast)
    Wire.setClock(400000);
}
#endif

bool AT24C32::begin() {
  Wire.beginTransmission(AT24C32_ADDRESS);
  return (Wire.endTransmission() == 0);
}

uint8_t AT24C32::read(uint16_t index) {
  Wire.beginTransmission(AT24C32_ADDRESS);
  Wire.write((index >> 8) & 0x0F);
  Wire.write(index & 0xFF);
  if (Wire.endTransmission() == 0) {
    Wire.requestFrom(AT24C32_ADDRESS, (uint8_t)1);
    return Wire.read();
  }

  return 0;
}

void AT24C32::read(uint16_t index, uint8_t* buf, uint16_t len) {
  Wire.beginTransmission(AT24C32_ADDRESS);
  Wire.write((index >> 8) & 0x0F);
  Wire.write(index & 0xFF);
  if (Wire.endTransmission() == 0) {
    while (len > 0) {
      uint8_t l;

      l = EEPROM_WORK_SIZE;
      if (l > len)
        l = len;
      len -= l;
      Wire.requestFrom(AT24C32_ADDRESS, l);
      while (l--)
        *buf++ = Wire.read();
    }
  }
}

void AT24C32::write(uint16_t index, uint8_t data) {
  Wire.beginTransmission(AT24C32_ADDRESS);
  Wire.write((index >> 8) & 0x0F);
  Wire.write(index & 0xFF);
  Wire.write(data);
  Wire.endTransmission();
  delay(EEPROM_WRITE_TIMEOUT);
}

void AT24C32::write(uint16_t index, const uint8_t* buf, uint16_t len) {
  index &= 0x0FFF;
  while (len > 0) {
    uint8_t l;

    l = EEPROM_WORK_SIZE - (index % EEPROM_WORK_SIZE);
    if (l > len)
      l = len;
    len -= l;
    Wire.beginTransmission(AT24C32_ADDRESS);
    Wire.write(index >> 8);
    Wire.write(index & 0xFF);
    while (l--) {
      Wire.write(*buf++);
      ++index;
    }
    if (Wire.endTransmission() != 0)
      break;
    while (! Wire.requestFrom(AT24C32_ADDRESS, (uint8_t)1)); // Polling EEPROM ready (write complete)
  }
}

AT24C32 at24c32;
