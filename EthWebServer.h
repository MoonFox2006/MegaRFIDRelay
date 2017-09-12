#ifndef __ETHWEBSERVER_H
#define __ETHWEBSERVER_H

#include "EthWebCfg.h"
#include <avr/pgmspace.h>
#include <Stream.h>
#include <Ethernet.h>
#ifdef USESDCARD
#include <SD.h>
#endif
#ifdef USEMQTT
#include <PubSubClient.h>
#endif
#include "StringLog.h"
#include "printp.h"
#include "Misc.h"

const char charNull = '\0';
const char charTab = '\t';
const char charLF = '\n';
const char charCR = '\r';
const char charSpace = ' ';
const char charQuestion = '?';
const char charPlus = '+';
const char charMinus = '-';
const char charPercent = '%';
const char charEqual = '=';
const char charLess = '<';
const char charGreater = '>';
const char charAmpersand = '&';
const char charColon = ':';
const char charSemicolon = ';';
const char charComma = ',';
const char charDot = '.';
const char charQuote = '"';
const char charApostrophe = '\'';
const char charSlash = '/';
const char charBackslash = '\\';

const char strEmpty[] PROGMEM = "";
const char strSlash[] PROGMEM = "/";

const char strCharSpace[] PROGMEM = "&nbsp;";
const char strCharQuote[] PROGMEM = "&quot;";

const char strFalse[] PROGMEM = "false";
const char strTrue[] PROGMEM = "true";

const char strNotEnoughMemory[] PROGMEM = "Not enough memory!";

const char typeTextPlain[] PROGMEM = "text/plain";
const char typeTextHtml[] PROGMEM = "text/html";
const char typeTextJson[] PROGMEM = "text/json";
const char typeTextCss[] PROGMEM = "text/css";
const char typeTextJavascript[] PROGMEM = "text/javascript";
const char typeApplicationJavascript[] PROGMEM = "application/javascript";
const char typeImageIcon[] PROGMEM = "image/x-icon";

const char tagHtml[] PROGMEM = "html";
const char tagHead[] PROGMEM = "head";
const char tagTitle[] PROGMEM = "title";
const char tagBody[] PROGMEM = "body";
const char tagStyle[] PROGMEM = "style";
const char tagScript[] PROGMEM = "script";
const char tagForm[] PROGMEM = "form";
const char tagBR[] PROGMEM = "br";
const char tagP[] PROGMEM = "p";
const char tagH1[] PROGMEM = "h1";
const char tagH2[] PROGMEM = "h2";
const char tagH3[] PROGMEM = "h3";
const char tagLabel[] PROGMEM = "label";
const char tagInput[] PROGMEM = "input";
const char tagSpan[] PROGMEM = "span";
const char tagTable[] PROGMEM = "table";
const char tagCaption[] PROGMEM = "caption";
const char tagTR[] PROGMEM = "tr";
const char tagTH[] PROGMEM = "th";
const char tagTD[] PROGMEM = "td";

const char attrType[] PROGMEM = "type=\"";
const char attrName[] PROGMEM = "name=\"";
const char attrValue[] PROGMEM = "value=\"";
const char attrSrc[] PROGMEM = "src=\"";
const char attrHref[] PROGMEM = "href=\"";
const char attrChecked[] PROGMEM = "checked";
const char attrSelected[] PROGMEM = "selected";

const char inputTypeText[] PROGMEM = "text";
const char inputTypePassword[] PROGMEM = "password";
const char inputTypeButton[] PROGMEM = "button";
const char inputTypeRadio[] PROGMEM = "radio";
const char inputTypeCheckbox[] PROGMEM = "checkbox";
const char inputTypeSubmit[] PROGMEM = "submit";
const char inputTypeReset[] PROGMEM = "reset";
const char inputTypeHidden[] PROGMEM = "hidden";

