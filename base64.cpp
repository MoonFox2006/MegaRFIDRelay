#include "base64.h"

static char to_base64(uint8_t val) {
  if (val < 26)
    return 'A' + val;
  if (val < 52)
    return 'a' + (val - 26);
  if (val < 62)
    return '0' + (val - 52);
  if (val == 62)
    return '+';
  if (val == 63)
    return '/';
}

static bool is_base64(char val) {
  return ((val >= 'A') && (val <= 'Z')) || ((val >= 'a') && (val <= 'z')) || ((val >= '0') && (val <= '9')) || (val == '+') || (val == '/');
}

static uint8_t from_base64(char val) {
  if ((val >= 'A') && (val <= 'Z'))
    return val - 'A';
  if ((val >= 'a') && (val <= 'z'))
    return val - 'a' + 26;
  if ((val >= '0') && (val <= '9'))
    return val - '0' + 52;
  if (val == '+')
    return 62;
  if (val == '/')
    return 63;
}

char* encode_base64(char* dest, const uint8_t* src, uint16_t size) {
  uint32_t srcbuf;
  char* destptr = dest;

  while (size) {
    srcbuf = (uint32_t)*src++ << 16;
    if (size > 1)
      srcbuf |= (uint32_t)*src++ << 8;
    if (size > 2)
      srcbuf |= *src++;
    *destptr++ = to_base64((srcbuf >> 18) & 0B00111111);
    *destptr++ = to_base64((srcbuf >> 12) & 0B00111111);
    if (size > 1)
      *destptr++ = to_base64((srcbuf >> 6) & 0B00111111);
    else
      *destptr++ = '=';
    if (size > 2)
      *destptr++ = to_base64(srcbuf & 0B00111111);
    else
      *destptr++ = '=';
    if (size > 3)
      size -= 3;
    else
      size = 0;
  }
  *destptr = '\0';

  return dest;
}

uint8_t* decode_base64(uint8_t* dest, const char* src) {
  uint32_t destbuf;
  uint8_t* destptr = dest;

  while (*src) {
    if (! is_base64(*src))
      return (uint8_t*)0;
    destbuf = (uint32_t)from_base64(*src++) << 18;
    if (! is_base64(*src))
      return (uint8_t*)0;
    destbuf |= (uint32_t)from_base64(*src++) << 12;
    *destptr++ = (destbuf >> 16) & 0xFF;
    if (is_base64(*src)) {
      destbuf |= (uint32_t)from_base64(*src++) << 6;
      *destptr++ = (destbuf >> 8) & 0xFF;
    } else if (*src != '=')
      return (uint8_t*)0;
    if (is_base64(*src)) {
      destbuf |= from_base64(*src++);
      *destptr++ = destbuf & 0xFF;
    } else if (*src != '=')
      return (uint8_t*)0;
  }

  return dest;
}
