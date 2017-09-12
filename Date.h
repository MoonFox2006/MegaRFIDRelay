#ifndef __DATE_H
#define __DATE_H

bool isLeapYear(int16_t year);
int8_t lastDayOfMonth(int8_t month, int16_t year);

void parseUnixTime(uint32_t unixtime, int8_t& hour, int8_t& minute, int8_t& second, uint8_t& weekday, int8_t& day, int8_t& month, int16_t& year);
uint32_t combineUnixTime(int8_t hour, int8_t minute, int8_t second, int8_t day, int8_t month, int16_t year);

char* timeToStr(char* str, int8_t hour, int8_t minute, int8_t second);
char* timeToStr(char* str, uint32_t unixtime);
char* dateToStr(char* str, int8_t day, int8_t month, int16_t year);
char* dateToStr(char* str, uint32_t unixtime);
char* timeDateToStr(char* str, int8_t hour, int8_t minute, int8_t second, int8_t day, int8_t month, int16_t year);
char* timeDateToStr(char* str, uint32_t unixtime);
char* dateTimeToStr(char* str, int8_t day, int8_t month, int16_t year, int8_t hour, int8_t minute, int8_t second);
char* dateTimeToStr(char* str, uint32_t unixtime);

char* weekdayName(char* str, uint8_t weekday);
char* monthName(char* str, int8_t month);

#endif