const char pathRoot[] PROGMEM = "/index.html";
const char pathFavicon[] PROGMEM = "/favicon.ico";
const char pathStdStyle[] PROGMEM = "/std.css";
const char pathStdScript[] PROGMEM = "/std.js";
const char pathJson[] PROGMEM = "/json";
const char pathStore[] PROGMEM = "/store";
const char pathEthernetConfig[] PROGMEM = "/ethernet";
const char pathTimeConfig[] PROGMEM = "/ntp";
const char pathSetTime[] PROGMEM = "/settime";
const char pathLog[] PROGMEM = "/log";
const char pathClearLog[] PROGMEM = "/clearlog";
const char pathRestart[] PROGMEM = "/restart";
#ifdef USESDCARD
const char pathSDCard[] PROGMEM = "/sdcard";
#endif
#ifdef USEMQTT
const char pathMQTTConfig[] PROGMEM = "/mqtt";
#endif

const uint8_t defMAC[6] PROGMEM = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
const uint32_t defIP = 0xF001A8C0; // 192.168.1.240
const uint32_t defMask = 0x00FFFFFF; // 255.255.255.0
const uint32_t defGate = 0x0101A8C0; // 192.168.1.1
const uint32_t defDNS = 0x0101A8C0; // 192.168.1.1
const uint16_t defMaintenanceInterval = 480; // 8 hours in minutes
const char defNtpServer[] PROGMEM = "pool.ntp.org";
const int8_t defTimeZone = 3; // GMT+03 (MSK)
const uint16_t defUpdateInterval = 240; // 4 hours in minutes
#ifdef USEAUTHORIZATION
const char defAuthUser[] PROGMEM = "MEGA";
const char defAuthPassword[] PROGMEM = "12345678";
#endif
#ifdef USEMQTT
const uint16_t defMQTTPort = 1883;
const char defMQTTClient[] PROGMEM = "MEGA";
#endif

const char paramReboot[] PROGMEM = "reboot";
const char paramMAC[] PROGMEM = "mac";
const char paramIP[] PROGMEM = "ip";
const char paramMask[] PROGMEM = "mask";
const char paramGate[] PROGMEM = "gate";
const char paramDNS[] PROGMEM = "dns";
const char paramMaintenance[] PROGMEM = "maintenance";
const char paramNtpServer[] PROGMEM = "ntpserver";
const char paramTimeZone[] PROGMEM = "timezone";
const char paramUpdateInterval[] PROGMEM = "ntpinterval";
const char paramTime[] PROGMEM = "time";
const char paramSrc[] PROGMEM = "src";
#ifdef USEAUTHORIZATION
const char paramAuthUser[] PROGMEM = "authuser";
const char paramAuthPassword[] PROGMEM = "authpassword";
#endif
#ifdef USEMQTT
const char paramMQTTServer[] PROGMEM = "mqttserver";
const char paramMQTTPort[] PROGMEM = "mqttport";
const char paramMQTTUser[] PROGMEM = "mqttuser";
const char paramMQTTPassword[] PROGMEM = "mqttpswd";
const char paramMQTTClient[] PROGMEM = "mqttclient";
#endif

const char jsonFreeMem[] PROGMEM = "freemem";
const char jsonUptime[] PROGMEM = "uptime";
const char jsonUnixTime[] PROGMEM = "unixtime";
const char jsonDate[] PROGMEM = "date";
const char jsonTime[] PROGMEM = "time";
#ifdef USEMQTT
const char jsonMQTTConnected[] PROGMEM = "mqttconnected";
#endif

#define WEAK_VIRTUAL

class EthWebServerApp {
public:
  EthWebServerApp(uint8_t chunks = 0, uint16_t port = 80);

  void setup();
  void loop();

  virtual void restart() {
    softReset();
  }

protected:
  /* Parameters */
  void clearParams();
  bool readParams();
  bool writeParams();
  virtual void defaultParams(uint8_t chunk);
  virtual bool setParameter(const char* name, const char* value);
  virtual int16_t readParamsData(uint16_t offset, uint8_t chunk);
  virtual int16_t writeParamsData(uint16_t offset, uint8_t chunk);
  int16_t checkParamsSignature();
  int16_t writeParamsSignature();
  WEAK_VIRTUAL uint16_t _readData(uint16_t offset, void* buf, uint16_t size, bool calcCRC = true);
  WEAK_VIRTUAL uint16_t _writeData(uint16_t offset, const void* buf, uint16_t size, bool calcCRC = true);
  void initCRC() {
    _crc = 0xFF;
  }
  void updateCRC(uint8_t data) {
    _crc = _crc8update(_crc, data);
  }
  uint8_t getCRC() const {
    return _crc;
  }
  int16_t checkCRC(uint16_t offset);
  int16_t writeCRC(uint16_t offset);
  static uint8_t _crc8update(uint8_t crc, uint8_t data);
  static uint8_t _crc8(uint8_t crc, const uint8_t* buf, uint16_t size);

