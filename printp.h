#ifndef __PRINTP_H
#define __PRINTP_H

#include <Print.h>

#define FPSTR(s) (const __FlashStringHelper*)(s)

const uint16_t PRINTF_BUF_SIZE = 256;

void print_P(Print& stream, PGM_P str, uint16_t maxlen = (uint16_t)-1);
void println_P(Print& stream, PGM_P str, uint16_t maxlen = (uint16_t)-1);

char* sprintf(char* str, uint16_t size, const char* fmt, ...);
char* sprintf(char* str, uint16_t size, const __FlashStringHelper* fmt, ...);

void printf(Print& stream, const char* fmt, ...);
void printf(Print& stream, const __FlashStringHelper* fmt, ...);

#endif
