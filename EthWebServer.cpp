#include <stdlib.h>
#include <string.h>
#include <EEPROM.h>
#include <SPI.h>
#include <EthernetUdp.h>
#include "EthWebServer.h"
#include "strp.h"
#include "Date.h"
#include "base64.h"
#ifdef USEDS3231
#include "DS3231.h"
#endif
#ifdef USEAT24C32
#include "AT24C32.h"
#endif

const char headerAuthorization[] PROGMEM = "Authorization";

/***
 * EthWebServerApp class implementation
 */

EthWebServerApp::EthWebServerApp(uint8_t chunks, uint16_t port) : _crc(0xFF), _lastTime(0), _lastMillis(0), _nextUpdate(0), _log(StringLog(&Serial)), _server(new EthernetServer(port)), _client(NULL), _bufferLength(0), _uri(NULL), _argCount(0), _headerCount(0),
#ifdef USEMQTT
#ifdef USEAUTHORIZATION
  _chunks(chunks + 4),
#else
  _chunks(chunks + 3),
#endif
  _ethClient(NULL), _pubSubClient(NULL) {
  _this = this;
#else
#ifdef USEAUTHORIZATION
  _chunks(chunks + 3) {
#else
  _chunks(chunks + 2) {
#endif
#endif
}

void EthWebServerApp::setup() {
#if defined(USEDS3231) || defined(USEAT24C32)
  const bool WIRE_FASTMODE = true;
#endif

#ifdef USEDS3231
  ds3231.init(WIRE_FASTMODE);
  _haveDS3231 = ds3231.begin();
  if (! _haveDS3231) {
    _log.println(F("DS3231 not detected!"));
  }
#endif
#ifdef USEAT24C32
#ifndef USEDS3231
  at24c32.init(WIRE_FASTMODE); // Wire initialization must be single!
#endif
  _haveAT24C32 = at24c32.begin();
  if (! _haveAT24C32) {
    _log.println(F("AT24C32 EEPROM not detected!"));
  }
#endif

  SPI.begin();

  if (! readParams()) {
    _log.println(F("EEPROM is empty or corrupt, default parameters will be used!"));
  }
#ifdef SD_SS_PIN
#ifdef USESDCARD
  _haveSD = SD.begin(SD_SS_PIN);
  if (_haveSD)
    _log.println(F("SD card is detected"));
  else
    _log.println(F("SD card is not present!"));
#else // #ifdef USESDCARD
  pinMode(SD_SS_PIN, OUTPUT);
  digitalWrite(SD_SS_PIN, HIGH); // Disable SD card on ethernet shield
  _log.println(F("SD card disabled"));
#endif // #ifdef USESDCARD
#endif // #ifdef SD_SS_PIN
  _nextMaintenance = (uint32_t)-1;
  if (ip) {
    Ethernet.begin(mac, IPAddress(ip), IPAddress(dns), IPAddress(gate), IPAddress(mask));
    _log.print(F("Use static IP "));
  } else {
    if (Ethernet.begin(mac)) {
      _log.print(F("Use DHCP IP "));
      _nextMaintenance = millis() + (uint32_t)maintenanceInterval * 60000;
    } else {
      _log.println(F("Failed to configure Ethernet using DHCP!"));
      Ethernet.begin(mac, IPAddress(defIP));
      _log.print(F("Use default static IP "));
    }
  }
  _log.print(Ethernet.localIP());
  _log.print('/');
  _log.print(Ethernet.subnetMask());
  _log.print(F(", gateway "));
  _log.print(Ethernet.gatewayIP());
  _log.print(F(" and DNS server "));
  _log.println(Ethernet.dnsServerIP());

  _server->begin();

  logDateTime(F("Module started"), 0, true);

#ifdef USEMQTT
  if (*mqttServer) {
    _ethClient = new EthernetClient();
    _pubSubClient = new PubSubClient(mqttServer, mqttPort, _mqttCallback, *_ethClient);
  }
#endif
}

void EthWebServerApp::loop() {
  const uint32_t MAINTENANCE_RETRY = 60000; // 1 min. in ms.

  if ((_nextMaintenance != (uint32_t)-1) && ((int32_t)(millis() - _nextMaintenance) >= 0)) {
    switch (Ethernet.maintain()) {
      case DHCP_CHECK_RENEW_FAIL:
      case DHCP_CHECK_REBIND_FAIL:
        logDateTime(F("Error renewing or rebinding DHCP address!"), 0, true);
        _nextMaintenance = millis() + MAINTENANCE_RETRY;
        break;
      case DHCP_CHECK_RENEW_OK:
      case DHCP_CHECK_REBIND_OK:
        logDateTime(F("Renewed or rebinded DHCP address "));
        _log.println(Ethernet.localIP());
      default: // no break before is not an error!
        _nextMaintenance = millis() + (uint32_t)maintenanceInterval * 60000;
    }
  }

  handleClient();

#ifdef USEMQTT
  if (_pubSubClient) {
    if (! _pubSubClient->connected())
      mqttReconnect();
    if (_pubSubClient->connected())
      _pubSubClient->loop();
  }
#endif

//  delay(1);
}

const uint8_t SIGNATURE_SIZE = 4;
const char _signature[SIGNATURE_SIZE] PROGMEM = { '#', 'A', 'V', 'R' };

void EthWebServerApp::clearParams() {
  char sign[SIGNATURE_SIZE];

  memset(sign, 0xFF, SIGNATURE_SIZE);
  _writeData(0, sign, SIGNATURE_SIZE, false);
}

bool EthWebServerApp::readParams() {
  int16_t result;

  result = checkParamsSignature();
  if (result < 0) {
    defaultParams(0);
    return false;
  }
  for (uint8_t chunk = 0; chunk < _chunks; ++chunk) {
    initCRC();
    result = readParamsData(result, chunk);
    if (result > 0)
      result = checkCRC(result);
    if (result < 0) {
      defaultParams(chunk);
      return false;
    }
  }

  return true;
}

bool EthWebServerApp::writeParams() {
  int16_t result;

  result = writeParamsSignature();
  if (result < 0)
    return false;
  for (uint8_t chunk = 0; chunk < _chunks; ++chunk) {
    initCRC();
    result = writeParamsData(result, chunk);
    if (result > 0)
      result = writeCRC(result);
    if (result < 0)
      return false;
  }

  return true;
}

void EthWebServerApp::defaultParams(uint8_t chunk) {
  if (chunk < 1) {
    memcpy_P(mac, defMAC, sizeof(mac));
    ip = 0; // defIP;
    mask = defMask;
    gate = defGate;
    dns = defDNS;
    maintenanceInterval = defMaintenanceInterval;
  }
  if (chunk < 2) {
    memset(ntpServer, 0, sizeof(ntpServer));
    strncpy_P(ntpServer, defNtpServer, sizeof(ntpServer) - 1);
    timeZone = defTimeZone;
    updateInterval = defUpdateInterval;
  }
#ifdef USEAUTHORIZATION
  if (chunk < 3) {
    memset(authUser, 0, sizeof(authUser));
    strncpy_P(authUser, defAuthUser, sizeof(authUser) - 1);
    memset(authPassword, 0, sizeof(authPassword));
    strncpy_P(authPassword, defAuthPassword, sizeof(authPassword) - 1);
  }
#endif
#ifdef USEMQTT
#ifdef USEAUTHORIZATION
  if (chunk < 4) {
#else
  if (chunk < 3) {
#endif
    memset(mqttServer, 0, sizeof(mqttServer));
    mqttPort = defMQTTPort;
    memset(mqttUser, 0, sizeof(mqttUser));
    memset(mqttPassword, 0, sizeof(mqttPassword));
    memset(mqttClient, 0, sizeof(mqttClient));
    strncpy_P(mqttClient, defMQTTClient, sizeof(mqttClient) - 1);
  }
#endif
}

static uint8_t strToHex(const char* digits) {
  uint8_t result;

  if ((digits[0] >= '0') && (digits[0] <= '9'))
    result = (digits[0] - '0') << 4;
  else if ((digits[0] >= 'A') && (digits[0] <= 'F'))
    result = (digits[0] - 'A' + 10) << 4;
  else if ((digits[0] >= 'a') && (digits[0] <= 'f'))
    result = (digits[0] - 'a' + 10) << 4;
  else
    return 0;

  if ((digits[1] >= '0') && (digits[1] <= '9'))
    result |= (digits[1] - '0');
  else if ((digits[1] >= 'A') && (digits[1] <= 'F'))
    result |= (digits[1] - 'A' + 10);
  else if ((digits[1] >= 'a') && (digits[1] <= 'f'))
    result |= (digits[1] - 'a' + 10);
  else
    return 0;

  return result;
}

bool EthWebServerApp::setParameter(const char* name, const char* value) {
  if (equals_P(name, paramMAC)) {
    if (strlen(value) == 12) {
      for (uint8_t i = 0; i < 6; ++i)
        mac[i] = strToHex(&value[i * 2]);
    }
  } else if (equals_P(name, paramIP)) {
    ip = atol(value);
  } else if (equals_P(name, paramMask)) {
    mask = atol(value);
  } else if (equals_P(name, paramGate)) {
    gate = atol(value);
  } else if (equals_P(name, paramDNS)) {
    dns = atol(value);
  } else if (equals_P(name, paramMaintenance)) {
    maintenanceInterval = atol(value);
  } else if (equals_P(name, paramNtpServer)) {
    memset(ntpServer, 0, sizeof(ntpServer));
    strncpy(ntpServer, value, sizeof(ntpServer) - 1);
  } else if (equals_P(name, paramTimeZone)) {
    timeZone = constrain(atoi(value), -12, 14);
  } else if (equals_P(name, paramUpdateInterval)) {
    updateInterval = atol(value);
  } else
#ifdef USEAUTHORIZATION
  if (equals_P(name, paramAuthUser)) {
    memset(authUser, 0, sizeof(authUser));
    strncpy(authUser, value, sizeof(authUser) - 1);
  } else if (equals_P(name, paramAuthPassword)) {
    memset(authPassword, 0, sizeof(authPassword));
    strncpy(authPassword, value, sizeof(authPassword) - 1);
  } else
#endif
#ifdef USEMQTT
  if (equals_P(name, paramMQTTServer)) {
    memset(mqttServer, 0, sizeof(mqttServer));
    strncpy(mqttServer, value, sizeof(mqttServer) - 1);
  } else if (equals_P(name, paramMQTTPort)) {
    mqttPort = atol(value);
  } else if (equals_P(name, paramMQTTUser)) {
    memset(mqttUser, 0, sizeof(mqttUser));
    strncpy(mqttUser, value, sizeof(mqttUser) - 1);
  } else if (equals_P(name, paramMQTTPassword)) {
    memset(mqttPassword, 0, sizeof(mqttPassword));
    strncpy(mqttPassword, value, sizeof(mqttPassword) - 1);
  } else if (equals_P(name, paramMQTTClient)) {
    memset(mqttClient, 0, sizeof(mqttClient));
    strncpy(mqttClient, value, sizeof(mqttClient) - 1);
  } else
#endif
    return false;

  return true;
}

int16_t EthWebServerApp::readParamsData(uint16_t offset, uint8_t chunk) {
  int16_t result = offset;

  if (chunk == 0) {
    result += _readData(result, mac, sizeof(mac));
    result += _readData(result, &ip, sizeof(ip));
    result += _readData(result, &mask, sizeof(mask));
    result += _readData(result, &gate, sizeof(gate));
    result += _readData(result, &dns, sizeof(dns));
    result += _readData(result, &maintenanceInterval, sizeof(maintenanceInterval));
  } else if (chunk == 1) {
    result += _readData(result, ntpServer, sizeof(ntpServer));
    result += _readData(result, &timeZone, sizeof(timeZone));
    result += _readData(result, &updateInterval, sizeof(updateInterval));
  } else
#ifdef USEAUTHORIZATION
  if (chunk == 2) {
    result += _readData(result, authUser, sizeof(authUser));
    result += _readData(result, authPassword, sizeof(authPassword));
  } else
#endif
#ifdef USEMQTT
#ifdef USEAUTHORIZATION
  if (chunk == 3) {
#else
  if (chunk == 2) {
#endif
    result += _readData(result, mqttServer, sizeof(mqttServer));
    result += _readData(result, &mqttPort, sizeof(mqttPort));
    result += _readData(result, mqttUser, sizeof(mqttUser));
    result += _readData(result, mqttPassword, sizeof(mqttPassword));
    result += _readData(result, mqttClient, sizeof(mqttClient));
  } else
#endif
    result = -1;

  return result;
}

int16_t EthWebServerApp::writeParamsData(uint16_t offset, uint8_t chunk) {
  int16_t result = offset;

  if (chunk == 0) {
    result += _writeData(result, mac, sizeof(mac));
    result += _writeData(result, &ip, sizeof(ip));
    result += _writeData(result, &mask, sizeof(mask));
    result += _writeData(result, &gate, sizeof(gate));
    result += _writeData(result, &dns, sizeof(dns));
    result += _writeData(result, &maintenanceInterval, sizeof(maintenanceInterval));
  } else if (chunk == 1) {
    result += _writeData(result, ntpServer, sizeof(ntpServer));
    result += _writeData(result, &timeZone, sizeof(timeZone));
    result += _writeData(result, &updateInterval, sizeof(updateInterval));
  } else
#ifdef USEAUTHORIZATION
  if (chunk == 2) {
    result += _writeData(result, authUser, sizeof(authUser));
    result += _writeData(result, authPassword, sizeof(authPassword));
  } else
#endif
#ifdef USEMQTT
#ifdef USEAUTHORIZATION
  if (chunk == 3) {
#else
  if (chunk == 2) {
#endif
    result += _writeData(result, mqttServer, sizeof(mqttServer));
    result += _writeData(result, &mqttPort, sizeof(mqttPort));
    result += _writeData(result, mqttUser, sizeof(mqttUser));
    result += _writeData(result, mqttPassword, sizeof(mqttPassword));
    result += _writeData(result, mqttClient, sizeof(mqttClient));
  } else
#endif
    result = -1;

  return result;
}

int16_t EthWebServerApp::checkParamsSignature() {
  char sign[SIGNATURE_SIZE];

  if (_readData(0, sign, SIGNATURE_SIZE, false) != SIGNATURE_SIZE)
    return -1;
  for (uint8_t i = 0; i < SIGNATURE_SIZE; ++i) {
    if (sign[i] != pgm_read_byte(_signature + i))
      return -1;
  }

  return SIGNATURE_SIZE;
}

int16_t EthWebServerApp::writeParamsSignature() {
  char sign[SIGNATURE_SIZE];

  memcpy_P(sign, _signature, SIGNATURE_SIZE);
  if (_writeData(0, sign, SIGNATURE_SIZE, false) != SIGNATURE_SIZE)
    return -1;

  return SIGNATURE_SIZE;  
}

uint16_t EthWebServerApp::_readData(uint16_t offset, void* buf, uint16_t size, bool calcCRC) {
  uint8_t* bufptr = (uint8_t*)buf;
  uint16_t result = 0;
  uint8_t data;

  while (size--) {
#ifdef USEAT24C32
    if (_haveAT24C32)
      data = at24c32.read(offset++);
    else
#endif
    data = EEPROM.read(offset++);
    if (calcCRC)
      updateCRC(data);
    *bufptr++ = data;
    ++result;
  }

  return result;
}

uint16_t EthWebServerApp::_writeData(uint16_t offset, const void* buf, uint16_t size, bool calcCRC) {
  uint8_t* bufptr = (uint8_t*)buf;
  uint16_t result = 0;
  uint8_t data;

  while (size--) {
    data = *bufptr++;
    if (calcCRC)
      updateCRC(data);
#ifdef USEAT24C32
    if (_haveAT24C32)
      at24c32.update(offset++, data);
    else
#endif
    EEPROM.update(offset++, data);
    ++result;
  }

  return result;
}

int16_t EthWebServerApp::checkCRC(uint16_t offset) {
  uint8_t crc;

  if ((_readData(offset, &crc, sizeof(crc), false) != sizeof(crc)) || (crc != _crc))
    return -1;

  return (offset + sizeof(crc));
}

int16_t EthWebServerApp::writeCRC(uint16_t offset) {
  if (_writeData(offset, &_crc, sizeof(_crc), false) != sizeof(_crc))
    return -1;

  return (offset + sizeof(_crc));
}

uint8_t EthWebServerApp::_crc8update(uint8_t crc, uint8_t data) {
  crc ^= data;

  for (uint8_t i = 0; i < 8; ++i)
    crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;

  return crc;
}

uint8_t EthWebServerApp::_crc8(uint8_t crc, const uint8_t* buf, uint16_t size) {
  while (size--) {
    crc = _crc8update(crc, *buf++);
  }

  return crc;
}

uint32_t EthWebServerApp::getTime() {
#ifdef USEDS3231
  if (_haveDS3231)
    return ds3231.get();
#endif

  const uint32_t RETRY_TIMEOUT = 5000;
  const uint8_t MAX_RETRY = 5;

  static uint8_t retry = 0;

  if (*ntpServer && (((! _lastTime) && (retry < MAX_RETRY)) || (_nextUpdate && ((int32_t)(millis() - _nextUpdate) >= 0)))) {
    _log.print(F("NTP update "));
    if (updateTime()) {
      retry = 0;
      if (updateInterval)
        _nextUpdate = millis() + (uint32_t)updateInterval * 60000;
      else
        _nextUpdate = 0;
      _log.println(F("successful"));
    } else {
      if (retry < MAX_RETRY) {
        ++retry;
        _nextUpdate = millis() + RETRY_TIMEOUT;
      } else
        _nextUpdate = millis() + 60000;
      _log.println(F("fail!"));
    }
  }
  if (_lastTime)
    return (_lastTime + (millis() - _lastMillis) / 1000);

  return 0;
}

void EthWebServerApp::setTime(uint32_t time) {
#ifdef USEDS3231
  if (_haveDS3231)
    return ds3231.set(time);
#endif
  _lastMillis = millis();
  _lastTime = time;
}

bool EthWebServerApp::updateTime(uint32_t timeout) {
  const uint16_t localPort = 8888;
  const uint8_t NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
  const uint32_t seventyYears = 2208988800UL;

  EthernetUDP udp;
  uint8_t buf[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets

  memset(buf, 0, sizeof(buf));
  // Initialize values needed to form NTP request
  buf[0] = 0B11100011; // LI, Version, Mode
  buf[1] = 0; // Stratum, or type of clock
  buf[2] = 6; // Polling Interval
  buf[3] = 0xEC; // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  buf[12] = 49;
  buf[13] = 0x4E;
  buf[14] = 49;
  buf[15] = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp
  if (! udp.begin(localPort))
    return false;
  if (! udp.beginPacket(ntpServer, 123)) { // NTP requests are to port 123
    udp.stop();
    return false;
  }
  if (udp.write(buf, NTP_PACKET_SIZE) != NTP_PACKET_SIZE) {
    udp.stop();
    return false;
  }
  if (! udp.endPacket()) {
    udp.stop();
    return false;
  }
  uint32_t time = millis();
  while (! udp.parsePacket()) {
    if (millis() - time > timeout) {
      udp.stop();
      return false;
    }
    delay(1);
  }
  if (udp.read(buf, NTP_PACKET_SIZE) != NTP_PACKET_SIZE) { // read the packet into the buffer
    udp.stop();
    return false;
  }
  udp.stop();
  // combine the four bytes (two words) into a long integer
  // this is NTP time (seconds since Jan 1 1900)
  time = ((uint32_t)buf[40] << 24) | ((uint32_t)buf[41] << 16) | ((uint32_t)buf[42] << 8) | buf[43];
  time -= seventyYears;
//  setTime(time + timeZone * 3600);
  _lastMillis = millis();
  _lastTime = time + timeZone * 3600;

  return true;
}

void EthWebServerApp::logDate(const char* str, uint32_t time, bool newline) {
  if (! time)
    time = getTime();
  if (time) {
    char str[11];

    _log.print(dateToStr(str, time));
    _log.write(charSpace);
  }
  if (str)
    _log.print(str);
  if (newline)
    _log.println();
}

void EthWebServerApp::logDate(const __FlashStringHelper* str, uint32_t time, bool newline) {
  if (! time)
    time = getTime();
  if (time) {
    char str[11];

    _log.print(dateToStr(str, time));
    _log.write(charSpace);
  }
  if (str)
    _log.print(str);
  if (newline)
    _log.println();
}

void EthWebServerApp::logTime(const char* str, uint32_t time, bool newline) {
  if (! time)
    time = getTime();
  if (time) {
    char str[9];

    _log.print(timeToStr(str, time));
    _log.write(charSpace);
  }
  if (str)
    _log.print(str);
  if (newline)
    _log.println();
}

void EthWebServerApp::logTime(const __FlashStringHelper* str, uint32_t time, bool newline) {
  if (! time)
    time = getTime();
  if (time) {
    char str[9];

    _log.print(timeToStr(str, time));
    _log.write(charSpace);
  }
  if (str)
    _log.print(str);
  if (newline)
    _log.println();
}

void EthWebServerApp::logDateTime(const char* str, uint32_t time, bool newline) {
  if (! time)
    time = getTime();
  if (time) {
    char str[20];

    _log.print(dateTimeToStr(str, time));
    _log.write(charSpace);
  }
  if (str)
    _log.print(str);
  if (newline)
    _log.println();
}

void EthWebServerApp::logDateTime(const __FlashStringHelper* str, uint32_t time, bool newline) {
  if (! time)
    time = getTime();
  if (time) {
    char str[20];

    _log.print(dateTimeToStr(str, time));
    _log.write(charSpace);
  }
  if (str)
    _log.print(str);
  if (newline)
    _log.println();
}

void EthWebServerApp::handleClient() {
  EthernetClient client = _server->available();
  if ((! client) || (! client.connected()))
    return;

  _client = &client;

  if (parseRequest()) {
    handleRequest(_uri);
  }

  _client = NULL;
  client.stop();
}

bool EthWebServerApp::checkClient() {
  if (_client && _client->connected())
    return true;
#ifdef DEBUG
  _log.println(F("Client not connected!"));
#endif
  return false;
}

void EthWebServerApp::_startResponse(int16_t code, const char* contentType, uint32_t contentLength, bool incomplete, bool progmem) {
  printf(*_client, F("HTTP/1.1 %d %S\r\n"), code, FPSTR(responseCodeToString(code)));
  _client->print(F("Content-Type: "));
  if (progmem)
    _client->println(FPSTR(contentType));
  else
    _client->println(contentType);
  if (contentLength != (uint32_t)-1)
    printf(*_client, F("Content-Length: %lu\r\n"), contentLength);
  _client->println(F("Connection: close"));
  _client->println(F("Access-Control-Allow-Origin: *"));
  if (! incomplete)
    _client->println();
}

bool EthWebServerApp::startResponse(int16_t code, const char* contentType, uint32_t contentLength, bool incomplete) {
  if (! checkClient())
    return false;

  _startResponse(code, contentType, contentLength, incomplete, false);

  return true;
}

bool EthWebServerApp::startResponse(int16_t code, const __FlashStringHelper* contentType, uint32_t contentLength, bool incomplete) {
  if (! checkClient())
    return false;

  _startResponse(code, (PGM_P)contentType, contentLength, incomplete, true);

  return true;
}

bool EthWebServerApp::continueResponse() {
  if (! checkClient())
    return false;

  _client->println();

  return true;
}

bool EthWebServerApp::endResponse() {
  if (! checkClient())
    return false;

  _client->flush();
//  _client->stop();

  return true;
}

bool EthWebServerApp::_authenticate(const char* user, const char* password, bool progmem1, bool progmem2) {
  if (hasHeader(FPSTR(headerAuthorization))) {
    char* authReq = (char*)header(FPSTR(headerAuthorization));
    if (startsWith_P(authReq, PSTR("Basic "))) {
      char auth[80], encoded[109];

      authReq += 6;
      if (progmem1)
        strcpy_P(auth, user);
      else
        strcpy(auth, user);
      strcat_P(auth, PSTR(":"));
      if (progmem2)
        strcat_P(auth, password);
      else
        strcat(auth, password);
      encode_base64(encoded, (const uint8_t*)auth, strlen(auth));
      return (! strcmp(authReq, encoded));
    }
  }

  return false;
}

void EthWebServerApp::requestAuthentication() {
  static const char page[] PROGMEM = "<!DOCTYPE html><html><body><h3>Forbidden!</h3></body></html>";

  if (! startResponse(401, FPSTR(typeTextHtml), strlen_P(page), true))
    return;
  sendHeader(F("WWW-Authenticate"), F("Basic realm=\"Login Required\""));
  continueResponse();
  sendContent(FPSTR(page));
  endResponse();
}

void EthWebServerApp::handleRequest(const char* uri) {
  if (equals_P(uri, strSlash) || equals_P(uri, pathRoot) || ((! strncmp_P(uri, pathRoot, 10)) && (! uri[10])))
    handleRootPage();
  else if (equals_P(uri, pathFavicon))
    handleFavicon();
  else if (equals_P(uri, pathJson))
    handleJson();
  else if (equals_P(uri, pathStore))
    handleStore();
  else if (equals_P(uri, pathEthernetConfig))
    handleEthernetConfig();
  else if (equals_P(uri, pathTimeConfig))
    handleTimeConfig();
  else if (equals_P(uri, pathSetTime))
    handleSetTime();
  else if (equals_P(uri, pathLog))
    handleLog();
  else if (equals_P(uri, pathClearLog))
    handleClearLog();
  else if (equals_P(uri, pathRestart))
    handleRestart();
  else
#ifdef USEMQTT
  if (equals_P(uri, pathMQTTConfig))
    handleMQTTConfig();
  else
#endif
#ifdef USESDCARD
  if (equals_P(uri, pathSDCard))
    handleSDCard();
  else if ((! _haveSD) || (! handleFile()))
#endif
    handleNotFound();
}

void EthWebServerApp::handleRootPage() {
#ifdef USEAUTHORIZATION
  if (! authenticate(authUser, authPassword))
    return requestAuthentication();
#endif

  if (! startResponse(200, FPSTR(typeTextHtml)))
    return;

  htmlPageStart(F("Arduino Web Server"));
  htmlPageStdStyle();
  htmlPageScriptStart();
  htmlPageStdScript(false);
  printf(*_client, F("function refreshData(){\n\
var request=getXmlHttpRequest();\n\
request.open('GET','%S?%S=0&dummy='+Date.now(),true);\n"), FPSTR(pathJson), FPSTR(paramSrc));
  _client->print(F("request.onreadystatechange=function(){\n\
if (request.readyState==4){\n\
var data=JSON.parse(request.responseText);\n"));
  printf(*_client, F("document.getElementById('%S').innerHTML=data.%S;\n"), FPSTR(jsonUptime), FPSTR(jsonUptime));
  printf(*_client, F("document.getElementById('%S').innerHTML=data.%S;\n"), FPSTR(jsonFreeMem), FPSTR(jsonFreeMem));
#ifdef USEMQTT
  printf(*_client, F("document.getElementById('%S').innerHTML=(data.%S!=true)?\"not \":\"\";\n"), FPSTR(jsonMQTTConnected), FPSTR(jsonMQTTConnected));
#endif
  _client->print(F("}\n\
}\n\
request.send(null);\n\
}\n\
setInterval(refreshData,1000);\n"));
  htmlPageScriptEnd();
  htmlPageBody();
  htmlTag(FPSTR(tagH3), F("Arduino Web Server"), true);
  printf(*_client, F("Uptime: <span id=\"%S\">?</span> sec.\n"), FPSTR(jsonUptime));
  printf(*_client, F("Free memory: <span id=\"%S\">?</span> bytes\n"), FPSTR(jsonFreeMem));
#ifdef USEMQTT
  htmlTagBR(true);
  printf(*_client, F("MQTT broker is <span id=\"%S\">not </span>connected\n"), FPSTR(jsonMQTTConnected));
#endif
  btnNavigator();
  htmlPageEnd();
  endResponse();
}

void EthWebServerApp::handleNotFound() {
  if (! startResponse(404, FPSTR(typeTextHtml)))
    return;

  htmlPageStart(F("Page Not Found"));
  htmlPageStdStyle();
  htmlPageBody();
  htmlTag(FPSTR(tagH3), F("Page Not Found!"), true);
  printf(*_client, F("URL: %s"), _uri);
  htmlTagBR(true);
  printf(*_client, F("Argument(s) count: %d"), args());
  htmlTagBR(true);

  uint8_t i;

  for (i = 0; i < args(); ++i) {
    printf(*_client, F("%d %s=\"%s\""), i + 1, argName(i), arg(i));
    htmlTagBR(true);
  }
  htmlTagBR(true);
  printf(*_client, F("Header(s) count: %d"), headers());
  htmlTagBR(true);
  for (i = 0; i < headers(); ++i) {
    printf(*_client, F("%d %s: \"%s\""), i + 1, headerName(i), header(i));
    htmlTagBR(true);
  }
  htmlPageEnd();
  endResponse();
}

void EthWebServerApp::handleFavicon() {
  const uint16_t BUF_SIZE = 128;

  static const uint8_t favicon[] PROGMEM = {
    0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x10, 0x10, 0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x68, 0x04,
    0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x20, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x99, 0x94,
    0x00, 0x36, 0x9A, 0x96, 0x00, 0xAE, 0x9A, 0x95, 0x00, 0xDE, 0x9A, 0x95, 0x00, 0xFC, 0x9A, 0x95,
    0x00, 0xFC, 0x9A, 0x95, 0x00, 0xDE, 0x9A, 0x94, 0x00, 0xAE, 0x99, 0x94, 0x00, 0x36, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9B, 0x95, 0x00, 0xB6, 0xA3, 0x9E,
    0x00, 0xFF, 0x9E, 0x99, 0x00, 0xFF, 0x9C, 0x97, 0x00, 0xFF, 0x9B, 0x96, 0x00, 0xFF, 0x9B, 0x96,
    0x00, 0xFF, 0x9C, 0x97, 0x00, 0xFF, 0x9E, 0x99, 0x00, 0xFF, 0xA3, 0x9E, 0x00, 0xFF, 0x9B, 0x95,
    0x00, 0xB6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9B, 0x95, 0x00, 0xDC, 0xA0, 0x9A, 0x00, 0xFF, 0x9B, 0x96,
    0x00, 0xFF, 0x9B, 0x95, 0x00, 0xFF, 0x9B, 0x96, 0x00, 0xFF, 0x9B, 0x96, 0x00, 0xFF, 0x9B, 0x96,
    0x00, 0xFF, 0x9B, 0x96, 0x00, 0xFF, 0x9B, 0x96, 0x00, 0xFF, 0x9B, 0x96, 0x00, 0xFF, 0xA0, 0x9B,
    0x00, 0xFF, 0x9B, 0x95, 0x00, 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x9B, 0x97, 0x00, 0xB6, 0xA0, 0x9B, 0x00, 0xFF, 0x99, 0x93, 0x00, 0xFF, 0x97, 0x92,
    0x00, 0xFF, 0x98, 0x93, 0x00, 0xFF, 0x9B, 0x96, 0x00, 0xFF, 0x9B, 0x96, 0x00, 0xFF, 0x9B, 0x96,
    0x00, 0xFF, 0x9B, 0x96, 0x00, 0xFF, 0x99, 0x94, 0x00, 0xFF, 0x97, 0x92, 0x00, 0xFF, 0x98, 0x93,
    0x00, 0xFF, 0xA0, 0x9A, 0x00, 0xFF, 0x9B, 0x95, 0x00, 0xB6, 0x00, 0x00, 0x00, 0x00, 0x99, 0x94,
    0x00, 0x36, 0xA3, 0x9E, 0x00, 0xFF, 0x93, 0x8D, 0x00, 0xFF, 0xB2, 0xAE, 0x54, 0xFF, 0xC6, 0xC4,
    0x89, 0xFF, 0xB4, 0xB1, 0x5B, 0xFF, 0x93, 0x8D, 0x00, 0xFF, 0x9A, 0x95, 0x00, 0xFF, 0x9A, 0x95,
    0x00, 0xFF, 0x93, 0x8D, 0x00, 0xFF, 0xAC, 0xA8, 0x42, 0xFF, 0xC6, 0xC3, 0x88, 0xFF, 0xB9, 0xB6,
    0x69, 0xFF, 0x94, 0x8F, 0x00, 0xFF, 0xA2, 0x9C, 0x00, 0xFF, 0x99, 0x94, 0x00, 0x36, 0x9A, 0x94,
    0x00, 0xAE, 0x9B, 0x95, 0x00, 0xFF, 0xEF, 0xEF, 0xDC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF3, 0xF3,
    0xE0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0xF6, 0xEB, 0xFF, 0x9F, 0x9A, 0x0E, 0xFF, 0x95, 0x8F,
    0x00, 0xFF, 0xE9, 0xE8, 0xCE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF4, 0xF3, 0xE2, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFC, 0xFC, 0xF7, 0xFF, 0xA5, 0xA0, 0x1D, 0xFF, 0x99, 0x93, 0x00, 0xAE, 0x94, 0x8F,
    0x00, 0xDE, 0xDE, 0xDD, 0xB8, 0xFF, 0xF7, 0xF6, 0xEC, 0xFF, 0x96, 0x91, 0x00, 0xFF, 0x8F, 0x89,
    0x00, 0xFF, 0x93, 0x8D, 0x00, 0xFF, 0xE8, 0xE7, 0xCF, 0xFF, 0xF9, 0xF9, 0xF1, 0xFF, 0xE8, 0xE7,
    0xCE, 0xFF, 0xF8, 0xF7, 0xEE, 0xFF, 0x9B, 0x96, 0x00, 0xFF, 0x94, 0x8E, 0x00, 0xFF, 0x8F, 0x89,
    0x00, 0xFF, 0xE6, 0xE5, 0xCB, 0xFF, 0xF5, 0xF4, 0xE4, 0xFF, 0x90, 0x8A, 0x00, 0xDE, 0x92, 0x8D,
    0x00, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xB5, 0xB2, 0x60, 0xFF, 0xAB, 0xA7, 0x42, 0xFF, 0xCA, 0xC8,
    0x91, 0xFF, 0xB9, 0xB5, 0x6A, 0xFF, 0x8E, 0x89, 0x00, 0xFF, 0xF3, 0xF2, 0xE4, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0x98, 0x93, 0x00, 0xFF, 0xAC, 0xA7, 0x42, 0xFF, 0xF9, 0xF9, 0xEC, 0xFF, 0xB3, 0xB0,
    0x5C, 0xFF, 0x9A, 0x95, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x9F, 0x9A, 0x00, 0xFC, 0x93, 0x8D,
    0x00, 0xFC, 0xFF, 0xFF, 0xFD, 0xFF, 0xB9, 0xB5, 0x67, 0xFF, 0xA6, 0xA1, 0x33, 0xFF, 0xBF, 0xBC,
    0x7B, 0xFF, 0xB0, 0xAD, 0x58, 0xFF, 0x92, 0x8C, 0x00, 0xFF, 0xF8, 0xF7, 0xEE, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0x9E, 0x99, 0x04, 0xFF, 0xA4, 0xA0, 0x30, 0xFF, 0xF6, 0xF5, 0xE3, 0xFF, 0xAB, 0xA7,
    0x48, 0xFF, 0x9E, 0x9A, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x9D, 0x98, 0x00, 0xFC, 0x95, 0x8F,
    0x00, 0xDE, 0xD9, 0xD7, 0xAC, 0xFF, 0xFD, 0xFD, 0xF8, 0xFF, 0x9F, 0x9A, 0x0B, 0xFF, 0x8E, 0x87,
    0x00, 0xFF, 0x9B, 0x96, 0x00, 0xFF, 0xF1, 0xF0, 0xDF, 0xFF, 0xF3, 0xF3, 0xE5, 0xFF, 0xE0, 0xDE,
    0xBD, 0xFF, 0xFE, 0xFE, 0xFB, 0xFF, 0xA7, 0xA2, 0x2D, 0xFF, 0x8B, 0x84, 0x00, 0xFF, 0x96, 0x90,
    0x00, 0xFF, 0xEE, 0xED, 0xDB, 0xFF, 0xEF, 0xEE, 0xD8, 0xFF, 0x91, 0x8C, 0x00, 0xDE, 0x9A, 0x94,
    0x00, 0xAE, 0x98, 0x92, 0x00, 0xFF, 0xE5, 0xE3, 0xC5, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFE,
    0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xED, 0xEC, 0xD6, 0xFF, 0x9A, 0x94, 0x00, 0xFF, 0x93, 0x8D,
    0x00, 0xFF, 0xDD, 0xDB, 0xB7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFA, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xF3, 0xF3, 0xE4, 0xFF, 0x9F, 0x9A, 0x00, 0xFF, 0x99, 0x94, 0x00, 0xAE, 0x99, 0x94,
    0x00, 0x36, 0xA3, 0x9D, 0x00, 0xFF, 0x94, 0x8E, 0x00, 0xFF, 0xA5, 0xA0, 0x24, 0xFF, 0xBB, 0xB7,
    0x72, 0xFF, 0xA7, 0xA3, 0x2E, 0xFF, 0x93, 0x8D, 0x00, 0xFF, 0x9A, 0x95, 0x00, 0xFF, 0x9B, 0x95,
    0x00, 0xFF, 0x95, 0x8F, 0x00, 0xFF, 0xA0, 0x9B, 0x0C, 0xFF, 0xBA, 0xB7, 0x70, 0xFF, 0xAC, 0xA8,
    0x40, 0xFF, 0x93, 0x8D, 0x00, 0xFF, 0xA2, 0x9D, 0x00, 0xFF, 0x99, 0x94, 0x00, 0x36, 0x00, 0x00,
    0x00, 0x00, 0x9B, 0x95, 0x00, 0xB6, 0xA0, 0x9A, 0x00, 0xFF, 0x9A, 0x94, 0x00, 0xFF, 0x98, 0x93,
    0x00, 0xFF, 0x99, 0x94, 0x00, 0xFF, 0x9B, 0x95, 0x00, 0xFF, 0x9B, 0x96, 0x00, 0xFF, 0x9B, 0x96,
    0x00, 0xFF, 0x9B, 0x96, 0x00, 0xFF, 0x9A, 0x95, 0x00, 0xFF, 0x98, 0x93, 0x00, 0xFF, 0x99, 0x94,
    0x00, 0xFF, 0xA0, 0x9A, 0x00, 0xFF, 0x9B, 0x95, 0x00, 0xB6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9B, 0x95, 0x00, 0xDC, 0xA0, 0x9A, 0x00, 0xFF, 0x9B, 0x95,
    0x00, 0xFF, 0x9B, 0x95, 0x00, 0xFF, 0x9B, 0x96, 0x00, 0xFF, 0x9A, 0x96, 0x00, 0xFF, 0x9A, 0x96,
    0x00, 0xFF, 0x9B, 0x96, 0x00, 0xFF, 0x9B, 0x96, 0x00, 0xFF, 0x9B, 0x96, 0x00, 0xFF, 0xA0, 0x9B,
    0x00, 0xFF, 0x9B, 0x95, 0x00, 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9B, 0x95, 0x00, 0xB6, 0xA3, 0x9E,
    0x00, 0xFF, 0x9E, 0x99, 0x00, 0xFF, 0x9C, 0x97, 0x00, 0xFF, 0x9B, 0x96, 0x00, 0xFF, 0x9B, 0x96,
    0x00, 0xFF, 0x9C, 0x97, 0x00, 0xFF, 0x9E, 0x99, 0x00, 0xFF, 0xA3, 0x9E, 0x00, 0xFF, 0x9B, 0x95,
    0x00, 0xB6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x99, 0x94,
    0x00, 0x36, 0x9B, 0x96, 0x00, 0xAE, 0x9A, 0x95, 0x00, 0xDE, 0x9B, 0x95, 0x00, 0xFC, 0x9B, 0x95,
    0x00, 0xFC, 0x9A, 0x95, 0x00, 0xDE, 0x9A, 0x96, 0x00, 0xAE, 0x99, 0x94, 0x00, 0x36, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x1F,
    0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x80, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x80, 0x01,
    0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0xF8, 0x1F, 0x00, 0x00 };
  uint8_t* pfavicon = (uint8_t*)favicon;
  uint8_t buffer[BUF_SIZE];
  uint16_t bytesRemain = sizeof(favicon);
  uint16_t chunkSize;

  if (! startResponse(200, FPSTR(typeImageIcon), bytesRemain))
    return;

  while (bytesRemain) {
    chunkSize = min(BUF_SIZE, bytesRemain);
    memcpy_P(buffer, pfavicon, chunkSize);
    if (_client->write(buffer, chunkSize) != chunkSize) {
      return;
    }
    pfavicon += chunkSize;
    bytesRemain -= chunkSize;
  }
  endResponse();
}

void EthWebServerApp::handleJson() {
  if (! startResponse(200, FPSTR(typeTextJson)))
    return;

  _client->write('{');
  if (hasArg(FPSTR(paramSrc)))
    handleJsonData(atoi(arg(FPSTR(paramSrc))));
  _client->write('}');
  endResponse();
}

void EthWebServerApp::handleJsonData(uint8_t src) {
  if (src == 0) { // JSON packet for root page
    printf(*_client, "\"%S\":%u", FPSTR(jsonFreeMem), getFreeMem());
    printf(*_client, ",\"%S\":%lu", FPSTR(jsonUptime), millis() / 1000);
#ifdef USEMQTT
    _client->print(F(",\""));
    _client->print(FPSTR(jsonMQTTConnected));
    _client->print(F("\":"));
    if (_pubSubClient && _pubSubClient->connected())
      _client->print(FPSTR(strTrue));
    else
      _client->print(FPSTR(strFalse));
#endif
  } else if (src == 1) { // JSON packet for time setup page
    uint32_t now = getTime();
    char str[20];

    printf(*_client, F("\"%S\":%lu"), FPSTR(jsonUnixTime), now);
    printf(*_client, F(",\"%S\":\""), FPSTR(jsonDate));
    if (now) {
      dateTimeToStr(str, now);
      str[10] = charNull;
      _client->print(str);
    }
    printf(*_client, F("\",\"%S\":\""), FPSTR(jsonTime));
    if (now)
      _client->print(&str[11]);
    _client->write('\"');
  }
}

void EthWebServerApp::handleStore() {
  for (uint8_t i = 0; i < args(); ++i) {
    setParameter(argName(i), arg(i));
  }
  if (! writeParams()) {
    _log.println(F("Error writing config to EEPROM!"));
  } else {
    _log.println(F("Config succefully stored to EEPROM"));
  }

  if (! startResponse(200, FPSTR(typeTextHtml)))
    return;

  htmlPageStart(F("Store Config"));
  _client->print(F("<meta http-equiv=\"refresh\" content=\"5;URL=/\">\n"));
  htmlPageStdStyle();
  htmlPageBody();
  _client->print(F("Configuration stored successfully.</br>\n"));
  if (*arg(FPSTR(paramReboot)) == '1') {
    _client->print(F("<i>You must reboot module to apply new configuration!</i>\n"));
  }
  _client->print(F("<p>\n\
Wait for 5 sec. or click <a href=\"/\">this</a> to return to main page.\n"));
  htmlPageEnd();
  endResponse();
}

static char* byteToHex(char* str, uint8_t val) {
  uint8_t d = val / 16;

  if (d > 9)
    str[0] = 'A' + d - 10;
  else
    str[0] = '0' + d;
  d = val % 16;
  if (d > 9)
    str[1] = 'A' + d - 10;
  else
    str[1] = '0' + d;
  str[2] = charNull;

  return str;
}

void EthWebServerApp::handleEthernetConfig() {
  static const char codePattern1[] PROGMEM = "form.%S%d.";
  static const char codePattern2[] PROGMEM = "form.%S.value=form.%S0.value|(form.%S1.value<<8)|(form.%S2.value<<16)|(form.%S3.value<<24);\n";
  static const char codePattern3[] PROGMEM = "disabled=true;";
  static const char codePattern4[] PROGMEM = "onblur=\"checkMAC(this)\"";
  static const char codePattern5[] PROGMEM = "onblur=\"checkNumber(this,0,255)\"";

  uint8_t i;

#ifdef USEAUTHORIZATION
  if (! authenticate(authUser, authPassword))
    return requestAuthentication();
#endif

  if (! startResponse(200, FPSTR(typeTextHtml)))
    return;

  htmlPageStart(F("Ethernet Setup"));
  htmlPageStdStyle();
  htmlPageScriptStart();
  printf(*_client, F("function completeForm(form){\n\
form.%S.value="), FPSTR(paramMAC));
  for (i = 0; i < 6; ++i) {
    if (i)
      _client->write('+');
    printf(*_client, FPSTR(codePattern1), FPSTR(paramMAC), i);
    _client->print(F("value"));
  }
  _client->print(F(";\n"));
  printf(*_client, FPSTR(codePattern2), FPSTR(paramIP), FPSTR(paramIP), FPSTR(paramIP), FPSTR(paramIP), FPSTR(paramIP));
  printf(*_client, FPSTR(codePattern2), FPSTR(paramMask), FPSTR(paramMask), FPSTR(paramMask), FPSTR(paramMask), FPSTR(paramMask));
  printf(*_client, FPSTR(codePattern2), FPSTR(paramGate), FPSTR(paramGate), FPSTR(paramGate), FPSTR(paramGate), FPSTR(paramGate));
  printf(*_client, FPSTR(codePattern2), FPSTR(paramDNS), FPSTR(paramDNS), FPSTR(paramDNS), FPSTR(paramDNS), FPSTR(paramDNS));
  for (i = 0; i < 6; ++i) {
    printf(*_client, FPSTR(codePattern1), FPSTR(paramMAC), i);
    _client->print(FPSTR(codePattern3));
  }
  _client->write(charLF);
  for (i = 0; i < 4; ++i) {
    printf(*_client, FPSTR(codePattern1), FPSTR(paramIP), i);
    _client->print(FPSTR(codePattern3));
  }
  _client->write(charLF);
  for (i = 0; i < 4; ++i) {
    printf(*_client, FPSTR(codePattern1), FPSTR(paramMask), i);
    _client->print(FPSTR(codePattern3));
  }
  _client->write(charLF);
  for (i = 0; i < 4; ++i) {
    printf(*_client, FPSTR(codePattern1), FPSTR(paramGate), i);
    _client->print(FPSTR(codePattern3));
  }
  _client->write(charLF);
  for (i = 0; i < 4; ++i) {
    printf(*_client, FPSTR(codePattern1), FPSTR(paramDNS), i);
    _client->print(FPSTR(codePattern3));
  }
  _client->print(F("\n}\n\
function checkMAC(src){\n\
var value = src.value;\n\
value=value.toUpperCase();\n\
if(value.length == 2){\n\
if((value.charAt(0)<'0')||((value.charAt(0)>'9')&&((value.charAt(0)<'A')||(value.charAt(0)>'F'))))\n\
value='0'+value.charAt(1);\n\
if((value.charAt(1)<'0')||((value.charAt(1)>'9')&&((value.charAt(1)<'A')||(value.charAt(1)>'F'))))\n\
value=value.charAt(0)+'0';\n\
}else\n\
value=\"00\";\n\
src.value=value;\n\
}\n\
function checkNumber(src,minval,maxval){\n\
var value=parseInt(src.value);\n\
if(isNaN(value))\n\
value=minval;\n\
if(value<minval)\n\
value=minval;\n\
if(value>maxval)\n\
value=maxval;\n\
src.value=value.toString();\n\
}\n"));
  htmlPageScriptEnd();
  htmlPageBody();
  printf(*_client, F("<form name=\"ethernet\" action=\"%S\" method=\"GET\" onsubmit=\"completeForm(this)\">\n"), FPSTR(pathStore));
  htmlTag(FPSTR(tagH3), F("Ethernet Setup"), true);
  htmlTag(FPSTR(tagLabel), F("MAC address:"));
  htmlTagBR(true);
  {
    char name[sizeof(paramMAC) + 1];
    char value[3];

    strcpy_P(name, paramMAC);
    name[sizeof(name) - 1] = charNull;
    for (i = 0; i < 6; ++i) {
      name[sizeof(name) - 2] = '0' + i;
      byteToHex(value, mac[i]);
      htmlTagInputText(name, value, 2, 2, FPSTR(codePattern4), false);
      if (i < 5)
        _client->print(F(" :\n"));
    }
  }
  htmlTagBR(true);
  htmlTagInput(FPSTR(inputTypeHidden), FPSTR(paramMAC), FPSTR(strEmpty), true);
  htmlTag(FPSTR(tagLabel), F("IP address:"));
  htmlTagBR(true);
  {
    char name[sizeof(paramIP) + 1];
    char value[4];

    strcpy_P(name, paramIP);
    name[sizeof(name) - 1] = charNull;
    for (i = 0; i < 4; ++i) {
      name[sizeof(name) - 2] = '0' + i;
      itoa((ip >> (i * 8)) & 0xFF, value, 10);
      htmlTagInputText(name, value, 3, 3, FPSTR(codePattern5), false);
      if (i < 3)
        _client->print(F(" .\n"));
    }
  }
  _client->print(F("\n(0.0.0.0 for DHCP)"));
  htmlTagBR(true);
  htmlTagInput(FPSTR(inputTypeHidden), FPSTR(paramIP), FPSTR(strEmpty), true);
  htmlTag(FPSTR(tagLabel), F("Submet mask:"));
  htmlTagBR(true);
  {
    char name[sizeof(paramMask) + 1];
    char value[4];

    strcpy_P(name, paramMask);
    name[sizeof(name) - 1] = charNull;
    for (i = 0; i < 4; ++i) {
      name[sizeof(name) - 2] = '0' + i;
      itoa((mask >> (i * 8)) & 0xFF, value, 10);
      htmlTagInputText(name, value, 3, 3, FPSTR(codePattern5), false);
      if (i < 3)
        _client->print(F(" .\n"));
    }
  }
  htmlTagBR(true);
  htmlTagInput(FPSTR(inputTypeHidden), FPSTR(paramMask), FPSTR(strEmpty), true);
  htmlTag(FPSTR(tagLabel), F("Gateway address:"));
  htmlTagBR(true);
  {
    char name[sizeof(paramGate) + 1];
    char value[4];

    strcpy_P(name, paramGate);
    name[sizeof(name) - 1] = charNull;
    for (i = 0; i < 4; ++i) {
      name[sizeof(name) - 2] = '0' + i;
      itoa((gate >> (i * 8)) & 0xFF, value, 10);
      htmlTagInputText(name, value, 3, 3, FPSTR(codePattern5), false);
      if (i < 3)
        _client->print(F(" .\n"));
    }
  }
  htmlTagBR(true);
  htmlTagInput(FPSTR(inputTypeHidden), FPSTR(paramGate), FPSTR(strEmpty), true);
  htmlTag(FPSTR(tagLabel), F("DNS address:"));
  htmlTagBR(true);
  {
    char name[sizeof(paramDNS) + 1];
    char value[4];

    strcpy_P(name, paramDNS);
    name[sizeof(name) - 1] = charNull;
    for (i = 0; i < 4; ++i) {
      name[sizeof(name) - 2] = '0' + i;
      itoa((dns >> (i * 8)) & 0xFF, value, 10);
      htmlTagInputText(name, value, 3, 3, FPSTR(codePattern5), false);
      if (i < 3)
        _client->print(F(" .\n"));
    }
  }
  htmlTagBR(true);
  htmlTagInput(FPSTR(inputTypeHidden), FPSTR(paramDNS), FPSTR(strEmpty), true);
  htmlTag(FPSTR(tagLabel), F("DHCP maintenance interval:"));
  htmlTagBR(true);
  {
    char value[6];

    utoa(maintenanceInterval, value, 10);
    htmlTagInputText(FPSTR(paramMaintenance), value, 5, 5, F("onblur=\"checkNumber(this,0,65535)\""), true);
  }
  _client->print(F("min. (0 to disable)\n"));
#ifdef USEAUTHORIZATION
  htmlTagSimple(FPSTR(tagP), true);
  htmlTag(FPSTR(tagLabel), F("Authorization user:"));
  htmlTagBR(true);
  htmlTagInputText(FPSTR(paramAuthUser), authUser, sizeof(authUser) - 1, sizeof(authUser) - 1, true);
  htmlTagBR(true);
  htmlTag(FPSTR(tagLabel), F("Authorization password:"));
  htmlTagBR(true);
  {
    char extra[32];

    sprintf(extra, sizeof(extra), F("size=%d maxlength=%d"), sizeof(authPassword) - 1, sizeof(authPassword) - 1);
    htmlTagInput(FPSTR(inputTypePassword), FPSTR(paramAuthPassword), authPassword, extra, true);
  }
#endif
  htmlTagInput(FPSTR(inputTypeHidden), FPSTR(paramReboot), F("1"), true);
  htmlTagSimple(FPSTR(tagP), true);
  htmlTagInput(FPSTR(inputTypeSubmit), FPSTR(NULL), F("Save"), true);
  btnBack();
  _client->print(F("</form>\n"));
  htmlPageEnd();
  endResponse();
}

void EthWebServerApp::handleTimeConfig() {
  static const char codePattern1[] PROGMEM = "document.getElementById('%S').innerHTML=";
  static const char codePattern2[] PROGMEM = "data.%S;\n";

#ifdef USEAUTHORIZATION
  if (! authenticate(authUser, authPassword))
    return requestAuthentication();
#endif

  if (! startResponse(200, FPSTR(typeTextHtml)))
    return;

  htmlPageStart(F("Time Setup"));
  htmlPageStdStyle();
  htmlPageScriptStart();
  htmlPageStdScript(false);
  printf(*_client, F("function updateTime(){\n\
openUrl('%S?time='+Math.floor(Date.now()/1000)+'&dummy='+Date.now());\n\
}\n"), FPSTR(pathSetTime));
  printf(*_client, F("function refreshData(){\n\
var request=getXmlHttpRequest();\n\
request.open('GET','%S?%S=1&dummy='+Date.now(),true);\n\
request.onreadystatechange=function(){\n\
if(request.readyState==4){\n\
var data=JSON.parse(request.responseText);\n"), FPSTR(pathJson), FPSTR(paramSrc));
  printf(*_client, F("if(data.%S==0){\n"), FPSTR(jsonUnixTime));
  printf(*_client, FPSTR(codePattern1), FPSTR(jsonDate));
  _client->print(F("\"Unset\";\n"));
  printf(*_client, FPSTR(codePattern1), FPSTR(jsonTime));
  _client->print(F("\"\";\n\
}else{\n"));
  printf(*_client, FPSTR(codePattern1), FPSTR(jsonDate));
  printf(*_client, FPSTR(codePattern2), FPSTR(jsonDate));
  printf(*_client, FPSTR(codePattern1), FPSTR(jsonTime));
  printf(*_client, FPSTR(codePattern2), FPSTR(jsonTime));
  _client->print(F("}\n\
}\n\
}\n\
request.send(null);\n\
}\n\
function checkNumber(src,minval,maxval){\n\
var value=parseInt(src.value);\n\
if(isNaN(value))\n\
value=minval;\n\
if(value<minval)\n\
value=minval;\n\
if(value>maxval)\n\
value=maxval;\n\
src.value=value.toString();\n\
}\n\
setInterval(refreshData,1000);\n"));
  htmlPageScriptEnd();
  htmlPageBody();
  printf(*_client, F("<form name=\"time\" action=\"%S\" method=\"GET\">\n"), FPSTR(pathStore));
  htmlTag(FPSTR(tagH3), F("Time Setup"), true);
  printf(*_client, F("Current date and time: <span id=\"%S\"></span> <span id=\"%S\"></span>\n\
<p>\n"), FPSTR(jsonDate), FPSTR(jsonTime));
  htmlTag(FPSTR(tagLabel), F("NTP server:"));
  htmlTagBR(true);
  htmlTagInputText(FPSTR(paramNtpServer), ntpServer, sizeof(ntpServer) - 1, sizeof(ntpServer) - 1, true);
  htmlTagBR(true);
  htmlTag(FPSTR(tagLabel), F("Time zone:"));
  htmlTagBR(true);
  printf(*_client, F("<select name=\"%S\" size=1>\n"), FPSTR(paramTimeZone));
  for (int8_t i = -12; i <= 14; ++i) {
    printf(*_client, F("<option value=\"%d\""), i);
    if (timeZone == i)
      _client->print(F(" selected"));
    _client->print(F(">GMT"));
    if (i > 0)
      _client->write('+');
    if (i)
      _client->print(i);
    _client->print(F("</option>\n"));
  }
  _client->print(F("</select>"));
  htmlTagBR(true);
  htmlTag(FPSTR(tagLabel), F("Update interval:"));
  htmlTagBR(true);
  {
    char value[6];

    utoa(updateInterval, value, 10);
    htmlTagInputText(FPSTR(paramUpdateInterval), value, 5, 5, F("onblur=\"checkNumber(this,0,65535)\""), true);
  }
  _client->print(F("min. (0 to disable)\n"));
  htmlTagInput(FPSTR(inputTypeHidden), FPSTR(paramReboot), F("0"), true);
  htmlTagSimple(FPSTR(tagP), true);
  htmlTagInput(FPSTR(inputTypeButton), FPSTR(NULL), F("Update time from browser"), F("onclick=\"updateTime()\""), true);
  htmlTagSimple(FPSTR(tagP), true);
  htmlTagInput(FPSTR(inputTypeSubmit), FPSTR(NULL), F("Save"), true);
  btnBack();
  _client->print(F("</form>\n"));
  htmlPageEnd();
  endResponse();
}

void EthWebServerApp::handleSetTime() {
  if (hasArg(FPSTR(paramTime))) {
    setTime(atol(arg(FPSTR(paramTime))) + timeZone * 3600);

    send(200, FPSTR(typeTextPlain), FPSTR(strEmpty));
  }
}

void EthWebServerApp::handleLog() {
  if (! startResponse(200, FPSTR(typeTextHtml)))
    return;

  htmlPageStart(F("Log View"));
  _client->print(F("<meta http-equiv=\"refresh\" content=\"5;URL=\">\n"));
  htmlPageStdStyle();
  htmlPageStdScript();
  htmlPageBody();
  htmlTag(FPSTR(tagH3), F("Log View"), true);
  _client->print(F("<textarea cols=80 rows=25 readonly>\n"));
  _log.printEncodedTo(*_client);
  _client->print(F("</textarea>\n"));
  htmlTagSimple(FPSTR(tagP), true);
  {
    char extra[80];

    htmlTagInput(FPSTR(inputTypeButton), FPSTR(NULL), F("Clear!"), sprintf(extra, sizeof(extra), F("onclick=\"if(confirm('Are you sure to clear log?')) openUrl('%S')\""), FPSTR(pathClearLog)), true);
  }
  btnBack();
  htmlPageEnd();
  endResponse();
}

void EthWebServerApp::handleClearLog() {
  _log.clear();

  send(200, FPSTR(typeTextPlain), FPSTR(strEmpty));
}

void EthWebServerApp::handleRestart() {
  if (! startResponse(200, FPSTR(typeTextHtml)))
    return;

  htmlPageStart(F("Reboot"));
  _client->print(F("<meta http-equiv=\"refresh\" content=\"5;URL=/\">\n"));
  htmlPageStdStyle();
  htmlPageBody();
  _client->print(F("Rebooting...\n"));
  htmlPageEnd();
  endResponse();

  restart();
}

#ifdef USESDCARD
void EthWebServerApp::handleSDCard() {
  if (! startResponse(200, FPSTR(typeTextHtml)))
    return;

  htmlPageStart(F("SD card"));
  htmlPageStdStyle();
  htmlPageStdScript();
  htmlPageBody();
  if (_haveSD) {
    char rootDir[2];
    uint16_t cnt = 0;
    File root, entry;

    htmlTag(FPSTR(tagH3), F("SD card"), true);
    rootDir[0] = charSlash;
    rootDir[1] = charNull;
    root = SD.open(rootDir); // "/"
    if (root) {
      htmlTagStart(FPSTR(tagTable), true);
      while (entry =  root.openNextFile()) {
        htmlTagStart(FPSTR(tagTR));
        htmlTag(FPSTR(tagTD), entry.name());
        {
          char sizeStr[12];
  
          htmlTag(FPSTR(tagTD), ltoa(entry.size(), sizeStr, 10));
        }
        htmlTagEnd(FPSTR(tagTR), true);
        entry.close();
        ++cnt;
      }
      root.close();
      htmlTagEnd(FPSTR(tagTable), true);
    }
    if (cnt)
      printf(*_client, F("%u file(s) on SD card\n"), cnt);
    else
      _client->print(F("No files found on SD card\n"));
  } else {
    htmlTag(FPSTR(tagH3), F("SD card is not present!"), true);
  }
  htmlTagSimple(FPSTR(tagP), true);
  btnBack();
  htmlPageEnd();
  endResponse();
}

bool EthWebServerApp::handleFile() {
  char fileName[13];

  if ((*_uri == '/') && (! _uri[1])) { // "/"
    strncpy_P(fileName, &pathRoot[1], sizeof(pathRoot) - 3); // "index.htm"
  } else {
    strncpy(fileName, &_uri[1], sizeof(fileName) - 1);
  }
  fileName[sizeof(fileName) - 1] = charNull;

  if (! SD.exists(fileName)) {
    printf(_log, F("File \"%s\" not found on SD card!\n"), fileName);
    return false;
  }

  File file = SD.open(fileName);
  if (! file) {
    printf(_log, F("Error open file \"%s\"!\n"), fileName);
    return false;
  }

  PGM_P type;

  if (hasHeader(F("download")))
    type = PSTR("application/octet-stream");
  else if (endsWith_P(_uri, PSTR(".htm")))
    type = typeTextHtml;
  else if (endsWith_P(_uri, PSTR(".css")))
    type = typeTextCss;
  else if (endsWith_P(_uri, PSTR(".js")))
    type = typeApplicationJavascript;
  else if (endsWith_P(_uri, PSTR(".png")))
    type = PSTR("image/png");
  else if (endsWith_P(_uri, PSTR(".gif")))
    type = PSTR("image/gif");
  else if (endsWith_P(_uri, PSTR(".jpg")))
    type = PSTR("image/jpeg");
  else if (endsWith_P(_uri, PSTR(".ico")))
    type = typeImageIcon;
  else if (endsWith_P(_uri, PSTR(".xml")))
    type = PSTR("text/xml");
  else if (endsWith_P(_uri, PSTR(".zip")))
    type = PSTR("application/x-zip");
  else if (endsWith_P(_uri, PSTR(".gz")))
    type = PSTR("application/x-gzip");
  else
    type = typeTextPlain;

  if ((! startResponse(200, FPSTR(type), file.size())) || (! sendFile(file)) || (! endResponse())) {
    file.close();
    return false;
  }
  file.close();

  return true;
}
#endif

#ifdef USEMQTT
void EthWebServerApp::handleMQTTConfig() {
#ifdef USEAUTHORIZATION
  if (! authenticate(authUser, authPassword))
    return requestAuthentication();
#endif

  if (! startResponse(200, FPSTR(typeTextHtml)))
    return;

  htmlPageStart(F("MQTT Setup"));
  htmlPageStdStyle();
  htmlPageScriptStart();
  htmlPageStdScript(false);
  _client->print(F("function checkNumber(src,minval,maxval){\n\
var value=parseInt(src.value);\n\
if(isNaN(value))\n\
value=minval;\n\
if(value<minval)\n\
value=minval;\n\
if(value>maxval)\n\
value=maxval;\n\
src.value=value.toString();\n\
}\n"));
  htmlPageScriptEnd();
  htmlPageBody();
  printf(*_client, F("<form name=\"mqtt\" action=\"%S\" method=\"GET\">\n"), FPSTR(pathStore));
  htmlTag(FPSTR(tagH3), F("MQTT Setup"), true);
  htmlTag(FPSTR(tagLabel), F("MQTT broker:"));
  htmlTagBR(true);
  htmlTagInputText(FPSTR(paramMQTTServer), mqttServer, sizeof(mqttServer) - 1, sizeof(mqttServer) - 1, true);
  _client->print(F("(leave blank to disable MQTT)"));
  htmlTagBR(true);
  htmlTag(FPSTR(tagLabel), F("Broker port:"));
  htmlTagBR(true);
  {
    char value[6];

    utoa(mqttPort, value, 10);
    htmlTagInputText(FPSTR(paramMQTTPort), value, 5, 5, F("onblur=\"checkNumber(this,0,65535)\""), true);
  }
  htmlTagBR(true);
  htmlTag(FPSTR(tagLabel), F("MQTT user name:"));
  htmlTagBR(true);
  htmlTagInputText(FPSTR(paramMQTTUser), mqttUser, sizeof(mqttUser) - 1, sizeof(mqttUser) - 1, true);
  _client->print(F("(leave blank if authorization is not used)"));
  htmlTagBR(true);
  htmlTag(FPSTR(tagLabel), F("MQTT password:"));
  htmlTagBR(true);
  {
    char extra[32];

    sprintf(extra, sizeof(extra), F("size=%d maxlength=%d"), sizeof(mqttPassword) - 1, sizeof(mqttPassword) - 1);
    htmlTagInput(FPSTR(inputTypePassword), FPSTR(paramMQTTPassword), mqttPassword, extra, true);
  }
  htmlTagBR(true);
  htmlTag(FPSTR(tagLabel), F("MQTT client name:"));
  htmlTagBR(true);
  htmlTagInputText(FPSTR(paramMQTTClient), mqttClient, sizeof(mqttClient) - 1, sizeof(mqttClient) - 1, true);
  htmlTagInput(FPSTR(inputTypeHidden), FPSTR(paramReboot), F("1"), true);
  htmlTagSimple(FPSTR(tagP), true);
  htmlTagInput(FPSTR(inputTypeSubmit), FPSTR(NULL), F("Save"), true);
  btnBack();
  _client->print(F("</form>\n"));
  htmlPageEnd();
  endResponse();
}
#endif

void EthWebServerApp::btnNavigator() {
  htmlTagSimple(FPSTR(tagP), true);
  btnEthernet();
  btnTime();
#ifdef USEMQTT
  btnMQTT();
#endif
  btnLog();
  btnRestart();
}

void EthWebServerApp::btnToGo(PGM_P caption, PGM_P path) {
  char extra[80];

  htmlTagInput(FPSTR(inputTypeButton), FPSTR(NULL), FPSTR(caption), sprintf(extra, sizeof(extra), F("onclick=\"location.href='%S'\""), FPSTR(path)), true);
}

void EthWebServerApp::btnBack() {
  btnToGo(PSTR("Back"), strSlash);
}

void EthWebServerApp::btnEthernet() {
  btnToGo(PSTR("Ethernet"), pathEthernetConfig);
}

void EthWebServerApp::btnTime() {
  btnToGo(PSTR("Time"), pathTimeConfig);
}

void EthWebServerApp::btnLog() {
  btnToGo(PSTR("Log"), pathLog);
}

void EthWebServerApp::btnRestart() {
  char extra[80];

  htmlTagInput(FPSTR(inputTypeButton), FPSTR(NULL), F("Restart!"), sprintf(extra, sizeof(extra), F("onclick=\"if(confirm('Are you sure to restart?')) location.href='%S'\""), FPSTR(pathRestart)), true);
}

#ifdef USEMQTT
void EthWebServerApp::btnMQTT() {
  btnToGo(PSTR("MQTT"), pathMQTTConfig);
}
#endif

const char* EthWebServerApp::arg(const char* name) {
  for (uint8_t i = 0; i < _argCount; ++i) {
    if (! strcmp(_args[i].name, name))
      return _args[i].value;
  }

  return NULL;
}

const char* EthWebServerApp::arg(const __FlashStringHelper* name) {
  for (uint8_t i = 0; i < _argCount; ++i) {
    if (! strcmp_P(_args[i].name, (PGM_P)name))
      return _args[i].value;
  }

  return NULL;
}

const char* EthWebServerApp::arg(uint8_t ind) {
  if (ind < _argCount)
    return _args[ind].value;
  else
    return NULL;
}

const char* EthWebServerApp::argName(uint8_t ind) {
  if (ind < _argCount)
    return _args[ind].name;
  else
    return NULL;
}

bool EthWebServerApp::hasArg(const char* name) {
  for (uint8_t i = 0; i < _argCount; ++i) {
    if (! strcmp(_args[i].name, name))
      return true;
  }

  return false;
}

bool EthWebServerApp::hasArg(const __FlashStringHelper* name) {
  for (uint8_t i = 0; i < _argCount; ++i) {
    if (! strcmp_P(_args[i].name, (PGM_P)name))
      return true;
  }

  return false;
}

const char* EthWebServerApp::header(const char* name) {
  for (uint8_t i = 0; i < _headerCount; ++i) {
    if (! strcmp(_headers[i].name, name))
      return _headers[i].value;
  }

  return NULL;
}

const char* EthWebServerApp::header(const __FlashStringHelper* name) {
  for (uint8_t i = 0; i < _headerCount; ++i) {
    if (! strcmp_P(_headers[i].name, (PGM_P)name))
      return _headers[i].value;
  }

  return NULL;
}

const char* EthWebServerApp::header(uint8_t ind) {
  if (ind < _headerCount)
    return _headers[ind].value;
  else
    return NULL;
}

const char* EthWebServerApp::headerName(uint8_t ind) {
  if (ind < _headerCount)
    return _headers[ind].name;
  else
    return NULL;
}

bool EthWebServerApp::hasHeader(const char* name) {
  for (uint8_t i = 0; i < _headerCount; ++i) {
    if (! strcmp(_headers[i].name, name))
      return true;
  }

  return false;
}

bool EthWebServerApp::hasHeader(const __FlashStringHelper* name) {
  for (uint8_t i = 0; i < _headerCount; ++i) {
    if (! strcmp_P(_headers[i].name, (PGM_P)name))
      return true;
  }

  return false;
}

bool EthWebServerApp::send(int16_t code, const char* contentType, const char* content) {
  return (startResponse(code, contentType, strlen(content)) && sendContent(content) && endResponse());
}

bool EthWebServerApp::send(int16_t code, const __FlashStringHelper* contentType, const __FlashStringHelper* content) {
  return (startResponse(code, FPSTR(contentType), strlen_P((PGM_P)content)) && sendContent(content) && endResponse());
}

bool EthWebServerApp::send(int16_t code, const char* contentType, const __FlashStringHelper* content) {
  return (startResponse(code, contentType, strlen_P((PGM_P)content)) && sendContent(content) && endResponse());
}

bool EthWebServerApp::send(int16_t code, const __FlashStringHelper* contentType, const char* content) {
  return (startResponse(code, FPSTR(contentType), strlen(content)) && sendContent(content) && endResponse());
}

#ifdef USESDCARD
bool EthWebServerApp::sendFile(File& file) {
  const uint16_t SEND_BUF_SIZE = 128;

  if (! checkClient())
    return false;

  uint8_t buffer[SEND_BUF_SIZE];
  size_t bytesRead;

  do {
    bytesRead = file.read(buffer, SEND_BUF_SIZE);
    if (bytesRead) {
      if (_client->write(buffer, bytesRead) != bytesRead)
        return false;
    }
  } while (bytesRead == SEND_BUF_SIZE);

  return true;
}
#endif

void EthWebServerApp::_sendHeader(const char* name, const char* value, bool progmem1, bool progmem2) {
  if (progmem1)
    _client->print(FPSTR(name));
  else
    _client->print(name);
  _client->print(F(": "));
  if (progmem2)
    _client->println(FPSTR(value));
  else
    _client->println(value);
}

bool EthWebServerApp::sendHeader(const char* name, const char* value) {
  if (! checkClient())
    return false;

  _sendHeader(name, value, false, false);

  return true;
}

bool EthWebServerApp::sendHeader(const __FlashStringHelper* name, const __FlashStringHelper* value) {
  if (! checkClient())
    return false;

  _sendHeader((PGM_P)name, (PGM_P)value, true, true);

  return true;
}

bool EthWebServerApp::sendHeader(const char* name, const __FlashStringHelper* value) {
  if (! checkClient())
    return false;

  _sendHeader(name, (PGM_P)value, false, true);

  return true;
}

bool EthWebServerApp::sendHeader(const __FlashStringHelper* name, const char* value) {
  if (! checkClient())
    return false;

  _sendHeader((PGM_P)name, value, true, false);

  return true;
}

bool EthWebServerApp::_sendContent(const char* content, bool progmem) {
  uint16_t bytesRemain = progmem ? strlen_P(content) : strlen(content);

  while (bytesRemain--) {
    char c;

    if (progmem)
      c = pgm_read_byte(content++);
    else
      c = *content++;
    if (_client->write(c) != sizeof(c))
      return false;
  }

  return true;
}

bool EthWebServerApp::sendContent(const char* content) {
  if (! checkClient())
    return false;

  return _sendContent(content, false);
}

bool EthWebServerApp::sendContent(const __FlashStringHelper* content) {
  if (! checkClient())
    return false;

  return _sendContent((PGM_P)content, true);
}

bool EthWebServerApp::sendContent(Stream& stream) {
  if (! checkClient())
    return false;

  while (stream.available()) {
    char c = stream.read();
    if (_client->write(c) != sizeof(c))
      return false;
  }

  return true;
}

PGM_P EthWebServerApp::responseCodeToString(int16_t code) {
  if (code == 100)
    return PSTR("Continue");
  else if (code == 101)
    return PSTR("Switching Protocols");
  else if (code == 200)
    return PSTR("OK");
  else if (code == 201)
    return PSTR("Created");
  else if (code == 202)
    return PSTR("Accepted");
  else if (code == 203)
    return PSTR("Non-Authoritative Information");
  else if (code == 204)
    return PSTR("No Content");
  else if (code == 205)
    return PSTR("Reset Content");
  else if (code == 206)
    return PSTR("Partial Content");
  else if (code == 300)
    return PSTR("Multiple Choices");
  else if (code == 301)
    return PSTR("Moved Permanently");
  else if (code == 302)
    return PSTR("Found");
  else if (code == 303)
    return PSTR("See Other");
  else if (code == 304)
    return PSTR("Not Modified");
  else if (code == 305)
    return PSTR("Use Proxy");
  else if (code == 307)
    return PSTR("Temporary Redirect");
  else if (code == 400)
    return PSTR("Bad Request");
  else if (code == 401)
    return PSTR("Unauthorized");
  else if (code == 402)
    return PSTR("Payment Required");
  else if (code == 403)
    return PSTR("Forbidden");
  else if (code == 404)
    return PSTR("Not Found");
  else if (code == 405)
    return PSTR("Method Not Allowed");
  else if (code == 406)
    return PSTR("Not Acceptable");
  else if (code == 407)
    return PSTR("Proxy Authentication Required");
  else if (code == 408)
    return PSTR("Request Time-out");
  else if (code == 409)
    return PSTR("Conflict");
  else if (code == 410)
    return PSTR("Gone");
  else if (code == 411)
    return PSTR("Length Required");
  else if (code == 412)
    return PSTR("Precondition Failed");
  else if (code == 413)
    return PSTR("Request Entity Too Large");
  else if (code == 414)
    return PSTR("Request-URI Too Large");
  else if (code == 415)
    return PSTR("Unsupported Media Type");
  else if (code == 416)
    return PSTR("Requested range not satisfiable");
  else if (code == 417)
    return PSTR("Expectation Failed");
  else if (code == 500)
    return PSTR("Internal Server Error");
  else if (code == 501)
    return PSTR("Not Implemented");
  else if (code == 502)
    return PSTR("Bad Gateway");
  else if (code == 503)
    return PSTR("Service Unavailable");
  else if (code == 504)
    return PSTR("Gateway Time-out");
  else if (code == 505)
    return PSTR("HTTP Version not supported");
  else
    return strEmpty;
}

static uint8_t hex(const char* digits) {
  uint8_t result;

  if ((digits[0] >= '0') && (digits[0] <= '9'))
    result = (digits[0] - '0') << 4;
  else if ((digits[0] >= 'A') && (digits[0] <= 'F'))
    result = (digits[0] - 'A' + 10) << 4;
  else if ((digits[0] >= 'a') && (digits[0] <= 'f'))
    result = (digits[0] - 'a' + 10) << 4;
  else
    return 0;

  if ((digits[1] >= '0') && (digits[1] <= '9'))
    result |= (digits[1] - '0');
  else if ((digits[1] >= 'A') && (digits[1] <= 'F'))
    result |= (digits[1] - 'A' + 10);
  else if ((digits[1] >= 'a') && (digits[1] <= 'f'))
    result |= (digits[1] - 'a' + 10);
  else
    return 0;

  return result;
}

char* EthWebServerApp::urlDecode(char* decoded, const char* url) {
  char* result = decoded;
  int16_t len = strlen(url);

  for (int16_t i = 0; i < len; ++i) {
    if ((url[i] == charPercent) && (i + 2 < len)) {
      *decoded++ = hex(&url[++i]);
      ++i;
    } else if (url[i] == charPlus)
      *decoded++ = charSpace;
    else
      *decoded++ = url[i];
  }
  *decoded = charNull;

  return result;
}

bool EthWebServerApp::parseRequest() {
  _uri = NULL;
  clearHeaders();
  clearArgs();
  _bufferLength = _client->readBytesUntil(charCR, (char*)_buffer, BUFFER_SIZE - 1);
  if ((_bufferLength < 14) || (_client->read() != charLF) || strncmp_P((const char*)_buffer, PSTR("GET "), 4) || strncmp_P((const char*)&_buffer[_bufferLength - 9], PSTR(" HTTP/1.1"), 9)) { // HTTP request first line looks like "GET /path HTTP/1.1"
    _log.println(F("Only HTTP 1.1 GET requests supported!"));
    return false;
  }
  _uri = (char*)&_buffer[4];
  _bufferLength -= 9;
  _buffer[_bufferLength++] = charNull;
  char* argList = strchr(_uri, charQuestion);
  if (argList) {
    *argList++ = charNull;
    parseArguments(argList);
  }
  char* headerList = NULL;
  uint16_t bufRemain = BUFFER_SIZE - _bufferLength - 1;
  while (bufRemain) {
    uint16_t numRead = _client->readBytesUntil(charCR, (char*)&_buffer[_bufferLength], bufRemain);
    if ((! numRead) || (_client->read() != charLF))
      break;
    _buffer[_bufferLength + numRead] = charNull;
    if (! strchr((const char*)&_buffer[_bufferLength], charColon))
      break;
    if (! headerList)
      headerList = (char*)&_buffer[_bufferLength];
    _bufferLength += numRead + 1;
    bufRemain -= numRead + 1;
  }
  _client->flush();
  if (headerList)
    parseHeaders(headerList);

  return true;
}

bool EthWebServerApp::parseArguments(const char* argList) {
  uint8_t cnt = 0;
  char* str = (char*)argList;

  while (*str) {
    if (*str++ == charEqual)
      ++cnt;
  }
  if (! cnt)
    return false;
  _args = (pair_t*)malloc(sizeof(pair_t) * cnt);
  if (! _args) {
    _log.println(FPSTR(strNotEnoughMemory));
    return false;
  }
  str = (char*)argList;
  _args[0].name = str;
  _args[0].value = NULL;
  _argCount = 0;
  while (*str) {
    if (*str == charEqual) {
      *str++ = charNull;
      if (! strlen(_args[_argCount].name)) {
        clearArgs();
        return false;
      }
      _args[_argCount].value = str;
    } else if (*str == charAmpersand) {
      *str++ = charNull;
      if (_args[_argCount].value)
        urlDecode(_args[_argCount].value, _args[_argCount].value);
      if ((! _args[_argCount].value) || (++_argCount >= cnt)) {
        clearArgs();
        return false;
      }
      _args[_argCount].name = str;
      _args[_argCount].value = NULL;
    } else
      ++str;
  }
  if (_args[_argCount].value)
    urlDecode(_args[_argCount].value, _args[_argCount].value);
  if ((! strlen(_args[_argCount].name)) || (! _args[_argCount].value) || (++_argCount != cnt)) {
    clearArgs();
    return false;
  }

  return true;
}

bool EthWebServerApp::parseHeaders(const char* headerList) {
  uint8_t cnt = 0;
  char* str = (char*)headerList;

  while (str < (char*)&_buffer[_bufferLength - 1]) {
    if (*str++ == charColon) {
      ++cnt;
      while (*str++);
    }
  }
  if (! cnt)
    return false;
  _headers = (pair_t*)malloc(sizeof(pair_t) * cnt);
  if (! _headers) {
    _log.println(FPSTR(strNotEnoughMemory));
    return false;
  }
  str = (char*)headerList;
  _headers[0].name = str;
  _headers[0].value = NULL;
  _headerCount = 0;
  while (str < (char*)&_buffer[_bufferLength - 1]) {
    if (*str == charColon) {
      *str++ = charNull;
      if (! strlen(_headers[_headerCount].name)) {
        clearHeaders();
        return false;
      }
      if (*str == charSpace)
        ++str;
      _headers[_headerCount].value = str;
      while (*str)
        ++str;
    } else if (! *str) {
      if ((! _headers[_headerCount].value) || (++_headerCount >= cnt)) {
        clearHeaders();
        return false;
      }
      _headers[_headerCount].name = ++str;
      _headers[_headerCount].value = NULL;
    } else
      ++str;
  }
  if ((! strlen(_headers[_headerCount].name)) || (! _headers[_headerCount].value) || (++_headerCount != cnt)) {
    clearHeaders();
    return false;
  }

  return true;
}

void EthWebServerApp::clearArgs() {
  if (_argCount) {
    free((void*)_args);
    _argCount = 0;
  }
}

void EthWebServerApp::clearHeaders() {
  if (_headerCount) {
    free((void*)_headers);
    _headerCount = 0;
  }
}

void EthWebServerApp::htmlPageStdStyle(bool full) {
  static const char style[] PROGMEM = "body{\n\
background-color:rgb(240,240,240);\n\
}";

  if (full)
    _htmlPageStyle((PGM_P)style, false, true);
  else {
    _client->print(FPSTR(style));
    _client->write(charLF);
  }
}

void EthWebServerApp::htmlPageStdScript(bool full) {
  static const char script[] PROGMEM = "function getXmlHttpRequest(){\n\
var xmlhttp;\n\
try{\n\
xmlhttp=new ActiveXObject(\"Msxml2.XMLHTTP\");\n\
}catch(e){\n\
try{\n\
xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\");\n\
}catch(E){\n\
xmlhttp=false;\n\
}\n\
}\n\
if ((!xmlhttp)&&(typeof XMLHttpRequest!='undefined')){\n\
xmlhttp=new XMLHttpRequest();\n\
}\n\
return xmlhttp;\n\
}\n\
function openUrl(url){\n\
var request=getXmlHttpRequest();\n\
request.open(\"GET\",url,false);\n\
request.send(null);\n\
}";

  if (full)
    _htmlPageScript((PGM_P)script, false, true);
  else {
    _client->print(FPSTR(script));
    _client->write(charLF);
  }
}

void EthWebServerApp::htmlPageEnd() {
  htmlTagEnd(FPSTR(tagBody), true);
  htmlTagEnd(FPSTR(tagHtml));
}

void EthWebServerApp::_htmlTagStart(const char* type, const char* attrs, bool newline, bool progmem1, bool progmem2) {
  _client->write(charLess);
  if (progmem1)
    _client->print(FPSTR(type));
  else
    _client->print(type);
  if (attrs) {
    _client->write(charSpace);
    if (progmem2)
      _client->print(FPSTR(attrs));
    else
      _client->print(attrs);
  }
  _client->write(charGreater);
  if (newline)
    _client->write(charLF);
}

void EthWebServerApp::_htmlTagEnd(const char* type, bool newline, bool progmem) {
  _client->write(charLess);
  _client->write(charSlash);
  if (progmem)
    _client->print(FPSTR(type));
  else
    _client->print(type);
  _client->write(charGreater);
  if (newline)
    _client->write(charLF);
}

void EthWebServerApp::_htmlPageStart(const char* title, bool progmem) {
  _client->print(F("<!DOCTYPE "));
  _client->print(FPSTR(tagHtml));
  _client->write(charGreater);
  _client->write(charLF);
  _htmlTagStart((PGM_P)tagHtml, (PGM_P)NULL, true, true, true);
  _htmlTagStart((PGM_P)tagHead, (PGM_P)NULL, true, true, true);
  _htmlTagStart((PGM_P)tagTitle, (PGM_P)NULL, false, true, true);
  if (progmem)
    _client->print(FPSTR(title));
  else
    _client->print(title);
  _htmlTagEnd((PGM_P)tagTitle, true, true);
}

void EthWebServerApp::_htmlPageStyleStart() {
  _client->write(charLess);
  _client->print(FPSTR(tagStyle));
  _client->write(charSpace);
  _client->print(FPSTR(attrType));
  _client->print(FPSTR(typeTextCss));
  _client->write(charQuote);
  _client->write(charGreater);
  _client->write(charLF);
}

void EthWebServerApp::_htmlPageStyleEnd() {
  _htmlTagEnd((PGM_P)tagStyle, true, true);
}

void EthWebServerApp::_htmlPageStyle(const char* style, bool file, bool progmem) {
  if (file) {
    _client->print(F("<link rel=\"stylesheet\" href=\""));
    if (progmem)
      _client->print(FPSTR(style));
    else
      _client->print(style);
    _client->print(F("\">\n"));
  } else {
    _htmlPageStyleStart();
    if (progmem)
      _client->print(FPSTR(style));
    else
      _client->print(style);
    _client->write(charLF);
    _htmlPageStyleEnd();
  }
}

void EthWebServerApp::_htmlPageScriptStart() {
  _client->write(charLess);
  _client->print(FPSTR(tagScript));
  _client->write(charSpace);
  _client->print(FPSTR(attrType));
  _client->print(FPSTR(typeTextJavascript));
  _client->write(charQuote);
  _client->write(charGreater);
  _client->write(charLF);
}

void EthWebServerApp::_htmlPageScriptEnd() {
  _htmlTagEnd((PGM_P)tagScript, true, true);
}

void EthWebServerApp::_htmlPageScript(const char* script, bool file, bool progmem) {
  if (file) {
    _client->write(charLess);
    _client->print(FPSTR(tagScript));
    _client->write(charSpace);
    _client->print(FPSTR(attrType));
    _client->print(FPSTR(typeTextJavascript));
    _client->write(charQuote);
    _client->write(charSpace);
    _client->print(FPSTR(attrSrc));
    if (progmem)
      _client->print(FPSTR(script));
    else
      _client->print(script);
    _client->write(charQuote);
    _client->write(charGreater);
  } else {
    _htmlPageScriptStart();
    if (progmem)
      _client->print(FPSTR(script));
    else
      _client->print(script);
    _client->write(charLF);
  }
  _htmlPageScriptEnd();
}

void EthWebServerApp::_htmlPageBody(const char* attrs, bool progmem) {
  _htmlTagEnd((PGM_P)tagHead, true, true);
  _htmlTagStart((PGM_P)tagBody, attrs, true, true, progmem);
}

void EthWebServerApp::_escapeQuote(const char* str, uint16_t maxlen, bool progmem) {
  char c;

  while (maxlen--) {
    if (progmem)
      c = pgm_read_byte(str++);
    else
      c = *str++;
    if (! c)
      break;
    if (c == charQuote)
      _client->print(FPSTR(strCharQuote));
    else
      _client->write(c);
  }
}

void EthWebServerApp::_htmlTag(const char* type, const char* attrs, const char* content, bool newline, bool progmem1, bool progmem2, bool progmem3) {
  _htmlTagStart(type, attrs, false, progmem1, progmem2);
  if (content) {
    if (progmem3)
      _client->print(FPSTR(content));
    else
      _client->print(content);
  }
  _htmlTagEnd(type, false, progmem1);
  if (newline)
    _client->write(charLF);
}

void EthWebServerApp::_htmlTagSimple(const char* type, const char* attrs, bool newline, bool progmem1, bool progmem2) {
  _client->write(charLess);
  if (progmem1)
    _client->print(FPSTR(type));
  else
    _client->print(type);
  if (attrs) {
    _client->write(charSpace);
    if (progmem2)
      _client->print(FPSTR(attrs));
    else
      _client->print(attrs);
  }
  _client->write(charSlash);
  _client->write(charGreater);
  if (newline)
    _client->write(charLF);
}

void EthWebServerApp::_htmlTagInput(const char* type, const char* name, const char* value, const char* attrs, bool newline, bool progmem1, bool progmem2, bool progmem3, bool progmem4) {
  _client->write(charLess);
  _client->print(FPSTR(tagInput));
  _client->write(charSpace);
  _client->print(FPSTR(attrType));
  if (progmem1)
    _client->print(FPSTR(type));
  else
    _client->print(type);
  _client->write(charQuote);
  if (name) {
    _client->write(charSpace);
    _client->print(FPSTR(attrName));
    if (progmem2)
      _client->print(FPSTR(name));
    else
      _client->print(name);
    _client->write(charQuote);
  }
  if (value) {
    _client->write(charSpace);
    _client->print(FPSTR(attrValue));
    _escapeQuote(value, (uint16_t)-1, progmem3);
    _client->write(charQuote);
  }
  if (attrs) {
    _client->write(charSpace);
    if (progmem4)
      _client->print(FPSTR(attrs));
    else
      _client->print(attrs);
  }
  _client->write(charSlash);
  _client->write(charGreater);
  if (newline)
    _client->write(charLF);
}

void EthWebServerApp::_htmlTagInputText(const char* name, const char* value, uint8_t size, uint8_t maxlength, const char* extra, bool newline, bool progmem1, bool progmem2, bool progmem3) {
  _client->write(charLess);
  _client->print(FPSTR(tagInput));
  _client->write(charSpace);
  _client->print(FPSTR(attrType));
  _client->print(FPSTR(inputTypeText));
  _client->write(charQuote);
  if (name) {
    _client->write(charSpace);
    _client->print(FPSTR(attrName));
    if (progmem1)
      _client->print(FPSTR(name));
    else
      _client->print(name);
    _client->write(charQuote);
  }
  if (value) {
    _client->write(charSpace);
    _client->print(FPSTR(attrValue));
    _escapeQuote(value, maxlength ? maxlength : (uint16_t)-1, progmem2);
    _client->write(charQuote);
  }
  if (size)
    printf(*_client, F(" size=%d"), size);
  if (maxlength)
    printf(*_client, F(" maxlength=%d"), maxlength);
  if (extra) {
    _client->write(charSpace);
    if (progmem3)
      _client->print(FPSTR(extra));
    else
      _client->print(extra);
  }
  _client->write(charSlash);
  _client->write(charGreater);
  if (newline)
    _client->write(charLF);
}

#ifdef USEMQTT
void EthWebServerApp::_mqttCallback(char* topic, byte* payload, unsigned int length) {
  _this->mqttCallback(topic, payload, length);
}

bool EthWebServerApp::mqttReconnect() {
  const uint32_t TIMEOUT = 30000; // 30 sec.
  static uint32_t nextTry;
  bool result = false;

  if ((int32_t)(millis() - nextTry) >= 0) {
    logDateTime(F("Attempting MQTT connection..."));
    if (*mqttUser)
      result = _pubSubClient->connect(mqttClient, mqttUser, mqttPassword);
    else
      result = _pubSubClient->connect(mqttClient);
    if (result) {
      _log.println(F(" successful"));
      mqttResubscribe();
    } else {
      _log.print(F(" failed, rc="));
      _log.println(_pubSubClient->state());
    }
    nextTry = millis() + TIMEOUT;
  }

  return result;
}

void EthWebServerApp::mqttResubscribe() {
  char topic[80];

  if (*mqttClient)
    sprintf(topic, sizeof(topic), F("/%s/#"), mqttClient);
  else
    strcpy_P(topic, PSTR("/#"));
  mqttSubscribe(topic);
}

void EthWebServerApp::mqttCallback(const char* topic, const uint8_t* payload, uint16_t length) {
  _log.print(F("MQTT message arrived ["));
  _log.print(topic);
  _log.print(F("] "));
  for (uint16_t i = 0; i < length; ++i) {
    _log.print((char)payload[i]);
  }
  _log.println();
}

bool EthWebServerApp::mqttSubscribe(const char* topic) {
  char _topic[128];

  if (*mqttClient) { // "/client..."
    strcpy_P(_topic, strSlash);
    strcat(_topic, mqttClient);
    strcat(_topic, topic);
  } else {
    strcpy(_topic, topic);
  }
  _log.print(F("Subscribe to MQTT topic \""));
  _log.print(_topic);
  _log.println('\"');

  return _pubSubClient->subscribe(_topic);
}

bool EthWebServerApp::mqttSubscribe(const __FlashStringHelper* topic) {
  bool result;
  char* _topic;

  _topic = (char*)malloc(strlen_P((PGM_P)topic) + 1);
  if (! _topic) {
    _log.println(FPSTR(strNotEnoughMemory));
    return false;
  }
  strcpy_P(_topic, (PGM_P)topic);
  result = mqttSubscribe(_topic);
  free((void*)_topic);

  return result;
}

bool EthWebServerApp::mqttPublish(const char* topic, const char* value, bool retained) {
  char _topic[128];

  if (*mqttClient) { // "/client..."
    strcpy_P(_topic, strSlash);
    strcat(_topic, mqttClient);
    strcat(_topic, topic);
  } else {
    strcpy(_topic, topic);
  }
  _log.print(F("Publish MQTT topic \""));
  _log.print(_topic);
  _log.print(F("\" with value \""));
  _log.print(value);
  _log.println('\"');

  return _pubSubClient->publish(_topic, value, retained);
}

bool EthWebServerApp::mqttPublish(const __FlashStringHelper* topic, const __FlashStringHelper* value, bool retained) {
  bool result;
  char* _topic;
  char* _value;

  _topic = (char*)malloc(strlen_P((PGM_P)topic) + 1);
  if (! _topic) {
    _log.println(FPSTR(strNotEnoughMemory));
    return false;
  }
  _value = (char*)malloc(strlen_P((PGM_P)value) + 1);
  if (! _value) {
    free((void*)_topic);
    _log.println(FPSTR(strNotEnoughMemory));
    return false;
  }
  strcpy_P(_topic, (PGM_P)topic);
  strcpy_P(_value, (PGM_P)value);
  result = mqttPublish(_topic, _value, retained);
  free((void*)_value);
  free((void*)_topic);

  return result;
}

bool EthWebServerApp::mqttPublish(const char* topic, const __FlashStringHelper* value, bool retained) {
  bool result;
  char* _value;

  _value = (char*)malloc(strlen_P((PGM_P)value) + 1);
  if (! _value) {
    _log.println(FPSTR(strNotEnoughMemory));
    return false;
  }
  strcpy_P(_value, (PGM_P)value);
  result = mqttPublish(topic, _value, retained);
  free((void*)_value);

  return result;
}

bool EthWebServerApp::mqttPublish(const __FlashStringHelper* topic, const char* value, bool retained) {
  bool result;
  char* _topic;

  _topic = (char*)malloc(strlen_P((PGM_P)topic) + 1);
  if (! _topic) {
    _log.println(FPSTR(strNotEnoughMemory));
    return false;
  }
  strcpy_P(_topic, (PGM_P)topic);
  result = mqttPublish(_topic, value, retained);
  free((void*)_topic);

  return result;
}

EthWebServerApp* EthWebServerApp::_this;
#endif