  static const uint8_t STR_PARAM_SIZE = 32;

  uint8_t _chunks;

  uint8_t mac[6];
  uint32_t ip;
  uint32_t mask;
  uint32_t gate;
  uint32_t dns;
  uint16_t maintenanceInterval;
  char ntpServer[STR_PARAM_SIZE];
  int8_t timeZone;
  uint16_t updateInterval;

#ifdef USEAUTHORIZATION
  char authUser[STR_PARAM_SIZE];
  char authPassword[STR_PARAM_SIZE];
#endif

  /* SoftRTC */
  uint32_t getTime();
  void setTime(uint32_t time);
  bool updateTime(uint32_t timeout = 1000);

  /* Logger */
  void logDate(const char* str = NULL, uint32_t time = 0, bool newline = false);
  void logDate(const __FlashStringHelper* str = NULL, uint32_t time = 0, bool newline = false);
  void logTime(const char* str = NULL, uint32_t time = 0, bool newline = false);
  void logTime(const __FlashStringHelper* str = NULL, uint32_t time = 0, bool newline = false);
  void logDateTime(const char* str = NULL, uint32_t time = 0, bool newline = false);
  void logDateTime(const __FlashStringHelper* str = NULL, uint32_t time = 0, bool newline = false);

  StringLog _log;

  /* WebServer */
  bool checkClient();

  bool startResponse(int16_t code, const char* contentType, uint32_t contentLength = (uint32_t)-1, bool incomplete = false);
  bool startResponse(int16_t code, const __FlashStringHelper* contentType, uint32_t contentLength = (uint32_t)-1, bool incomplete = false);
  bool continueResponse();
  bool endResponse();

  bool authenticate(const char* user, const char* password) {
    return _authenticate(user, password, false, false);
  }
  bool authenticate(const __FlashStringHelper* user, const __FlashStringHelper* password) {
    return _authenticate((PGM_P)user, (PGM_P)password, true, true);
  }
  bool authenticate(const char* user, const __FlashStringHelper* password) {
    return _authenticate(user, (PGM_P)password, false, true);
  }
  bool authenticate(const __FlashStringHelper* user, const char* password) {
    return _authenticate((PGM_P)user, password, true, false);
  }
  void requestAuthentication();

  void handleClient();
  virtual void handleRequest(const char* uri);
  bool parseRequest();
  bool parseArguments(const char* argList);
  bool parseHeaders(const char* headerList);

  virtual void handleRootPage();
  virtual void handleNotFound();
  WEAK_VIRTUAL void handleFavicon();
  WEAK_VIRTUAL void handleJson();
  virtual void handleJsonData(uint8_t src);
  WEAK_VIRTUAL void handleStore();
  WEAK_VIRTUAL void handleEthernetConfig();
  WEAK_VIRTUAL void handleTimeConfig();
  WEAK_VIRTUAL void handleSetTime();
  WEAK_VIRTUAL void handleLog();
  WEAK_VIRTUAL void handleClearLog();
  WEAK_VIRTUAL void handleRestart();
#ifdef USESDCARD
  WEAK_VIRTUAL void handleSDCard();
  WEAK_VIRTUAL bool handleFile();
#endif
#ifdef USEMQTT
  WEAK_VIRTUAL void handleMQTTConfig();
#endif

  void btnToGo(PGM_P caption, PGM_P path);
  virtual void btnNavigator();
  void btnBack();
  void btnEthernet();
  void btnTime();
  void btnLog();
  void btnRestart();
#ifdef USEMQTT
  void btnMQTT();
#endif

