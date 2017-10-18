#ifdef ESP8266
#include <pgmspace.h>
#else
#include <avr/pgmspace.h>
#endif
#include <string.h>
#include "Date.h"

static const int8_t daysInMonth[] PROGMEM = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static const char weekdayNames[] PROGMEM = { 'M', 'o', 'n', 'T', 'u', 'e', 'W', 'e', 'd', 'T', 'h', 'u', 'F', 'r', 'i', 'S', 'a', 't', 'S', 'u', 'n' };
static const char monthNames[] PROGMEM = { 'J', 'a', 'n', 'F', 'e', 'b', 'M', 'a', 'r', 'A', 'p', 'r', 'M', 'a', 'y', 'J', 'u', 'n', 'J', 'u', 'l', 'A', 'u', 'g', 'S', 'e', 'p', 'O', 'c', 't', 'N', 'o', 'v', 'D', 'e', 'c' };

const uint16_t EPOCH_TIME_2000 = 10957; // Days from 01.01.1970 to 01.01.2000
const uint16_t EPOCH_TIME_2017 = 17167; // Days from 01.01.1970 to 01.01.2017

const char dateSeparator = '.';
const char timeSeparator = ':';

bool isLeapYear(int16_t year) {
  return (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0));
}

int8_t lastDayOfMonth(int8_t month, int16_t year) {
  int8_t result = pgm_read_byte(daysInMonth + month - 1);
  if ((month == 2) && isLeapYear(year))
    result++;

  return result;
}

void parseUnixTime(uint32_t unixtime, int8_t& hour, int8_t& minute, int8_t& second, uint8_t& weekday, int8_t& day, int8_t& month, int16_t& year) {
  second = unixtime % 60;
  unixtime /= 60;
  minute = unixtime % 60;
  unixtime /= 60;
  hour = unixtime % 24;
  uint16_t days = unixtime / 24;
  weekday = (days + 3) % 7; // 1 Jan 1970 is Thursday
  bool leap;
  if (days >= EPOCH_TIME_2017) {
    year = 2017;
    days -= EPOCH_TIME_2017;
  } else if (days >= EPOCH_TIME_2000) {
    year = 2000;
    days -= EPOCH_TIME_2000;
  } else
    year = 1970;
  for (; ; year++) {
    leap = isLeapYear(year);
    if (days < 365 + leap)
      break;
    days -= 365 + leap;
  }
  for (month = 1; ; month++) {
    uint8_t daysPerMonth = pgm_read_byte(daysInMonth + month - 1);
    if (leap && (month == 2))
      daysPerMonth++;
    if (days < daysPerMonth)
      break;
    days -= daysPerMonth;
  }
  day = days + 1;
}

uint32_t combineUnixTime(int8_t hour, int8_t minute, int8_t second, int8_t day, int8_t month, int16_t year) {
  uint16_t days = day - 1;
  int16_t y;

  if (year >= 2017) {
    days += EPOCH_TIME_2017;
    y = 2017;
  } else if (year >= 2000) {
    days += EPOCH_TIME_2000;
    y = 2000;
  } else
    y = 1970;
  for (; y < year; y++)
    days += 365 + isLeapYear(y);
  for (y = 1; y < month; y++)
    days += pgm_read_byte(daysInMonth + y - 1);
  if ((month > 2) && isLeapYear(year))
    days++;

  return (((uint32_t)days * 24 + hour) * 60 + minute) * 60 + second;
}

char* timeToStr(char* str, int8_t hour, int8_t minute, int8_t second) {
  uint8_t len = 0;

  if (hour >= 0) {
    str[len++] = hour / 10 + '0';
    str[len++] = hour % 10 + '0';
  }
  if (minute >= 0) {
    str[len++] = timeSeparator;
    str[len++] = minute / 10 + '0';
    str[len++] = minute % 10 + '0';
  }
  str[len++] = timeSeparator;
  str[len++] = second / 10 + '0';
  str[len++] = second % 10 + '0';
  str[len] = '\0';

  return str;
}

char* timeToStr(char* str, uint32_t unixtime) {
  int8_t hh, mm, ss;
  uint8_t wd;
  int8_t d, m;
  int16_t y;

  parseUnixTime(unixtime, hh, mm, ss, wd, d, m, y);

  return timeToStr(str, hh, mm, ss);
}

char* dateToStr(char* str, int8_t day, int8_t month, int16_t year) {
  uint8_t len = 0;

  str[len++] = day / 10 + '0';
  str[len++] = day % 10 + '0';
  str[len++] = dateSeparator;
  str[len++] = month / 10 + '0';
  str[len++] = month % 10 + '0';
  str[len++] = dateSeparator;
  if (year > 100) {
    str[len++] = year / 1000 + '0';
    str[len++] = (year / 100) % 10 + '0';
  }
  str[len++] = (year / 10) % 10 + '0';
  str[len++] = year % 10 + '0';
  str[len] = '\0';

  return str;
}

char* dateToStr(char* str, uint32_t unixtime) {
  int8_t hh, mm, ss;
  uint8_t wd;
  int8_t d, m;
  int16_t y;

  parseUnixTime(unixtime, hh, mm, ss, wd, d, m, y);

  return dateToStr(str, d, m, y);
}

char* timeDateToStr(char* str, int8_t hour, int8_t minute, int8_t second, int8_t day, int8_t month, int16_t year) {
  timeToStr(str, hour, minute, second);
  uint16_t len = strlen(str);
  str[len++] = ' ';
  dateToStr(&str[len], day, month, year);

  return str;
}

char* timeDateToStr(char* str, uint32_t unixtime) {
  int8_t hh, mm, ss;
  uint8_t wd;
  int8_t d, m;
  int16_t y;

  parseUnixTime(unixtime, hh, mm, ss, wd, d, m, y);

  return timeDateToStr(str, hh, mm, ss, d, m, y);
}

char* dateTimeToStr(char* str, int8_t day, int8_t month, int16_t year, int8_t hour, int8_t minute, int8_t second) {
  dateToStr(str, day, month, year);
  uint16_t len = strlen(str);
  str[len++] = ' ';
  timeToStr(&str[len], hour, minute, second);

  return str;
}

char* dateTimeToStr(char* str, uint32_t unixtime) {
  int8_t hh, mm, ss;
  uint8_t wd;
  int8_t d, m;
  int16_t y;

  parseUnixTime(unixtime, hh, mm, ss, wd, d, m, y);

  return dateTimeToStr(str, d, m, y, hh, mm, ss);
}

char* weekdayName(char* str, uint8_t weekday) {
  if (weekday < 7) {
    strncpy_P(str, &weekdayNames[weekday * 3], 3);
    str[3] = '\0';
  } else
    *str = '\0';

  return str;
}

char* monthName(char* str, int8_t month) {
  if ((month >= 1) && (month <= 12)) {
    strncpy_P(str, &monthNames[(month - 1) * 3], 3);
    str[3] = '\0';
  } else
    *str = '\0';

  return str;
}
