#include <Wire.h>
#include "DS3231.h"
#include "Date.h"

const uint8_t DS3231_ADDRESS = 0x68; // I2C Slave address

/* DS3231 Registers */
const uint8_t DS3231_SEC_REG = 0x00;
const uint8_t DS3231_MIN_REG = 0x01;
const uint8_t DS3231_HOUR_REG = 0x02;
const uint8_t DS3231_WDAY_REG = 0x03;
const uint8_t DS3231_MDAY_REG = 0x04;
const uint8_t DS3231_MONTH_REG = 0x05;
const uint8_t DS3231_YEAR_REG = 0x06;

const uint8_t DS3231_CONTROL_REG = 0x0E;
const uint8_t DS3231_STATUS_REG = 0x0F;
const uint8_t DS3231_AGING_OFFSET_REG = 0x0F;
const uint8_t DS3231_TMP_UP_REG = 0x11;
const uint8_t DS3231_TMP_LOW_REG = 0x12;

static uint8_t bcd2bin(uint8_t val) { return val - 6 * (val >> 4); }
static uint8_t bin2bcd(uint8_t val) { return val + 6 * (val / 10); }

void DS3231::init(bool fast) {
  Wire.begin();
  if (fast)
    Wire.setClock(400000);
}

#ifdef ESP8266
void DS3231::init(int8_t pinSDA, int8_t pinSCL, bool fast) {
  Wire.begin(pinSDA, pinSCL);
  if (fast)
    Wire.setClock(400000);
}
#endif

bool DS3231::begin() {
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire.write((uint8_t)DS3231_CONTROL_REG);
  Wire.write((uint8_t)0B00011100);
  return (Wire.endTransmission() == 0);
}

uint32_t DS3231::get() {
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire.write((uint8_t)DS3231_SEC_REG);
  Wire.endTransmission();

  Wire.requestFrom(DS3231_ADDRESS, (uint8_t)7);
  int8_t s = bcd2bin(Wire.read());
  int8_t m = bcd2bin(Wire.read());
  int8_t h = bcd2bin(Wire.read() & ~0B11000000); // Ignore 24 Hour bit
  Wire.read(); // ignore dow
  int8_t day = bcd2bin(Wire.read());
  int8_t month = bcd2bin(Wire.read() & ~0B10000000);
  int16_t year = bcd2bin(Wire.read()) + 2000;

  return combineUnixTime(h, m, s, day, month, year);
}

void DS3231::set(uint32_t time) {
  int8_t h, m, s;
  uint8_t dow;
  int8_t day, month;
  int16_t year;

  parseUnixTime(time, h, m, s, dow, day, month, year);

  Wire.beginTransmission(DS3231_ADDRESS);
  Wire.write((uint8_t)DS3231_SEC_REG); // beginning from SEC Register address

  Wire.write((uint8_t)bin2bcd(s));
  Wire.write((uint8_t)bin2bcd(m));
  Wire.write((uint8_t)bin2bcd(h));
  Wire.write((uint8_t)dow);
  Wire.write((uint8_t)bin2bcd(day));
  Wire.write((uint8_t)bin2bcd(month));
  Wire.write((uint8_t)bin2bcd(year - 2000));
  Wire.endTransmission();
}

DS3231 ds3231;