  const char* uri() const {
    return _uri;
  }
  EthernetClient* client() {
    return _client;
  }
  uint8_t args() const {
    return _argCount;
  }
  const char* arg(const char* name);
  const char* arg(const __FlashStringHelper* name);
  const char* arg(uint8_t ind);
  const char* argName(uint8_t ind);
  bool hasArg(const char* name);
  bool hasArg(const __FlashStringHelper* name);
  uint8_t headers() const {
    return _headerCount;
  }
  const char* header(const char* name);
  const char* header(const __FlashStringHelper* name);
  const char* header(uint8_t ind);
  const char* headerName(uint8_t ind);
  bool hasHeader(const char* name);
  bool hasHeader(const __FlashStringHelper* name);
  bool send(int16_t code, const char* contentType, const char* content);
  bool send(int16_t code, const __FlashStringHelper* contentType, const __FlashStringHelper* content);
  bool send(int16_t code, const char* contentType, const __FlashStringHelper* content);
  bool send(int16_t code, const __FlashStringHelper* contentType, const char* content);
#ifdef USESDCARD
  bool sendFile(File& file);
#endif
  bool sendHeader(const char* name, const char* value);
  bool sendHeader(const __FlashStringHelper* name, const __FlashStringHelper* value);
  bool sendHeader(const char* name, const __FlashStringHelper* value);
  bool sendHeader(const __FlashStringHelper* name, const char* value);
  bool sendContent(Stream& stream);
  bool sendContent(const char* content);
  bool sendContent(const __FlashStringHelper* content);

