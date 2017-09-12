#ifndef __BASE64_H
#define __BASE64_H

#include <stdint.h>

char* encode_base64(char* dest, const uint8_t* src, uint16_t size);
uint8_t* decode_base64(uint8_t* dest, const char* src);

#endif
