#include "string.h"
#include "strp.h"

bool startsWith_P(const char* str, PGM_P prefix) {
  size_t len = strlen_P(prefix);

  if (len > strlen(str))
    return false;

  return (! strncmp_P(str, prefix, len));
}

bool endsWith_P(const char* str, PGM_P suffix) {
  size_t len1 = strlen(str);
  size_t len2 = strlen_P(suffix);

  if (len2 > len1)
    return false;

  return (! strcmp_P((const char*)&str[len1 - len2], suffix));
}