  void htmlPageStart(const char* title) {
    _htmlPageStart(title, false);
  }
  void htmlPageStart(const __FlashStringHelper* title) {
    _htmlPageStart((PGM_P)title, true);
  }
  void htmlPageStyleStart() {
    _htmlPageStyleStart();
  }
  void htmlPageStyleEnd() {
    _htmlPageStyleEnd();
  }
  void htmlPageStyle(const char* style, bool file = false) {
    _htmlPageStyle(style, file, false);
  }
  void htmlPageStyle(const __FlashStringHelper* style, bool file = false) {
    _htmlPageStyle((PGM_P)style, file, true);
  }
  void htmlPageStdStyle(bool full = true);
  void htmlPageScriptStart() {
    _htmlPageScriptStart();
  }
  void htmlPageScriptEnd() {
    _htmlPageScriptEnd();
  }
  void htmlPageScript(const char* script, bool file = false) {
    _htmlPageScript(script, file, false);
  }
  void htmlPageScript(const __FlashStringHelper* script, bool file = false) {
    _htmlPageScript((PGM_P)script, file, true);
  }
  void htmlPageStdScript(bool full = true);
  void htmlPageBody() {
    _htmlPageBody((const char*)NULL, false);
  }
  void htmlPageBody(const char* attrs) {
    _htmlPageBody(attrs, false);
  }
  void htmlPageBody(const __FlashStringHelper* attrs) {
    _htmlPageBody((PGM_P)attrs, true);
  }
  void htmlPageEnd();
  void htmlTagStart(const char* type, const char* attrs, bool newline = false) {
    _htmlTagStart(type, attrs, newline, false, false);
  }
  void htmlTagStart(const __FlashStringHelper* type, const __FlashStringHelper* attrs, bool newline = false) {
    _htmlTagStart((PGM_P)type, (PGM_P)attrs, newline, true, true);
  }
  void htmlTagStart(const char* type, const __FlashStringHelper* attrs, bool newline = false) {
    _htmlTagStart(type, (PGM_P)attrs, newline, false, true);
  }
  void htmlTagStart(const __FlashStringHelper* type, const char* attrs, bool newline = false) {
    _htmlTagStart((PGM_P)type, attrs, newline, true, false);
  }
  void htmlTagStart(const char* type, bool newline = false) {
    htmlTagStart(type, (const char*)NULL, newline);
  }
  void htmlTagStart(const __FlashStringHelper* type, bool newline = false) {
    htmlTagStart(type, FPSTR(NULL), newline);
  }
  void htmlTagEnd(const char* type, bool newline = false) {
    _htmlTagEnd(type, newline, false);
  }
  void htmlTagEnd(const __FlashStringHelper* type, bool newline = false) {
    _htmlTagEnd((PGM_P)type, newline, true);
  }
  void htmlTag(const char* type, const char* attrs, const char* content, bool newline = false) {
    _htmlTag(type, attrs, content, newline, false, false, false);
  }
  void htmlTag(const __FlashStringHelper* type, const __FlashStringHelper* attrs, const __FlashStringHelper* content, bool newline = false) {
    _htmlTag((PGM_P)type, (PGM_P)attrs, (PGM_P)content, newline, true, true, true);
  }
  void htmlTag(const char* type, const __FlashStringHelper* attrs, const __FlashStringHelper* content, bool newline = false) {
    _htmlTag(type, (PGM_P)attrs, (PGM_P)content, newline, false, true, true);
  }
  void htmlTag(const char* type, const char* attrs, const __FlashStringHelper* content, bool newline = false) {
    _htmlTag(type, attrs, (PGM_P)content, newline, false, false, true);
  }
  void htmlTag(const char* type, const __FlashStringHelper* attrs, const char* content, bool newline = false) {
    _htmlTag(type, (PGM_P)attrs, content, newline, false, true, false);
  }
  void htmlTag(const __FlashStringHelper* type, const char* attrs, const char* content, bool newline = false) {
    _htmlTag((PGM_P)type, attrs, content, newline, true, false, false);
  }
  void htmlTag(const __FlashStringHelper* type, const __FlashStringHelper* attrs, const char* content, bool newline = false) {
    _htmlTag((PGM_P)type, (PGM_P)attrs, content, newline, true, true, false);
  }
  void htmlTag(const __FlashStringHelper* type, const char* attrs, const __FlashStringHelper* content, bool newline = false) {
    _htmlTag((PGM_P)type, attrs, (PGM_P)content, newline, true, false, true);
  }
  void htmlTag(const char* type, const char* content, bool newline = false) {
    htmlTag(type, (const char*)NULL, content, newline);
  }
  void htmlTag(const __FlashStringHelper* type, const __FlashStringHelper* content, bool newline = false) {
    htmlTag(type, FPSTR(NULL), content, newline);
  }
  void htmlTag(const char* type, const __FlashStringHelper* content, bool newline = false) {
    htmlTag(type, FPSTR(NULL), content, newline);
  }
  void htmlTag(const __FlashStringHelper* type, const char* content, bool newline = false) {
    htmlTag(type, FPSTR(NULL), content, newline);
  }
  void htmlTagSimple(const char* type, const char* attrs, bool newline = false) {
    _htmlTagSimple(type, attrs, newline, false, false);
  }
  void htmlTagSimple(const __FlashStringHelper* type, const __FlashStringHelper* attrs, bool newline = false) {
    _htmlTagSimple((PGM_P)type, (PGM_P)attrs, newline, true, true);
  }
  void htmlTagSimple(const char* type, const __FlashStringHelper* attrs, bool newline = false) {
    _htmlTagSimple(type, (PGM_P)attrs, newline, false, true);
  }
  void htmlTagSimple(const __FlashStringHelper* type, const char* attrs, bool newline = false) {
    _htmlTagSimple((PGM_P)type, attrs, newline, true, false);
  }
  void htmlTagSimple(const char* type, bool newline = false) {
    htmlTagSimple(type, (const char*)NULL, newline);
  }
  void htmlTagSimple(const __FlashStringHelper* type, bool newline = false) {
    htmlTagSimple(type, FPSTR(NULL), newline);
  }
  void htmlTagBR(bool newline = false) {
    htmlTagSimple(FPSTR(tagBR), newline);
  }
  void htmlTagInput(const char* type, const char* name, const char* value, const char* attrs, bool newline = false) {
    _htmlTagInput(type, name, value, attrs, newline, false, false, false, false);
  }
  void htmlTagInput(const __FlashStringHelper* type, const __FlashStringHelper* name, const __FlashStringHelper* value, const __FlashStringHelper* attrs, bool newline = false) {
    _htmlTagInput((PGM_P)type, (PGM_P)name, (PGM_P)value, (PGM_P)attrs, newline, true, true, true, true);
  }
  void htmlTagInput(const char* type, const __FlashStringHelper* name, const __FlashStringHelper* value, const __FlashStringHelper* attrs, bool newline = false) {
    _htmlTagInput(type, (PGM_P)name, (PGM_P)value, (PGM_P)attrs, newline, false, true, true, true);
  }
  void htmlTagInput(const char* type, const char* name, const __FlashStringHelper* value, const __FlashStringHelper* attrs, bool newline = false) {
    _htmlTagInput(type, name, (PGM_P)value, (PGM_P)attrs, newline, false, false, true, true);
  }
  void htmlTagInput(const char* type, const char* name, const char* value, const __FlashStringHelper* attrs, bool newline = false) {
    _htmlTagInput(type, name, value, (PGM_P)attrs, newline, false, false, false, true);
  }
  void htmlTagInput(const char* type, const char* name, const __FlashStringHelper* value, const char* attrs, bool newline = false) {
    _htmlTagInput(type, name, (PGM_P)value, attrs, newline, false, false, true, false);
  }
  void htmlTagInput(const char* type, const __FlashStringHelper* name, const char* value, const char* attrs, bool newline = false) {
    _htmlTagInput(type, (PGM_P)name, value, attrs, newline, false, true, false, false);
  }
  void htmlTagInput(const char* type, const __FlashStringHelper* name, const char* value, const __FlashStringHelper* attrs, bool newline = false) {
    _htmlTagInput(type, (PGM_P)name, value, (PGM_P)attrs, newline, false, true, false, true);
  }
  void htmlTagInput(const char* type, const __FlashStringHelper* name, const __FlashStringHelper* value, const char* attrs, bool newline = false) {
    _htmlTagInput(type, (PGM_P)name, (PGM_P)value, attrs, newline, false, true, true, false);
  }
  void htmlTagInput(const __FlashStringHelper* type, const char* name, const char* value, const char* attrs, bool newline = false) {
    _htmlTagInput((PGM_P)type, name, value, attrs, newline, true, false, false, false);
  }
  void htmlTagInput(const __FlashStringHelper* type, const char* name, const char* value, const __FlashStringHelper* attrs, bool newline = false) {
    _htmlTagInput((PGM_P)type, name, value, (PGM_P)attrs, newline, true, false, false, true);
  }
  void htmlTagInput(const __FlashStringHelper* type, const char* name, const __FlashStringHelper* value, const char* attrs, bool newline = false) {
    _htmlTagInput((PGM_P)type, name, (PGM_P)value, attrs, newline, true, false, true, false);
  }
  void htmlTagInput(const __FlashStringHelper* type, const char* name, const __FlashStringHelper* value, const __FlashStringHelper* attrs, bool newline = false) {
    _htmlTagInput((PGM_P)type, name, (PGM_P)value, (PGM_P)attrs, newline, true, false, true, true);
  }
  void htmlTagInput(const __FlashStringHelper* type, const __FlashStringHelper* name, const char* value, const char* attrs, bool newline = false) {
    _htmlTagInput((PGM_P)type, (PGM_P)name, value, attrs, newline, true, true, false, false);
  }
  void htmlTagInput(const __FlashStringHelper* type, const __FlashStringHelper* name, const __FlashStringHelper* value, const char* attrs, bool newline = false) {
    _htmlTagInput((PGM_P)type, (PGM_P)name, (PGM_P)value, attrs, newline, true, true, true, false);
  }
  void htmlTagInput(const __FlashStringHelper* type, const __FlashStringHelper* name, const char* value, const __FlashStringHelper* attrs, bool newline = false) {
    _htmlTagInput((PGM_P)type, (PGM_P)name, value, (PGM_P)attrs, newline, true, true, false, true);
  }
  void htmlTagInput(const char* type, const char* name, const char* value, bool newline = false) {
    htmlTagInput(type, name, value, (const char*)NULL, newline);
  }
  void htmlTagInput(const __FlashStringHelper* type, const __FlashStringHelper* name, const __FlashStringHelper* value, bool newline = false) {
    htmlTagInput(type, name, value, FPSTR(NULL), newline);
  }
  void htmlTagInput(const char* type, const __FlashStringHelper* name, const __FlashStringHelper* value, bool newline = false) {
    htmlTagInput(type, name, value, FPSTR(NULL), newline);
  }
  void htmlTagInput(const char* type, const char* name, const __FlashStringHelper* value, bool newline = false) {
    htmlTagInput(type, name, value, FPSTR(NULL), newline);
  }
  void htmlTagInput(const char* type, const __FlashStringHelper* name, const char* value, bool newline = false) {
    htmlTagInput(type, name, value, FPSTR(NULL), newline);
  }
  void htmlTagInput(const __FlashStringHelper* type, const char* name, const char* value, bool newline = false) {
    htmlTagInput(type, name, value, FPSTR(NULL), newline);
  }
  void htmlTagInput(const __FlashStringHelper* type, const char* name, const __FlashStringHelper* value, bool newline = false) {
    htmlTagInput(type, name, value, FPSTR(NULL), newline);
  }
  void htmlTagInput(const __FlashStringHelper* type, const __FlashStringHelper* name, const char* value, bool newline = false) {
    htmlTagInput(type, name, value, FPSTR(NULL), newline);
  }
  void htmlTagInputText(const char* name, const char* value, uint8_t size = 0, uint8_t maxlength = 0, const char* extra = NULL, bool newline = false) {
    _htmlTagInputText(name, value, size, maxlength, extra, newline, false, false, false);
  }
  void htmlTagInputText(const __FlashStringHelper* name, const __FlashStringHelper* value, uint8_t size = 0, uint8_t maxlength = 0, const __FlashStringHelper* extra = NULL, bool newline = false) {
    _htmlTagInputText((PGM_P)name, (PGM_P)value, size, maxlength, (PGM_P)extra, newline, true, true, true);
  }
  void htmlTagInputText(const char* name, const __FlashStringHelper* value, uint8_t size = 0, uint8_t maxlength = 0, const __FlashStringHelper* extra = NULL, bool newline = false) {
    _htmlTagInputText(name, (PGM_P)value, size, maxlength, (PGM_P)extra, newline, false, true, true);
  }
  void htmlTagInputText(const char* name, const char* value, uint8_t size = 0, uint8_t maxlength = 0, const __FlashStringHelper* extra = NULL, bool newline = false) {
    _htmlTagInputText(name, value, size, maxlength, (PGM_P)extra, newline, false, false, true);
  }
  void htmlTagInputText(const char* name, const __FlashStringHelper* value, uint8_t size = 0, uint8_t maxlength = 0, const char* extra = NULL, bool newline = false) {
    _htmlTagInputText(name, (PGM_P)value, size, maxlength, extra, newline, false, true, false);
  }
  void htmlTagInputText(const __FlashStringHelper* name, const char* value, uint8_t size = 0, uint8_t maxlength = 0, const __FlashStringHelper* extra = NULL, bool newline = false) {
    _htmlTagInputText((PGM_P)name, value, size, maxlength, (PGM_P)extra, newline, true, false, true);
  }
  void htmlTagInputText(const __FlashStringHelper* name, const char* value, uint8_t size = 0, uint8_t maxlength = 0, const char* extra = NULL, bool newline = false) {
    _htmlTagInputText((PGM_P)name, value, size, maxlength, extra, newline, true, false, false);
  }
  void htmlTagInputText(const __FlashStringHelper* name, const __FlashStringHelper* value, uint8_t size = 0, uint8_t maxlength = 0, const char* extra = NULL, bool newline = false) {
    _htmlTagInputText((PGM_P)name, (PGM_P)value, size, maxlength, extra, newline, true, true, false);
  }
  void htmlTagInputText(const char* name, const char* value, uint8_t size = 0, uint8_t maxlength = 0, bool newline = false) {
    htmlTagInputText(name, value, size, maxlength, (const char*)NULL, newline);
  }
  void htmlTagInputText(const __FlashStringHelper* name, const __FlashStringHelper* value, uint8_t size = 0, uint8_t maxlength = 0, bool newline = false) {
    htmlTagInputText(name, value, size, maxlength, FPSTR(NULL), newline);
  }
  void htmlTagInputText(const char* name, const __FlashStringHelper* value, uint8_t size = 0, uint8_t maxlength = 0, bool newline = false) {
    htmlTagInputText(name, value, size, maxlength, FPSTR(NULL), newline);
  }
  void htmlTagInputText(const __FlashStringHelper* name, const char* value, uint8_t size = 0, uint8_t maxlength = 0, bool newline = false) {
    htmlTagInputText(name, value, size, maxlength, FPSTR(NULL), newline);
  }

