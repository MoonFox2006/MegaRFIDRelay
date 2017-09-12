#include <stdarg.h>
#include <avr/pgmspace.h>
#include "printp.h"

void print_P(Print& stream, PGM_P str, uint16_t maxlen) {
  if (! str)
    return;

  char c;

  while ((maxlen--) && (c = pgm_read_byte(str++))) {
    stream.write(c);
  }
}

void println_P(Print& stream, PGM_P str, uint16_t maxlen) {
  print_P(stream, str, maxlen);
  stream.println();
}

char* sprintf(char* str, uint16_t size, const char* fmt, ...) {
  va_list args;

  if ((! str) || (! size) || (! fmt))
    return NULL;
  va_start(args, fmt);
  vsnprintf(str, size, fmt, args);
  va_end(args);

  return str;
}

char* sprintf(char* str, uint16_t size, const __FlashStringHelper* fmt, ...) {
  va_list args;

  if ((! str) || (! size) || (! fmt))
    return NULL;
  va_start(args, fmt);
  vsnprintf_P(str, size, (PGM_P)fmt, args);
  va_end(args);

  return str;
}

void printf(Print& stream, const char* fmt, ...) {
  char buf[PRINTF_BUF_SIZE];
  va_list args;

  if (! fmt)
    return;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  stream.print(buf);
}

void printf(Print& stream, const __FlashStringHelper* fmt, ...) {
  char buf[PRINTF_BUF_SIZE];
  va_list args;

  if (! fmt)
    return;
  va_start(args, fmt);
  vsnprintf_P(buf, sizeof(buf), (PGM_P)fmt, args);
  va_end(args);
  stream.print(buf);
}
