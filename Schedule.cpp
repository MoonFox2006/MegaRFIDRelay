#include <stdlib.h>
#include <string.h>
#ifdef ESP8266
#include <pgmspace.h>
#else
#include <avr/pgmspace.h>
#endif
#include "Schedule.h"
#include "Date.h"

static void nextDay(int8_t& d, int8_t& m, int16_t& y) {
  if (++d > lastDayOfMonth(m, y)) {
    d = 1;
    if (++m > 12) {
      m = 1;
      y++;
    }
  }
}

void Schedule::set(Schedule::period_t p, int8_t hh, int8_t mm, int8_t ss, uint8_t wd, int8_t d, int8_t m, int16_t y) {
  if (p != NONE) {
    if (p >= MINUTELY) {
      if ((ss < 0) || (ss > 59))
        p = NONE;
    }
    if (p >= HOURLY) {
      if ((mm < 0) || (mm > 59))
        p = NONE;
    }
    if (p > HOURLY) {
      if ((hh < 0) || (hh > 23))
        p = NONE;
    }
    if (p == WEEKLY) {
      if (wd == 0)
        p = NONE;
    }
    if (p >= MONTHLY) {
      if ((d < 1) || (d > LASTDAYOFMONTH))
        p = NONE;
    }
    if (p >= YEARLY) {
      if ((m < 1) || (m > 12))
        p = NONE;
    }
    if (p == ONCE) {
      if ((y < 2017) || (y > 2099))
        p = NONE;
    }
  }
  _period = p;
  if (p != NONE) {
    _hour = hh;
    _minute = mm;
    _second = ss;
    if (p == WEEKLY) {
      _weekdays = wd;
    } else {
      _day = d;
      _month = m;
      _year = y;
    }
  }
  _nextTime = 0;
}

uint32_t Schedule::next(uint32_t unixtime) {
  uint32_t result = 0;

  if (_period != NONE) {
    int8_t hh, mm, ss;
    uint8_t wd;
    int8_t d, m;
    int16_t y;
    int8_t dd;

    parseUnixTime(unixtime, hh, mm, ss, wd, d, m, y);
    if (_period == MINUTELY) {
      if (ss > _second) {
        if (++mm > 59) {
          mm = 0;
          if (++hh > 23) {
            hh = 0;
            nextDay(d, m, y);
          }
        }
      }
      ss = _second;
    } else if (_period == HOURLY) {
      if ((mm > _minute) || ((mm == _minute) && (ss > _second))) {
        if (++hh > 23) {
          hh = 0;
          nextDay(d, m, y);
        }
      }
      ss = _second;
      mm = _minute;
    } else if (_period == WEEKLY) {
      if (_weekdays == 0)
        return NEVER;

      if (((_weekdays & (1 << wd)) == 0) || ((hh > _hour) || ((hh == _hour) && ((mm > _minute) || ((mm == _minute) && (ss > _second)))))) {
        do {
          nextDay(d, m, y);
          wd = (wd + 1) % 7;
        } while ((_weekdays & (1 << wd)) == 0);
      }
      ss = _second;
      mm = _minute;
      hh = _hour;
    } else if (_period == MONTHLY) {
      if (_day == LASTDAYOFMONTH)
        dd = lastDayOfMonth(m, y);
      else
        dd = _day;
      if ((d > dd) || ((d == dd) && ((hh > _hour) || ((hh == _hour) && ((mm > _minute) || ((mm == _minute) && (ss > _second))))))) {
        if (++m > 12) {
          m = 1;
          y++;
        }
      }
      ss = _second;
      mm = _minute;
      hh = _hour;
      if (_day == LASTDAYOFMONTH)
        d = lastDayOfMonth(m, y);
      else
        d = _day;
    } else if (_period == YEARLY) {
      if (_day == LASTDAYOFMONTH)
        dd = lastDayOfMonth(m, y);
      else
        dd = _day;
      if ((m > _month) || ((m == _month) && ((d > dd) || ((d == dd) && ((hh > _hour) || ((hh == _hour) && ((mm > _minute) || ((mm == _minute) && (ss > _second))))))))) {
        y++;
      }
      ss = _second;
      mm = _minute;
      hh = _hour;
      m = _month;
      if (_day == LASTDAYOFMONTH)
        d = lastDayOfMonth(m, y);
      else
        d = _day;
    } else if (_period == ONCE) {
      if (_day == LASTDAYOFMONTH)
        dd = lastDayOfMonth(m, y);
      else
        dd = _day;
      if ((y > _year) || ((y == _year) && ((m > _month) || ((m == _month) && ((d > dd) || ((d == dd) && ((hh > _hour) || ((hh == _hour) && ((mm > _minute) || ((mm == _minute) && (ss > _second)))))))))))
        return NEVER;

      ss = _second;
      mm = _minute;
      hh = _hour;
      m = _month;
      y = _year;
      if (_day == LASTDAYOFMONTH)
        d = lastDayOfMonth(m, y);
      else
        d = _day;
    }

    result = combineUnixTime(hh, mm, ss, d, m, y);
  }

  return result;
}