  static PGM_P responseCodeToString(int16_t code);
  static char* urlDecode(char* decoded, const char* url);

  void escapeQuote(const char* str, uint16_t maxlen = (uint16_t)-1) {
    _escapeQuote(str, maxlen, false);
  }
  void escapeQuote(const __FlashStringHelper* str, uint16_t maxlen = (uint16_t)-1) {
    _escapeQuote((PGM_P)str, maxlen, true);
  }

  struct {
    bool _haveSD : 1;
    bool _haveDS3231 : 1;
    bool _haveAT24C32 : 1;
  };

  EthernetServer* _server;
  EthernetClient* _client;
  char* _uri;
  uint8_t _argCount;
  uint8_t _headerCount;

#ifdef USEMQTT
  WEAK_VIRTUAL bool mqttReconnect();
  virtual void mqttResubscribe();
  virtual void mqttCallback(const char* topic, const uint8_t* payload, uint16_t length);
  bool mqttSubscribe(const char* topic);
  bool mqttSubscribe(const __FlashStringHelper* topic);
  bool mqttPublish(const char* topic, const char* value, bool retained = true);
  bool mqttPublish(const __FlashStringHelper* topic, const __FlashStringHelper* value, bool retained = true);
  bool mqttPublish(const char* topic, const __FlashStringHelper* value, bool retained = true);
  bool mqttPublish(const __FlashStringHelper* topic, const char* value, bool retained = true);

