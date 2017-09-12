#ifndef __STRINGLOG_H
#define __STRINGLOG_H

#include <Print.h>
#include <Stream.h>

class StringLog : public Print {
public:
  StringLog(const Stream* duplicate = NULL) : Print() {
    *_log = '\0';
    _logLen = 0;
    _duplicate = (Stream*)duplicate;
  }
  void clear() {
    *_log = '\0';
    _logLen = 0;
  }
  const char* text() const {
    return _log;
  }
  void printTo(Stream& stream);
  void printEncodedTo(Stream& stream);

  using Print::write;
  size_t write(uint8_t ch) override;
  size_t write(const uint8_t* buffer, size_t size) override;

  static const uint16_t LOG_SIZE = 1024; // Maximum size of log in characters

protected:
  char _log[LOG_SIZE];
  uint16_t _logLen;
  Stream* _duplicate;
};

#endif