bool Schedule::check(uint32_t unixtime) {
  bool result = false;

  if (_period != NONE) {
    if (_nextTime == 0)
      _nextTime = next(unixtime);
    if (_nextTime != NEVER) {
      result = unixtime >= _nextTime;
      if (result)
        _nextTime = next(unixtime + 1);
    }
  }

  return result;
}

const char strUndefined[] PROGMEM = "Undefined";

char* Schedule::toString(char* str) {
  static const char strEvery[] PROGMEM = "Every ";
  static const char strAt[] PROGMEM = " at ";
  static const char strLast[] PROGMEM = "last";

  char timestr[20];

  switch (_period) {
    case NONE:
      strcpy_P(str, strUndefined);
      break;
    case MINUTELY:
      strcpy_P(str, strEvery);
      strcat_P(str, PSTR("minute"));
      strcat_P(str, strAt);
      strcat(str, timeToStr(timestr, -1, -1, _second));
      break;
    case HOURLY:
      strcpy_P(str, strEvery);
      strcat_P(str, PSTR("hour"));
      strcat_P(str, strAt);
      strcat(str, timeToStr(timestr, -1, _minute, _second));
      break;
    case WEEKLY:
      strcpy_P(str, strEvery);
      for (uint8_t i = 0; i < 7; ++i) {
        if (_weekdays & (1 << i)) {
          if (str[strlen(str) - 1] != ' ')
            strcat_P(str, PSTR(", "));
          strcat(str, weekdayName(timestr, i));
        }
      }
      strcat_P(str, strAt);
      strcat(str, timeToStr(timestr, _hour, _minute, _second));
      break;
    case MONTHLY:
      strcpy_P(str, strEvery);
      if (_day == LASTDAYOFMONTH)
        strcat_P(str, strLast);
      else
        strcat(str, itoa(_day, timestr, 10));
      strcat_P(str, PSTR(" day of month at "));
      strcat(str, timeToStr(timestr, _hour, _minute, _second));
      break;
    case YEARLY:
      strcpy_P(str, strEvery);
      if (_day == LASTDAYOFMONTH)
        strcat_P(str, strLast);
      else
        strcat(str, itoa(_day, timestr, 10));
      strcat_P(str, PSTR(" day of "));
      strcat(str, monthName(timestr, _month));
      strcat_P(str, strAt);
      strcat(str, timeToStr(timestr, _hour, _minute, _second));
      break;
    case ONCE:
      strcpy_P(str, PSTR("Once at "));
      strcat(str, dateTimeToStr(timestr, _day, _month, _year, _hour, _minute, _second));
      break;
  }

  return str;
}

char* Schedule::nextTimeStr(char* str) {
  if (_nextTime == 0)
    strcpy_P(str, strUndefined);
  else if (_nextTime == NEVER)
    strcpy_P(str, PSTR("Never"));
  else
    dateTimeToStr(str, _nextTime);

  return str;
}
