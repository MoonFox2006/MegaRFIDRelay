#ifndef __SCHEDULE_H
#define __SCHEDULE_H

class Schedule { // Класс, реализующий расписание событий
public:
  enum period_t : uint8_t { NONE, MINUTELY, HOURLY, WEEKLY, MONTHLY, YEARLY, ONCE }; // Частота повторов: нет, ежеминутно, ежечасно, еженедельно(ежедневно), ежемесячно, ежегодно, однократно

  static const int8_t LASTDAYOFMONTH = 32; // Последний день месяца (при сравнении будет подставлено нужное значение)
  static const uint32_t NEVER = (uint32_t)-1; // Никогда (для времени следующего события в случае прошедшего однократного)

  Schedule() : _period(NONE) { } // Конструктор по умолчанию
  Schedule(int8_t ss) { set(ss); } // Ежеминутный конструктор
  Schedule(int8_t mm, int8_t ss) { set(mm, ss); } // Ежечасный конструктор
  Schedule(int8_t hh, int8_t mm, int8_t ss, uint8_t wd = 0B01111111) { set(hh, mm, ss, wd); } // Еженедельный(ежедневный) конструктор
  Schedule(int8_t hh, int8_t mm, int8_t ss, int8_t d) { set(hh, mm, ss, d); } // Ежемесячный конструктор
  Schedule(int8_t hh, int8_t mm, int8_t ss, int8_t d, int8_t m) { set(hh, mm, ss, d, m); } // Ежегодный конструктор
  Schedule(int8_t hh, int8_t mm, int8_t ss, int8_t d, int8_t m, int16_t y) { set(hh, mm, ss, d, m, y); } // Однократный конструктор
  void clear() { _period = NONE; _nextTime = 0; } // Очистка расписания
  void set(Schedule::period_t p, int8_t hh, int8_t mm, int8_t ss, uint8_t wd, int8_t d, int8_t m, int16_t y);
  void set(int8_t ss) { set(MINUTELY, 0, 0, ss, 0, 0, 0, 0); }
  void set(int8_t mm, int8_t ss) { set(HOURLY, 0, mm, ss, 0, 0, 0, 0); }
  void set(int8_t hh, int8_t mm, int8_t ss, uint8_t wd = 0B01111111) { set(WEEKLY, hh, mm, ss, wd, 0, 0, 0); }
  void set(int8_t hh, int8_t mm, int8_t ss, int8_t d) { set(MONTHLY, hh, mm, ss, 0, d, 0, 0); }
  void set(int8_t hh, int8_t mm, int8_t ss, int8_t d, int8_t m) { set(YEARLY, hh, mm, ss, 0, d, m, 0); }
  void set(int8_t hh, int8_t mm, int8_t ss, int8_t d, int8_t m, int16_t y) { set(ONCE, hh, mm, ss, 0, d, m, y); }
  period_t period() const { return _period; } // Частота повторов
  int8_t hour() const { return _hour; } // Час
  int8_t minute() const { return _minute; } // Минута
  int8_t second() const { return _second; } // Секунда
  uint8_t weekdays() const { return _weekdays; } // Дни недели
  int8_t day() const { return _day; } // День месяца
  int8_t month() const { return _month; } // Месяц
  int16_t year() const { return _year; } // Год
  bool check(uint32_t unixtime); // Проверить на совпадение расписания с указанным временем
  char* toString(char* str); // Строковое представление расписания
  char* nextTimeStr(char* str); // Строковое представление времени следующего события
protected:
  uint32_t next(uint32_t unixtime); // Вычисление времени следующего события

  period_t _period; // Частота повторов
  int8_t _hour; // Час
  int8_t _minute; // Минута
  int8_t _second; // Секунда
  union {
    uint8_t _weekdays; // Дни недели (битовое поле, где установленные биты 0..6 отображаются на пн..вс)
    struct {
      int8_t _day; // День месяца (может быть специальной константой LASTDAYOFMONTH)
      int8_t _month; // Месяц
      int16_t _year; // Год
    };
  };
  uint32_t _nextTime; // Время следующего события (0 - еще не вычислено, NEVER - никогда и не нужно больше вычислять)
};

#endif
