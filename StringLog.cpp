#include "StringLog.h"

void StringLog::printTo(Stream& stream) {
  for (uint16_t i = 0; i < _logLen; ++i)
    stream.write(_log[i]);
}

void StringLog::printEncodedTo(Stream& stream) {
  for (uint16_t i = 0; i < _logLen; ++i) {
    char ch = _log[i];
    if (ch == '"')
      stream.print(F("&quot;"));
    else if (ch == '<')
      stream.print(F("&lt;"));
    else if (ch == '>')
      stream.print(F("&gt;"));
    else
      stream.write(ch);
  }
}

size_t StringLog::write(uint8_t ch) {
  return write((const uint8_t*)&ch, sizeof(ch));
}

size_t StringLog::write(const uint8_t* buffer, size_t size) {
  uint16_t i, len = 0;

  for (i = 0; i < size; ++i) {
    if ((buffer[i] == '\t') || (buffer[i] == '\n') || (buffer[i] >= ' '))
      ++len;
  }
  if (len) {
    if (len > LOG_SIZE - _logLen) { // Not enough free space in buffer
      len -= LOG_SIZE - _logLen;
      _logLen -= len;
      memcpy(_log, &_log[len], _logLen);
    }
    for (i = 0; i < size; ++i) {
      if ((buffer[i] == '\t') || (buffer[i] == '\n') || (buffer[i] >= ' '))
        _log[_logLen++] = buffer[i];
    }
    if (_logLen < LOG_SIZE)
      _log[_logLen] = '\0';
  }

  if (_duplicate)
    _duplicate->write(buffer, size);

  return size;
}
