#ifndef __STRP_H
#define __STRP_H

#include <avr/pgmspace.h>

inline bool equals_P(const char* str1, PGM_P str2) {
  return (! strcmp_P(str1, str2));
}

bool startsWith_P(const char* str, PGM_P prefix);
bool endsWith_P(const char* str, PGM_P suffix);

#endif