  char mqttServer[STR_PARAM_SIZE];
  uint16_t mqttPort;
  char mqttUser[STR_PARAM_SIZE];
  char mqttPassword[STR_PARAM_SIZE];
  char mqttClient[STR_PARAM_SIZE];

  PubSubClient* _pubSubClient;
#endif

private:
  void _startResponse(int16_t code, const char* contentType, uint32_t contentLength, bool incomplete, bool progmem);
  bool _authenticate(const char* user, const char* password, bool progmem1, bool progmem2);
  void _sendHeader(const char* name, const char* value, bool progmem1, bool progmem2);
  bool _sendContent(const char* content, bool progmem);

  void _htmlTagStart(const char* type, const char* attrs, bool newline, bool progmem1, bool progmem2);
  void _htmlTagEnd(const char* type, bool newline, bool progmem);
  void _htmlPageStart(const char* title, bool progmem);
  void _htmlPageStyleStart();
  void _htmlPageStyleEnd();
  void _htmlPageStyle(const char* style, bool file, bool progmem);
  void _htmlPageScriptStart();
  void _htmlPageScriptEnd();
  void _htmlPageScript(const char* script, bool file, bool progmem);
  void _htmlPageBody(const char* attrs, bool progmem);
  void _escapeQuote(const char* str, uint16_t maxlen, bool progmem);
  void _htmlTag(const char* type, const char* attrs, const char* content, bool newline, bool progmem1, bool progmem2, bool progmem3);
  void _htmlTagSimple(const char* type, const char* attrs, bool newline, bool progmem1, bool progmem2);
  void _htmlTagInput(const char* type, const char* name, const char* value, const char* attrs, bool newline, bool progmem1, bool progmem2, bool progmem3, bool progmem4);
  void _htmlTagInputText(const char* name, const char* value, uint8_t size, uint8_t maxlength, const char* extra, bool newline, bool progmem1, bool progmem2, bool progmem3);

  void clearArgs();
  void clearHeaders();

  uint8_t _crc;

  uint32_t _nextMaintenance;

  uint32_t _lastTime;
  uint32_t _lastMillis;
  uint32_t _nextUpdate;

  static const uint16_t BUFFER_SIZE = 1024; // Arduino Mega!!!

  struct pair_t {
    char* name;
    char* value;
  };

  uint8_t _buffer[BUFFER_SIZE];
  uint16_t _bufferLength;
  pair_t* _args;
  pair_t* _headers;

#ifdef USEMQTT
  static void _mqttCallback(char* topic, byte* payload, unsigned int length);

  EthernetClient* _ethClient;
  static EthWebServerApp* _this;
#endif
};

#endif
