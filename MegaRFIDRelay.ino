#define MAX_RELAYS 48 // Максимальное количество каналов реле
#define RELAYS_PER_COLUMN 16
#define MAX_SCHEDULES 20 // Закомментируйте этот макроопределение, если вы не хотите использовать расписание

#if MAX_RELAYS > 48
#error Too many relays!
#endif

#define USEDS1820 // Закомментируйте, если не хотите подключать DS18x20

#include "EthWebServer.h"
#include "strp.h"
#ifdef MAX_SCHEDULES
#include "Schedule.h"
#include "Date.h"
#endif
#include <MFRC522.h>
#ifdef USEDS1820
#include "DS1820.h"
#endif

#ifndef USEMQTT
#error USEMQTT must be defined!
#endif

const int8_t MIN_PIN_NO = 2; // Skip RX0 and TX0
const int8_t MAX_PIN_NO = 53;

#define LED_PIN 13 // LED anode pin (comment if not needed)
const int8_t RFID_RST_PIN = 49; // SPI RFID RST pin
const int8_t RFID_SS_PIN = 53; // SPI RFID SS pin

const uint8_t UID_LENGTH = 7; // Максимальная длина UID RFID-ключа
const uint8_t MAX_KEYS = 128; // Максимальное количество RFID-ключей

const int8_t defRelayPin = -1; // Пин реле по умолчанию (-1 - не подключено)
const bool defRelayLevel = HIGH; // Уровень срабатывания реле по умолчанию
const bool defRelayOnBoot = false; // Состояние реле при старте модуля по умолчанию
const uint16_t defRelayAutoRelease = 0; // Время в секундах до автоотключения реле по умолчанию (0 - нет автоотключения)

const int8_t defBtnPin = -1; // Пин кнопки по умолчанию (-1 - не подключено)
const bool defBtnLevel = HIGH; // Уровень кнопки при замыкании по умолчанию
const bool defBtnTrigger = false; // Является ли кнопка фиксируемым выключателем по умолчанию
const uint16_t defDebounceTime = 20; // Время стабилизации уровня в миллисекундах для борьбы с дребезгом по умолчанию (0 - не используется)

const char pathGet[] PROGMEM = "/get";
const char pathSet[] PROGMEM = "/set";
const char pathRelayConfig[] PROGMEM = "/relay"; // Путь до страницы настройки параметров реле
const char pathControlConfig[] PROGMEM = "/control"; // Путь до страницы настройки параметров кнопок
const char pathSwitch[] PROGMEM = "/switch"; // Путь до страницы управления переключением реле
#ifdef MAX_SCHEDULES
const char pathScheduleConfig[] PROGMEM = "/schedule"; // Путь до страницы настройки параметров расписания
const char pathGetSchedule[] PROGMEM = "/getschedule"; // Путь до страницы, возвращающей JSON-пакет элемента расписания
const char pathSetSchedule[] PROGMEM = "/setschedule"; // Путь до страницы изменения элемента расписания
#endif
const char pathLastKey[] PROGMEM = "/lastkey"; // Путь до страницы, возвращающей JSON-пакет последнего считанного ключа
const char pathKeysConfig[] PROGMEM = "/keys"; // Путь до страницы настройки зарегистрированных ключей
const char pathGetKey[] PROGMEM = "/getkey"; // Путь до страницы, возвращающей JSON-пакет параметров ключа
const char pathSetKey[] PROGMEM = "/setkey"; // Путь до страницы изменения ключа
#ifdef USEDS1820
const char pathSensorConfig[] PROGMEM = "/sensor"; // Путь до страницы настройки параметров сенсоров
#endif

// Имена параметров для Web-форм
const char paramId[] PROGMEM = "id";
const char paramOn[] PROGMEM = "on";
const char paramFrom[] PROGMEM = "from";
const char paramRelayGPIO[] PROGMEM = "relaygpio";
const char paramRelayLevel[] PROGMEM = "relaylevel";
const char paramRelayOnBoot[] PROGMEM = "relayonboot";
const char paramRelayAutoRelease[] PROGMEM = "relayautorelease";
const char paramRelayName[] PROGMEM = "relayname";
const char paramBtnGPIO[] PROGMEM = "btngpio";
const char paramBtnLevel[] PROGMEM = "btnlevel";
const char paramBtnTrigger[] PROGMEM = "btntrigger";
const char paramBtnDebounce[] PROGMEM = "btndebounce";
#ifdef MAX_SCHEDULES
const char paramSchedulePeriod[] PROGMEM = "period";
const char paramScheduleHour[] PROGMEM = "hour";
const char paramScheduleMinute[] PROGMEM = "minute";
const char paramScheduleSecond[] PROGMEM = "second";
const char paramScheduleWeekdays[] PROGMEM = "weekdays";
const char paramScheduleDay[] PROGMEM = "day";
const char paramScheduleMonth[] PROGMEM = "month";
const char paramScheduleYear[] PROGMEM = "year";
const char paramScheduleRelay[] PROGMEM = "relay";
const char paramScheduleAction[] PROGMEM = "action";
#endif
const char paramKey[] PROGMEM = "key";
const char paramKeyMaster[] PROGMEM = "master";
const char paramKeyRelay[] PROGMEM = "relay";
const char paramKeyAction[] PROGMEM = "action";
#ifdef USEDS1820
const char paramDSGPIO[] PROGMEM = "dsgpio";
const char paramDSMinTemp[] PROGMEM = "dsmin_";
const char paramDSMaxTemp[] PROGMEM = "dsmax_";
const char paramDSMinTempRelay[] PROGMEM = "dsminrelay";
const char paramDSMinTempAction[] PROGMEM = "dsminaction";
const char paramDSMaxTempRelay[] PROGMEM = "dsmaxrelay";
const char paramDSMaxTempAction[] PROGMEM = "dsmaxaction";
#endif

// Имена JSON-переменных
const char jsonRelay[] PROGMEM = "relay";
const char jsonRelayAutoRelease[] PROGMEM = "relauto";
const char jsonUsedGPIO[] PROGMEM = "usedgpio";
const char jsonKey[] PROGMEM = "key";
#ifdef USEDS1820
const char jsonDSTemp[] PROGMEM = "dstemp";
#endif

// Названия топиков для MQTT
const char mqttRelayTopic[] PROGMEM = "/Relay";
const char mqttRFIDTopic[] PROGMEM = "/RFID";
#ifdef USEDS1820
const char mqttDSTempTopic[] PROGMEM = "/DS1820";
#endif

const char strNone[] PROGMEM = "(None)";
const char strWrongRelay[] PROGMEM = "Wrong relay index!";

const char attrAlignLeft[] PROGMEM = "align=\"left\"";
const char attrAlignCenter[] PROGMEM = "align=\"center\"";
const char attrAlignRight[] PROGMEM = "align=\"right\"";

class EthRFIDRelayApp : public EthWebServerApp {
public:
#ifdef MAX_SCHEDULES
#ifdef USEDS1820
  EthRFIDRelayApp() : EthWebServerApp(4), rfid(MFRC522(RFID_SS_PIN, RFID_RST_PIN)) {}
#else
  EthRFIDRelayApp() : EthWebServerApp(3), rfid(MFRC522(RFID_SS_PIN, RFID_RST_PIN)) {}
#endif // #ifdef USEDS1820
#else // #ifdef MAX_SCHEDULES
#ifdef USEDS1820
  EthRFIDRelayApp() : EthWebServerApp(3), rfid(MFRC522(RFID_SS_PIN, RFID_RST_PIN)) {}
#else
  EthRFIDRelayApp() : EthWebServerApp(2), rfid(MFRC522(RFID_SS_PIN, RFID_RST_PIN)) {}
#endif // #ifdef USEDS1820
#endif // #ifdef MAX_SCHEDULES

  void restart();

  void setup();
  void loop();

protected:
  enum action_t : uint8_t { ACTION_OFF, ACTION_ON, ACTION_TOGGLE };

  void defaultParams(uint8_t chunk);
#ifdef USEDS1820
  bool setParameter(const char* name, const char* value);
#endif
  int16_t readParamsData(uint16_t offset, uint8_t chunk);
  int16_t writeParamsData(uint16_t offset, uint8_t chunk);

  void handleRequest(const char* uri);
  void handleGet();
  void handleSet();
  void handleRootPage();
  void handleJsonData(uint8_t src);
  void handleRelayConfig();
  void handleControlConfig();
  void handleRelaySwitch();
#ifdef MAX_SCHEDULES
  void handleScheduleConfig();
  void handleGetSchedule();
  void handleSetSchedule();
#endif
  void handleLastKey();
  void handleKeysConfig();
  void handleGetKey();
  void handleSetKey();
#ifdef USEDS1820
  void handleSensorConfig();
#endif

  void btnNavigator();
  void btnRelay();
  void btnControl();
#ifdef MAX_SCHEDULES
  void btnSchedule();
#endif
  void btnKeys();
#ifdef USEDS1820
  void btnSensor();
#endif

  void mqttResubscribe();
  void mqttCallback(const char* topic, const uint8_t* payload, uint16_t length);

  void switchRelay(uint8_t id, bool on);
  void toggleRelay(uint8_t id);

  void listUsedGPIO();
  void listGPIO(const char* name, bool withNone, int8_t current = -1) {
    _listGPIO(name, withNone, current, false);
  }
  void listGPIO(const __FlashStringHelper* name, bool withNone, int8_t current = -1) {
    _listGPIO((PGM_P)name, withNone, current, true);
  }
  void listRelays(const char* name, uint8_t current = 0) {
    _listRelays(name, current, false);
  }
  void listRelays(const __FlashStringHelper* name, uint8_t current = 0) {
    _listRelays((PGM_P)name, current, true);
  }
  void listActions(const char* name, action_t current = ACTION_OFF) {
    _listActions(name, current, false);
  }
  void listActions(const __FlashStringHelper* name, action_t current = ACTION_OFF) {
    _listActions((PGM_P)name, current, true);
  }

private:
  bool debounceRead(uint8_t id, uint32_t debounceTime);

#ifdef MAX_SCHEDULES
  int16_t readScheduleConfig(uint16_t offset);
  int16_t writeScheduleConfig(uint16_t offset);
#endif

  void _listGPIO(const char* name, bool withNone, int8_t current, bool progmem);
  void _listRelays(const char* name, uint8_t current, bool progmem);
  void _listActions(const char* name, action_t current, bool progmem);

  static void clearKey(uint8_t* key);
  static void copyKey(uint8_t* dest, const uint8_t* src, uint8_t len);
  static bool compareKeys(const uint8_t* key1, const uint8_t* key2);
  static bool isKeyEmpty(const uint8_t* key);
  static char* keyToStr(char* str, const uint8_t* key);
  static void strToKey(const char* str, uint8_t* key);

  void publishKey();

  static const uint8_t RELAY_NAME_SIZE = 16;

  struct relay_t {
    int8_t relayPin; // Пин, к которому подключено реле (-1 - не подключено)
    int8_t btnPin; // Пин, к которому подключена кнопка (-1 - не подключено)
    struct {
      bool relayLevel : 1; // Уровень срабатывания реле
      bool relayOnBoot : 1; // Состояние реле при старте модуля
      bool btnLevel : 1; // Уровень на цифровом входе при замыкании кнопки
      bool btnTrigger : 1; // Является ли кнопка фиксируемым выключателем
    };
    uint16_t relayAutoRelease; // Значения задержки реле в секундах до автоотключения (0 - нет автоотключения)
    uint16_t btnDebounceTime; // Длительность в миллисекундах для подавления дребезга (0 - не используется, например для сенсорных кнопок)
    char relayName[RELAY_NAME_SIZE]; // Название реле
  };

  relay_t relays[MAX_RELAYS];
  uint32_t _autoRelease[MAX_RELAYS]; // Значения в миллисекундах для сравнения с millis(), когда реле должно отключиться автоматически (0 - нет автоотключения)

  struct target_t {
    uint8_t relay : 6;
    action_t action : 2;
  };

#ifdef MAX_SCHEDULES
  Schedule schedules[MAX_SCHEDULES]; // Массив расписания событий
  target_t scheduleRelay[MAX_SCHEDULES]; // Какой канал реле и что с ним делать по срабатыванию события
#endif

  struct key_t {
    uint8_t uid[UID_LENGTH];
    struct {
      uint8_t relay : 6;
      action_t action : 2;
    };
  };

  key_t keys[MAX_KEYS];
  uint8_t masterKey[UID_LENGTH];
  uint8_t lastKey[UID_LENGTH]; // UID последнего успешно считанного ключа

  MFRC522 rfid;

#ifdef USEDS1820
  struct trigger_t {
    bool minTriggered : 1;
    bool maxTriggered : 1;
  };

  int8_t dsPin; // Пин, к которому подключен датчик DS1820
  float dsMinTemp[MAX_DS1820], dsMaxTemp[MAX_DS1820]; // Минимальное и максимальное значение температуры срабатывания реле
  target_t dsMinTempTarget[MAX_DS1820], dsMaxTempTarget[MAX_DS1820]; // Какой канал реле и что с ним делать по срабатыванию события

  float dsTemperature[MAX_DS1820]; // Значение успешно прочитанной температуры
  uint32_t dsReadTime; // Время в миллисекундах, после которого можно считывать новое значение температуры
  trigger_t dsTempTriggered[MAX_DS1820]; // Было ли срабатывание реле по порогу температуры?

  DS1820 *ds;
#endif
};

/***
 * EthRFIDRelayApp class implementation
 */

void EthRFIDRelayApp::restart() {
  digitalWrite(RFID_SS_PIN, HIGH); // Deactivate RFID
  digitalWrite(ETH_SS_PIN, HIGH); // Deactivate Ethernet
  SPI.end();
  Serial.println(F("Deactivate SPI bus"));

  EthWebServerApp::restart();
}

void EthRFIDRelayApp::setup() {
  EthWebServerApp::setup();

  for (uint8_t i = 0; i < MAX_RELAYS; ++i) {
    _autoRelease[i] = 0;

    if (relays[i].relayPin != -1) {
      digitalWrite(relays[i].relayPin, relays[i].relayLevel == relays[i].relayOnBoot);
      pinMode(relays[i].relayPin, OUTPUT);
    }

    if (relays[i].btnPin != -1)
      pinMode(relays[i].btnPin, relays[i].btnLevel ? INPUT : INPUT_PULLUP);
  }

#ifdef LED_PIN
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
#endif

  rfid.PCD_Init();
  clearKey(lastKey);

#ifdef USEDS1820
  if (dsPin != -1) {
    ds = new DS1820(dsPin);
    uint8_t cnt = ds->find();
    if (! cnt) {
      delete ds;
      ds = NULL;
      _log.println(F("DS18x20 sensors not detected!"));
    } else {
      ds->update();
      dsReadTime = millis() + DS1820::MEASURE_TIME;
      for (uint8_t i = 0; i < MAX_DS1820; ++i) {
        dsTemperature[i] = NAN;
        dsTempTriggered[i].minTriggered = false;
        dsTempTriggered[i].maxTriggered = false;
      }
      _log.print(cnt);
      _log.println(F(" DS18x20 sensor(s) detected"));
    }
  }
#endif
}

void EthRFIDRelayApp::loop() {
  EthWebServerApp::loop();

  for (uint8_t i = 0; i < MAX_RELAYS; ++i) {
    if ((relays[i].relayPin != -1) && _autoRelease[i] && (millis() > _autoRelease[i])) {
      logDateTime(F("Relay #"));
      _log.print(i + 1);
      _log.println(F(" auto off"));
      switchRelay(i, false);
      _autoRelease[i] = 0;
    }

    if (relays[i].btnPin != -1) {
      static bool btnLast[MAX_RELAYS];
      bool btnPressed = debounceRead(i, relays[i].btnDebounceTime);

      if (btnPressed != btnLast[i]) {
        if (relays[i].btnTrigger) { // Switch
          switchRelay(i, btnPressed);
        } else { // Button
          if (btnPressed)
            toggleRelay(i);
        }
        logDateTime(F("Button["));
        _log.print(i + 1);
        _log.print(F("] "));
        if (btnPressed)
          _log.println(F("pressed"));
        else
          _log.println(F("released"));
        btnLast[i] = btnPressed;
      }
    }
  }

#ifdef MAX_SCHEDULES
  uint32_t now = getTime();
  if (now) {
    for (uint8_t i = 0; i < MAX_SCHEDULES; ++i) {
      if (schedules[i].period() != Schedule::NONE) {
        if (schedules[i].check(now)) {
          if (scheduleRelay[i].action == ACTION_TOGGLE)
            toggleRelay(scheduleRelay[i].relay);
          else
            switchRelay(scheduleRelay[i].relay, scheduleRelay[i].action == ACTION_ON);
          logDateTime(F("Schedule \""), now);
          {
            char scheduleStr[80];

            _log.print(schedules[i].toString(scheduleStr));
          }
          _log.print(F("\" turned relay #"));
          _log.print(scheduleRelay[i].relay + 1);
          if (scheduleRelay[i].action == ACTION_TOGGLE)
            _log.println(F(" opposite"));
          else
            _log.println(scheduleRelay[i].action == ACTION_ON ? F(" on") : F(" off"));
        }
      }
    }
  }
#endif

  static const uint32_t RFID_TIMEOUT = 100; // 100 ms.
  static uint32_t nextCardRead = 0;
  static bool masterMode = false;
#ifdef LED_PIN
  static const uint32_t MASTER_BLINK_PULSE = 250; // 250 ms.
  static uint32_t masterBlinkTime;
#endif
  uint32_t ms = millis();

#ifdef LED_PIN
  if (masterMode) {
    if (ms > masterBlinkTime) {
      digitalWrite(LED_PIN, ! digitalRead(LED_PIN));
      masterBlinkTime = ms + MASTER_BLINK_PULSE;
    }
  }
#endif

  if ((ms > nextCardRead) && rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    uint16_t i;

    copyKey(lastKey, rfid.uid.uidByte, rfid.uid.size);
    rfid.PICC_HaltA();
    logDateTime(F("Detected RFID key with UID "));
    {
      char keystr[UID_LENGTH * 2 + 1];

      _log.println(keyToStr(keystr, lastKey));
    }
    publishKey();
    if (compareKeys(masterKey, lastKey)) {
      _log.println(F("Master key detected"));
      if (masterMode) {
        masterMode = false;
#ifdef LED_PIN
        digitalWrite(LED_PIN, LOW);
        masterBlinkTime = 0;
#endif
        logDateTime(F("Master mode ended"), 0, true);
      } else {
        masterMode = true;
        logDateTime(F("Master mode started"), 0, true);
      }
    } else {
      for (i = 0; i < MAX_KEYS; ++i) {
        if (compareKeys(keys[i].uid, lastKey)) {
          if (masterMode) {
            clearKey(keys[i].uid);
            keys[i].relay = 0;
            keys[i].action = ACTION_OFF;
            writeParams();
            _log.println(F("Old key was deleted"));
          } else {
            if (keys[i].action == ACTION_TOGGLE)
              toggleRelay(keys[i].relay);
            else
              switchRelay(keys[i].relay, keys[i].action == ACTION_ON);
            _log.print(F("Valid key turned relay #"));
            _log.print(keys[i].relay + 1);
            if (keys[i].action == ACTION_TOGGLE)
              _log.println(F(" opposite"));
            else
              _log.println(keys[i].action == ACTION_ON ? F(" on") : F(" off"));
          }
          break;
        }
      }
      if (i == MAX_KEYS) { // Key not found
        if (masterMode) {
          for (i = 0; i < MAX_KEYS; ++i) {
            if (isKeyEmpty(keys[i].uid)) {
              copyKey(keys[i].uid, lastKey, UID_LENGTH);
              keys[i].relay = 0;
              keys[i].action = ACTION_TOGGLE;
              writeParams();
              _log.println(F("New key was added"));
              break;
            }
          }
          if (i == MAX_KEYS)
            _log.println(F("No space for new key!"));
        } else {
          if (isKeyEmpty(masterKey)) {
            copyKey(masterKey, lastKey, UID_LENGTH);
            writeParams();
            masterMode = true;
            _log.println(F("New master key was defined"));
            logDateTime(F("Master mode started"), 0, true);
          } else
            _log.println(F("Unknown key!"));
        }
      }
    }
    nextCardRead = ms + RFID_TIMEOUT;
  }

#ifdef USEDS1820
  const float DS1820_TOLERANCE = 0.2;

  if ((dsPin != -1) && ds) {
    if (millis() >= dsReadTime) {
      float v;

      for (uint8_t i = 0; i < MAX_DS1820; ++i) {
        if (! ds->isValid(i))
          break;
        v = ds->readTemperature(i);
        ds->update(i);
        if ((! isnan(v)) && (v >= -50.0) && (v <= 50.0)) {
          if (isnan(dsTemperature[i]) || (abs(dsTemperature[i] - v) > DS1820_TOLERANCE)) {
            dsTemperature[i] = v;
            if (_pubSubClient && _pubSubClient->connected()) {
              char topic[80];
              char value[16];

              strcpy_P(topic, mqttDSTempTopic);
              strcat_P(topic, strSlash);
              strcat(topic, itoa(i + 1, value, 10));
              dtostrf(dsTemperature[i], 6, 2, value);
              mqttPublish(topic, value);
            }
            if (! isnan(dsMinTemp[i])) {
              if (dsTemperature[i] < dsMinTemp[i]) {
                if (! dsTempTriggered[i].minTriggered) {
                  if (dsMinTempTarget[i].action == ACTION_TOGGLE)
                    toggleRelay(dsMinTempTarget[i].relay);
                  else
                    switchRelay(dsMinTempTarget[i].relay, dsMinTempTarget[i].action == ACTION_ON);
                  dsTempTriggered[i].minTriggered = true;
                  logDateTime(F("DS1820["));
                  _log.print(i + 1);
                  _log.println(F("] minimal temperature triggered"));
                }
              } else
                dsTempTriggered[i].minTriggered = false;
            }
            if (! isnan(dsMaxTemp[i])) {
              if (dsTemperature[i] > dsMaxTemp[i]) {
                if (! dsTempTriggered[i].maxTriggered) {
                  if (dsMaxTempTarget[i].action == ACTION_TOGGLE)
                    toggleRelay(dsMaxTempTarget[i].relay);
                  else
                    switchRelay(dsMaxTempTarget[i].relay, dsMaxTempTarget[i].action == ACTION_ON);
                  dsTempTriggered[i].maxTriggered = true;
                  logDateTime(F("DS1820["));
                  _log.print(i + 1);
                  _log.println(F("] maximal temperature triggered"));
                }
              } else
                dsTempTriggered[i].maxTriggered = false;
            }
          }
        } else {
          logDateTime(F("DS1820["));
          _log.print(i + 1);
          _log.println(F("] temperature read error!"));
        }
      }
      dsReadTime = millis() + 1000; // DS1820::MEASURE_TIME;
    }
  }
#endif
}

void EthRFIDRelayApp::defaultParams(uint8_t chunk) {
  EthWebServerApp::defaultParams(chunk);

  if (chunk < 5) {
    for (uint8_t i = 0; i < MAX_RELAYS; ++i) {
      relays[i].relayPin = defRelayPin;
      relays[i].btnPin = defBtnPin;
      relays[i].relayLevel = defRelayLevel;
      relays[i].relayOnBoot = defRelayOnBoot;
      relays[i].btnLevel = defBtnLevel;
      relays[i].btnTrigger = defBtnTrigger;
      relays[i].relayAutoRelease = defRelayAutoRelease;
      relays[i].btnDebounceTime = defDebounceTime;
      memset(relays[i].relayName, 0, sizeof(relays[i].relayName));
    }
  }
#ifdef MAX_SCHEDULES
  if (chunk < 6) {
    for (uint8_t i = 0; i < MAX_SCHEDULES; ++i) {
      schedules[i].clear();
      scheduleRelay[i].relay = 0;
      scheduleRelay[i].action = ACTION_OFF;
    }
  }
#endif
#ifdef MAX_SCHEDULES
  if (chunk < 7) {
#else
  if (chunk < 6) {
#endif
    memset(keys, 0, sizeof(keys));
    clearKey(masterKey);
  }
#ifdef USEDS1820
#ifdef MAX_SCHEDULES
  if (chunk < 8) {
#else
  if (chunk < 7) {
#endif
    dsPin = -1;
    for (uint8_t i = 0; i < MAX_DS1820; ++i) {
      dsMinTemp[i] = NAN;
      dsMaxTemp[i] = NAN;
      dsMinTempTarget[i].relay = 0;
      dsMinTempTarget[i].action = ACTION_OFF;
      dsMaxTempTarget[i].relay = 0;
      dsMaxTempTarget[i].action = ACTION_OFF;
    }
  }
#endif
}

#ifdef USEDS1820
bool EthRFIDRelayApp::setParameter(const char* name, const char* value) {
  bool result = EthWebServerApp::setParameter(name, value);

  if (! result) {
    int16_t ind = strlen(name);

    if ((ind > 1) && ((name[ind - 1] >= '0') && (name[ind - 1] <= '9'))) { // name ends with digit
      --ind;
      while ((ind > 1) && ((name[ind - 1] >= '0') && (name[ind - 1] <= '9')))
        --ind;
      ind = atoi(&name[ind]);
    } else
      ind = -1;
    result = true;
    if (equals_P(name, paramDSGPIO))
      dsPin = constrain(atoi(value), -1, MAX_PIN_NO);
    else if (startsWith_P(name, paramDSMinTemp)) {
      if ((ind >= 0) && (ind < MAX_DS1820)) {
        dsMinTemp[ind] = *value ? atof(value) : NAN;
      }
    } else if (startsWith_P(name, paramDSMaxTemp)) {
      if ((ind >= 0) && (ind < MAX_DS1820)) {
        dsMaxTemp[ind] = *value ? atof(value) : NAN;
      }
    } else if (startsWith_P(name, paramDSMinTempRelay)) {
      if ((ind >= 0) && (ind < MAX_DS1820))
        dsMinTempTarget[ind].relay = constrain(atoi(value), 0, MAX_RELAYS - 1);
    } else if (startsWith_P(name, paramDSMinTempAction)) {
      if ((ind >= 0) && (ind < MAX_DS1820))
        dsMinTempTarget[ind].action = (action_t)atoi(value);
    } else if (startsWith_P(name, paramDSMaxTempRelay)) {
      if ((ind >= 0) && (ind < MAX_DS1820))
        dsMaxTempTarget[ind].relay = constrain(atoi(value), 0, MAX_RELAYS - 1);
    } else if (startsWith_P(name, paramDSMaxTempAction)) {
      if ((ind >= 0) && (ind < MAX_DS1820))
        dsMaxTempTarget[ind].action = (action_t)atoi(value);
    } else
      result = false;
  }

  return result;
}
#endif

int16_t EthRFIDRelayApp::readParamsData(uint16_t offset, uint8_t chunk) {
  if (chunk < 4)
    return EthWebServerApp::readParamsData(offset, chunk);

  int16_t result = offset;

  if (chunk == 4) {
    result += _readData(result, relays, sizeof(relays));
  } else
#ifdef MAX_SCHEDULES
  if (chunk == 5) {
    result += readScheduleConfig(result);
  } else
#endif
#ifdef MAX_SCHEDULES
  if (chunk == 6) {
#else
  if (chunk == 5) {
#endif
    result += _readData(result, keys, sizeof(keys));
    result += _readData(result, masterKey, sizeof(masterKey));
  } else
#ifdef USEDS1820
#ifdef MAX_SCHEDULES
  if (chunk == 7) {
#else
  if (chunk == 6) {
#endif
#ifdef USEDS1820
    result += _readData(result, &dsPin, sizeof(dsPin));
    result += _readData(result, &dsMinTemp, sizeof(dsMinTemp));
    result += _readData(result, &dsMaxTemp, sizeof(dsMaxTemp));
    result += _readData(result, &dsMinTempTarget, sizeof(dsMinTempTarget));
    result += _readData(result, &dsMaxTempTarget, sizeof(dsMaxTempTarget));
#endif
  } else
#endif
    result = -1;

  return result;
}

int16_t EthRFIDRelayApp::writeParamsData(uint16_t offset, uint8_t chunk) {
  if (chunk < 4)
    return EthWebServerApp::writeParamsData(offset, chunk);

  int16_t result = offset;

  if (chunk == 4) {
    result += _writeData(result, relays, sizeof(relays));
  } else
#ifdef MAX_SCHEDULES
  if (chunk == 5) {
    result += writeScheduleConfig(result);
  } else
#endif
#ifdef MAX_SCHEDULES
  if (chunk == 6) {
#else
  if (chunk == 5) {
#endif
    result += _writeData(result, keys, sizeof(keys));
    result += _writeData(result, masterKey, sizeof(masterKey));
  } else
#ifdef USEDS1820
#ifdef MAX_SCHEDULES
  if (chunk == 7) {
#else
  if (chunk == 6) {
#endif
#ifdef USEDS1820
    result += _writeData(result, &dsPin, sizeof(dsPin));
    result += _writeData(result, &dsMinTemp, sizeof(dsMinTemp));
    result += _writeData(result, &dsMaxTemp, sizeof(dsMaxTemp));
    result += _writeData(result, &dsMinTempTarget, sizeof(dsMinTempTarget));
    result += _writeData(result, &dsMaxTempTarget, sizeof(dsMaxTempTarget));
#endif
  } else
#endif
    result = -1;

  return result;
}

void EthRFIDRelayApp::handleRequest(const char* uri) {
  if (equals_P(uri, pathGet))
    handleGet();
  else if (equals_P(uri, pathSet))
    handleSet();
  else if (equals_P(uri, pathRelayConfig))
    handleRelayConfig();
  else if (equals_P(uri, pathControlConfig))
    handleControlConfig();
  else if (equals_P(uri, pathSwitch))
    handleRelaySwitch();
  else
#ifdef MAX_SCHEDULES
  if (equals_P(uri, pathScheduleConfig))
    handleScheduleConfig();
  else if (equals_P(uri, pathGetSchedule))
    handleGetSchedule();
  else if (equals_P(uri, pathSetSchedule))
    handleSetSchedule();
  else
#endif
  if (equals_P(uri, pathLastKey))
    handleLastKey();
  else if (equals_P(uri, pathKeysConfig))
    handleKeysConfig();
  else if (equals_P(uri, pathGetKey))
    handleGetKey();
  else if (equals_P(uri, pathSetKey))
    handleSetKey();
  else
#ifdef USEDS1820
  if (equals_P(uri, pathSensorConfig))
    handleSensorConfig();
  else
#endif
    EthWebServerApp::handleRequest(uri);
}

void EthRFIDRelayApp::handleGet() {
  if (! startResponse(200, FPSTR(typeTextJson)))
    return;

  _client->write('{');
  if (hasArg(FPSTR(paramSrc))) {
    uint8_t src = atoi(arg(FPSTR(paramSrc)));

    if (src == 0) { // Relay
      int8_t id = -1;

      if (hasArg(FPSTR(paramId)))
        id = atoi(arg(FPSTR(paramId)));

      if ((id >= 0) && (id < MAX_RELAYS)) {
        printf(*_client, F("\"%S\":%d,\"%S\":%d,\"%S\":%d,\"%S\":%u,\"%S\":\"%s\""), FPSTR(paramRelayGPIO), relays[id].relayPin,
          FPSTR(paramRelayLevel), relays[id].relayLevel, FPSTR(paramRelayOnBoot), relays[id].relayOnBoot, FPSTR(paramRelayAutoRelease), relays[id].relayAutoRelease,
          FPSTR(paramRelayName), relays[id].relayName);
        printf(*_client, F(",\"%S\":["), FPSTR(jsonUsedGPIO));
        listUsedGPIO();
        _client->write(']');
      }
    } else if (src == 1) { // Control
      int8_t id = -1;

      if (hasArg(FPSTR(paramId)))
        id = atoi(arg(FPSTR(paramId)));

      if ((id >= 0) && (id < MAX_RELAYS)) {
        printf(*_client, F("\"%S\":%d,\"%S\":%d,\"%S\":%d,\"%S\":%u"), FPSTR(paramBtnGPIO), relays[id].btnPin,
          FPSTR(paramBtnLevel), relays[id].btnLevel, FPSTR(paramBtnTrigger), relays[id].btnTrigger, FPSTR(paramBtnDebounce), relays[id].btnDebounceTime);
        printf(*_client, F(",\"%S\":["), FPSTR(jsonUsedGPIO));
        listUsedGPIO();
        _client->write(']');
      }
    }
  }
  _client->write('}');
  endResponse();
}

void EthRFIDRelayApp::handleSet() {
  char* _argName;
  char* _argValue;
  int8_t id = -1;
  char* from = NULL;

  if (hasArg(FPSTR(paramId)))
    id = atoi(arg(FPSTR(paramId)));
  if (hasArg(FPSTR(paramFrom)))
    from = (char*)arg(FPSTR(paramFrom));

  if ((id >= 0) && (id < MAX_RELAYS)) {
    for (uint8_t i = 0; i < args(); ++i) {
      _argName = (char*)argName(i);
      _argValue = (char*)arg(i);

      if (equals_P(_argName, paramRelayGPIO))
        relays[id].relayPin = constrain(atoi(_argValue), -1, MAX_PIN_NO);
      else if (equals_P(_argName, paramRelayLevel))
        relays[id].relayLevel = constrain(atoi(_argValue), 0, 1);
      else if (equals_P(_argName, paramRelayOnBoot))
        relays[id].relayOnBoot = constrain(atoi(_argValue), 0, 1);
      else if (equals_P(_argName, paramRelayAutoRelease))
        relays[id].relayAutoRelease = max(0, atol(_argValue));
      else if (equals_P(_argName, paramBtnGPIO))
        relays[id].btnPin = constrain(atoi(_argValue), -1, MAX_PIN_NO);
      else if (equals_P(_argName, paramBtnLevel))
        relays[id].btnLevel = constrain(atoi(_argValue), 0, 1);
      else if (equals_P(_argName, paramBtnTrigger))
        relays[id].btnTrigger = constrain(atoi(_argValue), 0, 1);
      else if (equals_P(_argName, paramBtnDebounce))
        relays[id].btnDebounceTime = max(0, atol(_argValue));
      else if (equals_P(_argName, paramRelayName)) {
        memset(relays[id].relayName, 0, sizeof(relays[id].relayName));
        strncpy(relays[id].relayName, _argValue, sizeof(relays[id].relayName) - 1);
      }
    }
  }

  if (! startResponse(200, FPSTR(typeTextHtml)))
    return;
  htmlPageStart(F("Store Parameters"));
  if (from)
    printf(*_client, F("<meta http-equiv=\"refresh\" content=\"1;URL=%s\">\n"), from);
  htmlPageStdStyle();
  htmlPageBody();

  if ((id >= 0) && (id < MAX_RELAYS)) {
    _client->print(F("Parameters updated successfully.\n"));
  } else {
    _client->print(F("Nothing to update!\n"));
  }
  htmlPageEnd();
  endResponse();
}

void EthRFIDRelayApp::handleRootPage() {
#ifdef USEAUTHORIZATION
  if (! authenticate(authUser, authPassword))
    return requestAuthentication();
#endif

  if (! startResponse(200, FPSTR(typeTextHtml)))
    return;

  htmlPageStart(F("Arduino Mega Relay"));
  htmlPageStyleStart();
  htmlPageStdStyle(false);
  _client->print(F(".checkbox{\n\
vertical-align:top;\n\
margin:0 3px 0 0;\n\
width:17px;\n\
height:17px;\n\
}\n\
.checkbox+label{\n\
cursor:pointer;\n\
}\n\
.checkbox:not(checked){\n\
position:absolute;\n\
opacity:0;\n\
}\n\
.checkbox:not(checked)+label{\n\
position:relative;\n\
padding:0 0 0 60px;\n\
}\n\
.checkbox:not(checked)+label:before{\n\
content:'';\n\
position:absolute;\n\
top:-4px;\n\
left:0;\n\
width:50px;\n\
height:26px;\n\
border-radius:13px;\n\
background:#CDD1DA;\n\
box-shadow:inset 0 2px 3px rgba(0,0,0,.2);\n\
}\n\
.checkbox:not(checked)+label:after{\n\
content:'';\n\
position:absolute;\n\
top:-2px;\n\
left:2px;\n\
width:22px;\n\
height:22px;\n\
border-radius:10px;\n\
background:#FFF;\n\
box-shadow:0 2px 5px rgba(0,0,0,.3);\n\
transition:all .2s;\n\
}\n\
.checkbox:checked+label:before{\n\
background:#9FD468;\n\
}\n\
.checkbox:checked+label:after{\n\
left:26px;\n\
}\n"));
  htmlPageStyleEnd();
  htmlPageScriptStart();
  htmlPageStdScript(false);
  printf(*_client, F("function refreshData(){\n\
var request=getXmlHttpRequest();\n\
request.open('GET','%S?src=0&dummy='+Date.now(),true);\n"), FPSTR(pathJson));
  _client->print(F("request.onreadystatechange=function(){\n\
if(request.readyState==4){\n\
var data=JSON.parse(request.responseText);\n\
var tm,uptime='';\n"));
  printf(*_client, F("if(data.%S>=86400)\n\
uptime=parseInt(data.%S/86400)+' day(s) ';\n"), FPSTR(jsonUptime), FPSTR(jsonUptime));
  printf(*_client, F("tm=parseInt(data.%S%%86400/3600);\n\
if(tm<10)\n\
uptime+='0';\n\
uptime+=tm+':';\n"), FPSTR(jsonUptime));
  printf(*_client, F("tm=parseInt(data.%S%%3600/60);\n\
if(tm<10)\n\
uptime+='0';\n\
uptime+=tm+':';\n"), FPSTR(jsonUptime));
  printf(*_client, F("tm=parseInt(data.%S%%60);\n\
if(tm<10)\n\
uptime+='0';\n\
uptime+=tm;\n"), FPSTR(jsonUptime));
  printf(*_client, F("document.getElementById('%S').innerHTML=uptime;\n"), FPSTR(jsonUptime));
  printf(*_client, F("document.getElementById('%S').innerHTML=data.%S;\n"), FPSTR(jsonFreeMem), FPSTR(jsonFreeMem));
  printf(*_client, F("document.getElementById('%S').innerHTML=(data.%S!=true)?\"not \":\"\";\n"),
    FPSTR(jsonMQTTConnected), FPSTR(jsonMQTTConnected));
  printf(*_client, F("for(var i=0;i<data.%S.length;i++){\n\
document.getElementById('%S'+i).checked=data.%S[i];\n"), FPSTR(jsonRelay), FPSTR(jsonRelay), FPSTR(jsonRelay));
  printf(*_client, F("if(data.%S[i]>0)\n\
document.getElementById('%S'+i).innerHTML=' ('+data.%S[i]+' sec. to auto off)';\n"), FPSTR(jsonRelayAutoRelease), FPSTR(jsonRelayAutoRelease), FPSTR(jsonRelayAutoRelease), FPSTR(jsonRelayAutoRelease));
  printf(*_client, F("else\n\
document.getElementById('%S'+i).innerHTML='';\n"), FPSTR(jsonRelayAutoRelease));
  _client->print(F("}\n"));
#ifdef USEDS1820
  if ((dsPin != -1) && ds) {
    printf(*_client, F("for(var i=0;i<data.%S.length;i++){\n\
document.getElementById('%S'+i).innerHTML=data.%S[i];\n\
}\n"), FPSTR(jsonDSTemp), FPSTR(jsonDSTemp), FPSTR(jsonDSTemp));
  }
#endif
  _client->print(F("}\n\
}\n\
request.send(null);\n\
}\n\
setInterval(refreshData,500);\n"));
  htmlPageScriptEnd();
  htmlPageBody();
  htmlTag(FPSTR(tagH3), F("Arduino Mega Relay"), true);
  printf(*_client, F("Uptime <span id=\"%S\">?</span>\n"), FPSTR(jsonUptime));
  htmlTagBR(true);
  printf(*_client, F("Free memory: <span id=\"%S\">?</span> bytes\n"), FPSTR(jsonFreeMem));
  htmlTagBR(true);
  printf(*_client, F("MQTT broker is <span id=\"%S\">not </span>connected\n"), FPSTR(jsonMQTTConnected));
#ifdef USEDS1820
  if ((dsPin != -1) && ds) {
    for (uint8_t i = 0; i < MAX_DS1820; ++i) {
      if (! ds->isValid(i))
        break;
      htmlTagBR(true);
      printf(*_client, F("Temperature #%d (DS18x20): <span id=\"%S%d\">?</span><sup>o</sup>C\n"), i + 1, FPSTR(jsonDSTemp), i);
    }
  }
#endif
  htmlTagSimple(FPSTR(tagP), true);
#if MAX_RELAYS > RELAYS_PER_COLUMN
  htmlTagStart(FPSTR(tagTable), F("cols=2 width=\"100%\""), true);
  htmlTagSimple(F("col"), F("span=2 width=\"50%\""), true);
#endif
  for (uint8_t id = 0; id < MAX_RELAYS; ++id) {
#if MAX_RELAYS > RELAYS_PER_COLUMN
    if (id % 2 == 0)
      htmlTagStart(FPSTR(tagTR));
    htmlTagStart(FPSTR(tagTD), F("height=30"));
#endif
    printf(*_client, F("<input type=\"checkbox\" class=\"checkbox\" id=\"%S%d\" onchange=\"openUrl('%S?%S=%d&%S='+this.checked+'&dummy='+Date.now())\""),
      FPSTR(jsonRelay), id, FPSTR(pathSwitch), FPSTR(paramId), id, FPSTR(paramOn));
    if (relays[id].relayPin == -1)
      _client->print(F(" disabled"));
    else {
      if (digitalRead(relays[id].relayPin) == relays[id].relayLevel)
        _client->print(F(" checked"));
    }
    printf(*_client, F(">\n\
<label for=\"%S%d\">"), FPSTR(jsonRelay), id);
    if (*relays[id].relayName)
      _client->print(relays[id].relayName);
    else
      printf(*_client, F("Relay %d"), id + 1);
    printf(*_client, F("</label>\n\
<span id=\"%S%d\"></span>"), FPSTR(jsonRelayAutoRelease), id);
#if MAX_RELAYS > RELAYS_PER_COLUMN
    htmlTagEnd(FPSTR(tagTD), (id % 2) == 0);
    if (id % 2)
      htmlTagEnd(FPSTR(tagTR), true);
#else
    htmlTagSimple(FPSTR(tagP), true);
#endif
  }
#if MAX_RELAYS > RELAYS_PER_COLUMN
  if (MAX_RELAYS % 2)
    htmlTagEnd(FPSTR(tagTR));
  htmlTagEnd(FPSTR(tagTable), true);
#endif
  btnNavigator();
  htmlPageEnd();
  endResponse();
}

void EthRFIDRelayApp::handleJsonData(uint8_t src) {
  EthWebServerApp::handleJsonData(src);

  if (src == 0) { // JSON packet for root page
    printf(*_client, F(",\"%S\":["), FPSTR(jsonRelay));
    for (uint8_t i = 0; i < MAX_RELAYS; ++i) {
      if (i)
        _client->write(',');
      if ((relays[i].relayPin != -1) && (digitalRead(relays[i].relayPin) == relays[i].relayLevel))
        _client->print(FPSTR(strTrue));
      else
        _client->print(FPSTR(strFalse));
    }
    printf(*_client, F("],\"%S\":["), FPSTR(jsonRelayAutoRelease));
    for (uint8_t i = 0; i < MAX_RELAYS; ++i) {
      if (i)
        _client->write(',');
      if (_autoRelease[i])
        _client->print((int32_t)(_autoRelease[i] - millis()) / 1000);
      else
        _client->print(0);
    }
    _client->write(']');
#ifdef USEDS1820
    if ((dsPin != -1) && ds) {
      _client->print(F(",\""));
      _client->print(FPSTR(jsonDSTemp));
      _client->print(F("\":["));
      for (uint8_t i = 0; i < MAX_DS1820; ++i) {
        if (! ds->isValid(i))
          break;
        if (i)
          _client->write(',');
        if (isnan(dsTemperature[i]))
          _client->print(F("\"?\""));
        else
          _client->print(dsTemperature[i], 2);
      }
      _client->write(']');
    }
#endif
  }
}

void EthRFIDRelayApp::handleRelayConfig() {
  static const char codePattern1[] PROGMEM = "form.%S.value=data.%S;\n";
  static const char codePattern2[] PROGMEM = "setRadios(form.%S,data.%S);\n";

#ifdef USEAUTHORIZATION
  if (! authenticate(authUser, authPassword))
    return requestAuthentication();
#endif

  if (! startResponse(200, FPSTR(typeTextHtml)))
    return;

  htmlPageStart(F("Relay Setup"));
  htmlPageStyleStart();
  htmlPageStdStyle(false);
  _client->print(F(".modal{\n\
display:none;\n\
position:fixed;\n\
z-index:1;\n\
left:0;\n\
top:0;\n\
width:100%;\n\
height:100%;\n\
overflow:auto;\n\
background-color:rgb(0,0,0);\n\
background-color:rgba(0,0,0,0.4);\n\
}\n\
.modal-content{\n\
background-color:#fefefe;\n\
margin:15% auto;\n\
padding:20px;\n\
border:1px solid #888;\n\
width:240px;\n\
}\n\
.close{\n\
color:#aaa;\n\
float:right;\n\
font-size:28px;\n\
font-weight:bold;\n\
}\n\
.close:hover,\n\
.close:focus{\n\
color:black;\n\
text-decoration:none;\n\
cursor:pointer;\n\
}\n\
.hidden{\n\
display:none;\n\
}\n"));
  htmlPageStyleEnd();
  htmlPageScriptStart();
  htmlPageStdScript(false);
  _client->print(F("function setRadios(radios,val){\n\
for(var i=0;i<radios.length;i++){\n\
if(radios[i].value==val)\n\
radios[i].checked=true;\n\
}\n\
}\n\
function indexOfArray(arr,val){\n\
for(var i=0;i<arr.length;i++){\n\
if(arr[i]==val)\n\
return i;\n\
}\n\
return -1;\n\
}\n\
function setSelect(sel,dis,val){\n\
for(var i=0;i<sel.options.length;i++){\n\
if(sel.options[i].value==val){\n\
sel.options[i].selected=true;\n\
sel.options[i].disabled=false;\n\
}else{\n\
sel.options[i].disabled=(indexOfArray(dis,sel.options[i].value)>=0);\n\
}\n\
}\n\
}\n"));
  printf(*_client, F("function loadData(form){\n\
var request=getXmlHttpRequest();\n\
request.open('GET','%S?%S=0&%S='+form.id.value+'&dummy='+Date.now(),false);\n"), FPSTR(pathGet), FPSTR(paramSrc), FPSTR(paramId));
  _client->print(F("request.send(null);\n\
if(request.status==200){\n\
var data=JSON.parse(request.responseText);\n"));
  printf(*_client, F("setSelect(form.%S,data.%S,data.%S);\n"), FPSTR(paramRelayGPIO), FPSTR(jsonUsedGPIO), FPSTR(paramRelayGPIO));
//  printf(*_client, FPSTR(codePattern1), FPSTR(paramRelayGPIO), FPSTR(paramRelayGPIO));
  printf(*_client, FPSTR(codePattern2), FPSTR(paramRelayLevel), FPSTR(paramRelayLevel));
  printf(*_client, FPSTR(codePattern2), FPSTR(paramRelayOnBoot), FPSTR(paramRelayOnBoot));
  printf(*_client, FPSTR(codePattern1), FPSTR(paramRelayAutoRelease), FPSTR(paramRelayAutoRelease));
  printf(*_client, FPSTR(codePattern1), FPSTR(paramRelayName), FPSTR(paramRelayName));
  _client->print(F("}\n\
}\n\
function openForm(form,id){\n\
form.id.value=id;\n\
loadData(form);\n\
document.getElementById('form').style.display='block';\n\
}\n\
function closeForm(){\n\
document.getElementById('form').style.display='none';\n\
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
  htmlTagStart(FPSTR(tagTable), F("cols=6"), true);
  htmlTag(FPSTR(tagCaption), F("<h3>Relay Setup</h3>"), true);
  htmlTagStart(FPSTR(tagTR));
  htmlTag(FPSTR(tagTH), F("#"));
  htmlTag(FPSTR(tagTH), F("GPIO"));
  htmlTag(FPSTR(tagTH), F("Level"));
  htmlTag(FPSTR(tagTH), F("On boot"));
  htmlTag(FPSTR(tagTH), F("Auto release<sup>*</sup>"));
  htmlTag(FPSTR(tagTH), F("Relay name"));
  htmlTagEnd(FPSTR(tagTR), true);
  for (uint8_t id = 0; id < MAX_RELAYS; ++id) {
    htmlTagStart(FPSTR(tagTR));
    htmlTagStart(FPSTR(tagTD), FPSTR(attrAlignCenter));
    printf(*_client, F("<a href=\"#\" onclick=\"openForm(document.form,%d)\">%d</a>"), id, id + 1);
    htmlTagEnd(FPSTR(tagTD));
    htmlTagStart(FPSTR(tagTD), FPSTR(attrAlignCenter));
    if (relays[id].relayPin != -1)
      _client->print(relays[id].relayPin);
    else
      _client->print(FPSTR(strNone));
    htmlTagEnd(FPSTR(tagTD));
    htmlTagStart(FPSTR(tagTD), FPSTR(attrAlignCenter));
    if (relays[id].relayLevel)
      _client->print(F("HIGH"));
    else
      _client->print(F("LOW"));
    htmlTagEnd(FPSTR(tagTD));
    htmlTagStart(FPSTR(tagTD), FPSTR(attrAlignCenter));
    if (relays[id].relayOnBoot)
      _client->print(F("ON"));
    else
      _client->print(F("OFF"));
    htmlTagEnd(FPSTR(tagTD));
    htmlTagStart(FPSTR(tagTD), FPSTR(attrAlignRight));
    _client->print(relays[id].relayAutoRelease);
    htmlTagEnd(FPSTR(tagTD));
    htmlTagEnd(FPSTR(tagTD));
    htmlTagStart(FPSTR(tagTD));
    if (*relays[id].relayName)
      _client->print(relays[id].relayName);
    htmlTagEnd(FPSTR(tagTD));
    htmlTagEnd(FPSTR(tagTR), true);
  }
  htmlTagEnd(FPSTR(tagTable), true);
  _client->print(F("<sup>*</sup> time in seconds to auto off relay (0 to disable this feature)\n"));
  htmlTagSimple(FPSTR(tagP), true);
  _client->print(F("<i>Don't forget to save changes and restart!</i>\n"));
  htmlTagSimple(FPSTR(tagP), true);
  {
    char str[80];

    sprintf(str, sizeof(str), F("onclick=\"location.href='%S?%S=1'\""), FPSTR(pathStore), FPSTR(paramReboot));
    htmlTagInput(FPSTR(inputTypeButton), FPSTR(NULL), F("Save"), str, true);
  }
  btnBack();
  _client->print(F("\n\
<div id=\"form\" class=\"modal\">\n\
<div class=\"modal-content\">\n\
<span class=\"close\" onclick=\"closeForm()\">&times;</span>\n"));
  printf(*_client, F("<form name=\"form\" method=\"GET\" action=\"%S\" onsubmit=\"closeForm()\">\n"), FPSTR(pathSet));
  htmlTagInput(FPSTR(inputTypeHidden), FPSTR(paramId), F("-1"), true);
  htmlTagInput(FPSTR(inputTypeHidden), FPSTR(paramFrom), _uri, true);
  htmlTag(FPSTR(tagLabel), F("GPIO:"));
  htmlTagBR(true);
  listGPIO(FPSTR(paramRelayGPIO), true);
  htmlTagBR(true);
  htmlTag(FPSTR(tagLabel), F("Level:"));
  htmlTagBR(true);
  htmlTagInput(FPSTR(inputTypeRadio), FPSTR(paramRelayLevel), F("1"));
  _client->print(F("HIGH\n"));
  htmlTagInput(FPSTR(inputTypeRadio), FPSTR(paramRelayLevel), F("0"));
  _client->print(F("LOW"));
  htmlTagBR(true);
  htmlTag(FPSTR(tagLabel), F("On boot:"));
  htmlTagBR(true);
  htmlTagInput(FPSTR(inputTypeRadio), FPSTR(paramRelayOnBoot), F("1"));
  _client->print(F("ON\n"));
  htmlTagInput(FPSTR(inputTypeRadio), FPSTR(paramRelayOnBoot), F("0"));
  _client->print(F("OFF"));
  htmlTagBR(true);
  htmlTag(FPSTR(tagLabel), F("Auto release:"));
  htmlTagBR(true);
  htmlTagInputText(FPSTR(paramRelayAutoRelease), F("0"), 5, 5, F("onblur=\"checkNumber(this,0,65535)\""));
  htmlTagBR(true);
  htmlTag(FPSTR(tagLabel), F("Relay name:"));
  htmlTagBR(true);
  htmlTagInputText(FPSTR(paramRelayName), FPSTR(strEmpty), sizeof(relays[0].relayName) - 1, sizeof(relays[0].relayName) - 1, true);
  htmlTagSimple(FPSTR(tagP), true);
  htmlTagInput(FPSTR(inputTypeSubmit), FPSTR(NULL), F("Update"), true);
  _client->print(F("</form>\n\
</div>\n\
</div>\n"));
  htmlPageEnd();
  endResponse();
}

void EthRFIDRelayApp::handleControlConfig() {
  static const char codePattern1[] PROGMEM = "form.%S.value=data.%S;\n";
  static const char codePattern2[] PROGMEM = "setRadios(form.%S,data.%S);\n";

#ifdef USEAUTHORIZATION
  if (! authenticate(authUser, authPassword))
    return requestAuthentication();
#endif

  if (! startResponse(200, FPSTR(typeTextHtml)))
    return;

  htmlPageStart(F("Control Setup"));
  htmlPageStyleStart();
  htmlPageStdStyle(false);
  _client->print(F(".modal{\n\
display:none;\n\
position:fixed;\n\
z-index:1;\n\
left:0;\n\
top:0;\n\
width:100%;\n\
height:100%;\n\
overflow:auto;\n\
background-color:rgb(0,0,0);\n\
background-color:rgba(0,0,0,0.4);\n\
}\n\
.modal-content{\n\
background-color:#fefefe;\n\
margin:15% auto;\n\
padding:20px;\n\
border:1px solid #888;\n\
width:240px;\n\
}\n\
.close{\n\
color:#aaa;\n\
float:right;\n\
font-size:28px;\n\
font-weight:bold;\n\
}\n\
.close:hover,\n\
.close:focus{\n\
color:black;\n\
text-decoration:none;\n\
cursor:pointer;\n\
}\n\
.hidden{\n\
display:none;\n\
}\n"));
  htmlPageStyleEnd();
  htmlPageScriptStart();
  htmlPageStdScript(false);
  _client->print(F("function setRadios(radios,val){\n\
for(var i=0;i<radios.length;i++){\n\
if(radios[i].value==val)\n\
radios[i].checked=true;\n\
}\n\
}\n\
function indexOfArray(arr,val){\n\
for(var i=0;i<arr.length;i++){\n\
if(arr[i]==val)\n\
return i;\n\
}\n\
return -1;\n\
}\n\
function setSelect(sel,dis,val){\n\
for(var i=0;i<sel.options.length;i++){\n\
if(sel.options[i].value==val){\n\
sel.options[i].selected=true;\n\
sel.options[i].disabled=false;\n\
}else{\n\
sel.options[i].disabled=(indexOfArray(dis,sel.options[i].value)>=0);\n\
}\n\
}\n\
}\n"));
  printf(*_client, F("function loadData(form){\n\
var request=getXmlHttpRequest();\n\
request.open('GET','%S?%S=1&%S='+form.id.value+'&dummy='+Date.now(),false);\n"), FPSTR(pathGet), FPSTR(paramSrc), FPSTR(paramId));
  _client->print(F("request.send(null);\n\
if(request.status==200){\n\
var data=JSON.parse(request.responseText);\n"));
  printf(*_client, F("setSelect(form.%S,data.%S,data.%S);\n"), FPSTR(paramBtnGPIO), FPSTR(jsonUsedGPIO), FPSTR(paramBtnGPIO));
//  printf(*_client, FPSTR(codePattern1), FPSTR(paramBtnGPIO), FPSTR(paramBtnGPIO));
  printf(*_client, FPSTR(codePattern2), FPSTR(paramBtnLevel), FPSTR(paramBtnLevel));
  printf(*_client, FPSTR(codePattern2), FPSTR(paramBtnTrigger), FPSTR(paramBtnTrigger));
  printf(*_client, FPSTR(codePattern1), FPSTR(paramBtnDebounce), FPSTR(paramBtnDebounce));
  _client->print(F("}\n\
}\n\
function openForm(form,id){\n\
form.id.value=id;\n\
loadData(form);\n\
document.getElementById('form').style.display='block';\n\
}\n\
function closeForm(){\n\
document.getElementById('form').style.display='none';\n\
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
  htmlTagStart(FPSTR(tagTable), F("cols=5"), true);
  htmlTag(FPSTR(tagCaption), F("<h3>Control Setup</h3>"), true);
  htmlTagStart(FPSTR(tagTR));
  htmlTag(FPSTR(tagTH), F("#"));
  htmlTag(FPSTR(tagTH), F("GPIO"));
  htmlTag(FPSTR(tagTH), F("Level"));
  htmlTag(FPSTR(tagTH), F("Switch"));
  htmlTag(FPSTR(tagTH), F("Debounce<sup>*</sup>"));
  htmlTagEnd(FPSTR(tagTR), true);
  for (uint8_t id = 0; id < MAX_RELAYS; ++id) {
    htmlTagStart(FPSTR(tagTR));
    htmlTagStart(FPSTR(tagTD), FPSTR(attrAlignCenter));
    printf(*_client, F("<a href=\"#\" onclick=\"openForm(document.form,%d)\">%d</a>"), id, id + 1);
    htmlTagEnd(FPSTR(tagTD));
    htmlTagStart(FPSTR(tagTD), FPSTR(attrAlignCenter));
    if (relays[id].btnPin != -1)
      _client->print(relays[id].btnPin);
    else
      _client->print(FPSTR(strNone));
    htmlTagEnd(FPSTR(tagTD));
    htmlTagStart(FPSTR(tagTD), FPSTR(attrAlignCenter));
    if (relays[id].btnLevel)
      _client->print(F("HIGH"));
    else
      _client->print(F("LOW"));
    htmlTagEnd(FPSTR(tagTD));
    htmlTagStart(FPSTR(tagTD), FPSTR(attrAlignCenter));
    if (relays[id].btnTrigger)
      _client->write('*');
    htmlTagEnd(FPSTR(tagTD));
    htmlTagStart(FPSTR(tagTD), FPSTR(attrAlignRight));
    _client->print(relays[id].btnDebounceTime);
    htmlTagEnd(FPSTR(tagTD));
    htmlTagEnd(FPSTR(tagTR), true);
  }
  htmlTagEnd(FPSTR(tagTable), true);
  _client->print(F("<sup>*</sup> time in milliseconds to debounce switch (0 to disable this feature)\n"));
  htmlTagSimple(FPSTR(tagP), true);
  _client->print(F("<i>Don't forget to save changes and restart!</i>\n"));
  htmlTagSimple(FPSTR(tagP), true);
  {
    char str[80];

    sprintf(str, sizeof(str), F("onclick=\"location.href='%S?%S=1'\""), FPSTR(pathStore), FPSTR(paramReboot));
    htmlTagInput(FPSTR(inputTypeButton), FPSTR(NULL), F("Save"), str, true);
  }
  btnBack();
  _client->print(F("\n\
<div id=\"form\" class=\"modal\">\n\
<div class=\"modal-content\">\n\
<span class=\"close\" onclick=\"closeForm()\">&times;</span>\n"));
  printf(*_client, F("<form name=\"form\" method=\"GET\" action=\"%S\" onsubmit=\"closeForm()\">\n"), FPSTR(pathSet));
  htmlTagInput(FPSTR(inputTypeHidden), FPSTR(paramId), F("-1"), true);
  htmlTagInput(FPSTR(inputTypeHidden), FPSTR(paramFrom), _uri, true);
  htmlTag(FPSTR(tagLabel), F("GPIO:"));
  htmlTagBR(true);
  listGPIO(FPSTR(paramBtnGPIO), true);
  htmlTagBR(true);
  htmlTag(FPSTR(tagLabel), F("Level:"));
  htmlTagBR(true);
  htmlTagInput(FPSTR(inputTypeRadio), FPSTR(paramBtnLevel), F("1"));
  _client->print(F("HIGH\n"));
  htmlTagInput(FPSTR(inputTypeRadio), FPSTR(paramBtnLevel), F("0"));
  _client->print(F("LOW"));
  htmlTagBR(true);
  htmlTag(FPSTR(tagLabel), F("Switch:"));
  htmlTagBR(true);
  htmlTagInput(FPSTR(inputTypeRadio), FPSTR(paramBtnTrigger), F("1"));
  _client->print(F("TRIGGER\n"));
  htmlTagInput(FPSTR(inputTypeRadio), FPSTR(paramBtnTrigger), F("0"));
  _client->print(F("BUTTON"));
  htmlTagBR(true);
  htmlTag(FPSTR(tagLabel), F("Debounce time:"));
  htmlTagBR(true);
  htmlTagInputText(FPSTR(paramBtnDebounce), F("0"), 5, 5, F("onblur=\"checkNumber(this,0,65535)\""), true);
  htmlTagSimple(FPSTR(tagP), true);
  htmlTagInput(FPSTR(inputTypeSubmit), FPSTR(NULL), F("Update"), true);
  _client->print(F("</form>\n\
</div>\n\
</div>\n"));
  htmlPageEnd();
  endResponse();
}

void EthRFIDRelayApp::handleRelaySwitch() {
  uint8_t id = atoi(arg(FPSTR(paramId)));
  bool on = equals_P(arg(FPSTR(paramOn)), strTrue);

  switchRelay(id, on);

  send(200, FPSTR(typeTextPlain), FPSTR(strEmpty));
}

#ifdef MAX_SCHEDULES
void EthRFIDRelayApp::handleScheduleConfig() {
  static const char codePattern[] PROGMEM = "form.%S.value=data.%S;\n";

#ifdef USEAUTHORIZATION
  if (! authenticate(authUser, authPassword))
    return requestAuthentication();
#endif

  if (! startResponse(200, FPSTR(typeTextHtml)))
    return;

  htmlPageStart(F("Schedule Setup"));
  htmlPageStyleStart();
  htmlPageStdStyle(false);
  _client->print(F(".modal{\n\
display:none;\n\
position:fixed;\n\
z-index:1;\n\
left:0;\n\
top:0;\n\
width:100%;\n\
height:100%;\n\
overflow:auto;\n\
background-color:rgb(0,0,0);\n\
background-color:rgba(0,0,0,0.4);\n\
}\n\
.modal-content{\n\
background-color:#fefefe;\n\
margin:15% auto;\n\
padding:20px;\n\
border:1px solid #888;\n\
width:400px;\n\
}\n\
.close{\n\
color:#aaa;\n\
float:right;\n\
font-size:28px;\n\
font-weight:bold;\n\
}\n\
.close:hover,\n\
.close:focus{\n\
color:black;\n\
text-decoration:none;\n\
cursor:pointer;\n\
}\n\
.hidden{\n\
display:none;\n\
}\n"));
  htmlPageStyleEnd();
  htmlPageScriptStart();
  htmlPageStdScript(false);
  printf(*_client, F("function loadData(form){\n\
var request=getXmlHttpRequest();\n\
request.open('GET','%S?%S='+form.id.value+'&dummy='+Date.now(),false);\n"), FPSTR(pathGetSchedule), FPSTR(paramId));
  _client->print(F("request.send(null);\n\
if(request.status==200){\n\
var data=JSON.parse(request.responseText);\n"));
  printf(*_client, FPSTR(codePattern), FPSTR(paramSchedulePeriod), FPSTR(paramSchedulePeriod));
  printf(*_client, FPSTR(codePattern), FPSTR(paramScheduleHour), FPSTR(paramScheduleHour));
  printf(*_client, FPSTR(codePattern), FPSTR(paramScheduleMinute), FPSTR(paramScheduleMinute));
  printf(*_client, FPSTR(codePattern), FPSTR(paramScheduleSecond), FPSTR(paramScheduleSecond));
  printf(*_client, F("if(data.%S==3){\n"), FPSTR(paramSchedulePeriod));
  _client->print(F("var weekdaysdiv=document.getElementById('weekdays');\n\
var elements=weekdaysdiv.getElementsByTagName('input');\n\
for(var i=0;i<elements.length;i++){\n\
if(elements[i].type=='checkbox'){\n"));
  printf(*_client, F("if((data.%S&elements[i].value)!=0)\n"), FPSTR(paramScheduleWeekdays));
  _client->print(F("elements[i].checked=true;\n\
else\n\
elements[i].checked=false;\n\
}\n\
}\n"));
  printf(*_client, FPSTR(codePattern), FPSTR(paramScheduleWeekdays), FPSTR(paramScheduleWeekdays));
  printf(*_client, F("}else{\n\
form.%S.value=0;\n"), FPSTR(paramScheduleWeekdays));
  printf(*_client, FPSTR(codePattern), FPSTR(paramScheduleDay), FPSTR(paramScheduleDay));
  printf(*_client, FPSTR(codePattern), FPSTR(paramScheduleMonth), FPSTR(paramScheduleMonth));
  printf(*_client, FPSTR(codePattern), FPSTR(paramScheduleYear), FPSTR(paramScheduleYear));
  printf(*_client, F("}\n\
form.%S.value=data.%S;\n"), FPSTR(paramScheduleRelay), FPSTR(paramScheduleRelay));
  printf(*_client, F("var radios=document.getElementsByName('%S');\n"), FPSTR(paramScheduleAction));
  printf(*_client, F("for(var i=0;i<radios.length;i++){\n\
if(radios[i].value==data.%S)radios[i].checked=true;\n\
}\n\
}\n\
}\n"), FPSTR(paramScheduleAction));
  printf(*_client, F("function openForm(form,id){\n\
form.id.value=id;\n\
loadData(form);\n\
form.%S.onchange();\n\
document.getElementById('form').style.display='block';\n\
}\n"), FPSTR(paramSchedulePeriod));
  _client->print(F("function closeForm(){\n\
document.getElementById('form').style.display='none';\n\
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
function periodChanged(period){\n\
document.getElementById(\"time\").style.display=(period.value!=0)?\"inline\":\"none\";\n\
document.getElementById(\"hh\").style.display=(period.value>2)?\"inline\":\"none\";\n\
document.getElementById(\"mm\").style.display=(period.value>1)?\"inline\":\"none\";\n\
document.getElementById(\"weekdays\").style.display=(period.value==3)?\"block\":\"none\";\n\
document.getElementById(\"date\").style.display=(period.value>3)?\"block\":\"none\";\n\
document.getElementById(\"month\").style.display=(period.value>4)?\"inline\":\"none\";\n\
document.getElementById(\"year\").style.display=(period.value==6)?\"inline\":\"none\";\n\
document.getElementById(\"relay\").style.display=(period.value!=0)?\"block\":\"none\";\n\
}\n\
function weekChanged(wd){\n"));
  printf(*_client, F("var weekdays=document.form.%S.value;\n\
if(wd.checked==\"\")weekdays&=~wd.value;\n\
else\n\
weekdays|=wd.value;\n\
document.form.%S.value=weekdays;\n\
}\n"), FPSTR(paramScheduleWeekdays), FPSTR(paramScheduleWeekdays));
  printf(*_client, F("function validateForm(form){\n\
if((form.%S.value==3)&&(form.%S.value==0)){\n\
alert(\"None of weekdays selected!\");\n\
return false;\n\
}\n\
return true;\n\
}\n"), FPSTR(paramSchedulePeriod), FPSTR(paramScheduleWeekdays));
  htmlPageScriptEnd();
  htmlPageBody();
  htmlTagStart(FPSTR(tagTable), F("cols=4"), true);
  htmlTag(FPSTR(tagCaption), F("<h3>Schedule Setup</h3>"), true);
  htmlTagStart(FPSTR(tagTR));
  htmlTag(FPSTR(tagTH), F("#"));
  htmlTag(FPSTR(tagTH), F("Event"));
  htmlTag(FPSTR(tagTH), F("Next time"));
  htmlTag(FPSTR(tagTH), F("Relay"));
  htmlTagEnd(FPSTR(tagTR), true);
  for (uint8_t i = 0; i < MAX_SCHEDULES; ++i) {
    htmlTagStart(FPSTR(tagTR));
    htmlTagStart(FPSTR(tagTD), FPSTR(attrAlignCenter));
    printf(*_client, F("<a href=\"#\" onclick=\"openForm(document.form,%d)\">%d</a>"), i, i + 1);
    htmlTagEnd(FPSTR(tagTD));
    htmlTagStart(FPSTR(tagTD));

    char str[80];

    schedules[i].toString(str);
    _client->print(str);
    htmlTagEnd(FPSTR(tagTD));
    htmlTagStart(FPSTR(tagTD));
    schedules[i].nextTimeStr(str);
    _client->print(str);
    htmlTagEnd(FPSTR(tagTD));
    htmlTagStart(FPSTR(tagTD));
    if (schedules[i].period() != Schedule::NONE) {
      if (scheduleRelay[i].relay < MAX_RELAYS) {
        _client->print(scheduleRelay[i].relay + 1);
        if (*relays[scheduleRelay[i].relay].relayName) {
          _client->print(F(" ("));
          _client->print(relays[scheduleRelay[i].relay].relayName);
          _client->write(')');
        }
        if (scheduleRelay[i].action == ACTION_TOGGLE)
          _client->print(F(" toggle"));
        else
          _client->print(scheduleRelay[i].action == ACTION_ON ? F(" on") : F(" off"));
      }
    }
    htmlTagEnd(FPSTR(tagTD));
    htmlTagEnd(FPSTR(tagTR), true);
  }
  htmlTagEnd(FPSTR(tagTable), true);
  htmlTagSimple(FPSTR(tagP), true);
  _client->print(F("<i>Don't forget to save changes!</i>\n"));
  htmlTagSimple(FPSTR(tagP), true);
  {
    char str[80];

    sprintf(str, sizeof(str), F("onclick=\"location.href='%S?reboot=0'\""), FPSTR(pathStore));
    htmlTagInput(FPSTR(inputTypeButton), FPSTR(NULL), F("Save"), str, true);
  }
  btnBack();
  _client->print(F("\n\
<div id=\"form\" class=\"modal\">\n\
<div class=\"modal-content\">\n\
<span class=\"close\" onclick=\"closeForm()\">&times;</span>\n"));
  printf(*_client, F("<form name=\"form\" method=\"GET\" action=\"%S\" onsubmit=\"if(validateForm(this))closeForm(); else return false;\">\n"), FPSTR(pathSetSchedule));
  htmlTagInput(FPSTR(inputTypeHidden), FPSTR(paramId), F("-1"), true);
  printf(*_client, F("<select name=\"%S\" size=1 onchange=\"periodChanged(this)\">\n"), FPSTR(paramSchedulePeriod));
  _client->print(F("<option value=\"0\">Never!</option>\n\
<option value=\"1\">Every minute</option>\n\
<option value=\"2\">Every hour</option>\n\
<option value=\"3\">Every week</option>\n\
<option value=\"4\">Every month</option>\n\
<option value=\"5\">Every year</option>\n\
<option value=\"6\">Once</option>\n\
</select>\n\
<span id=\"time\" class=\"hidden\">at\n\
<span id=\"hh\" class=\"hidden\">"));
  htmlTagInputText(FPSTR(paramScheduleHour), F("0"), 2, 2, F("onblur=\"checkNumber(this,0,23)\""), true);
  _client->print(F(":</span>\n\
<span id=\"mm\" class=\"hidden\">"));
  htmlTagInputText(FPSTR(paramScheduleMinute), F("0"), 2, 2, F("onblur=\"checkNumber(this,0,59)\""), true);
  _client->print(F(":</span>\n"));
  htmlTagInputText(FPSTR(paramScheduleSecond), F("0"), 2, 2, F("onblur=\"checkNumber(this,0,59)\""));
  _client->print(F("</span><br/>\n\
<div id=\"weekdays\" class=\"hidden\">\n"));
  htmlTagInput(FPSTR(inputTypeHidden), FPSTR(paramScheduleWeekdays), F("0"), true);
  for (uint8_t i = 0; i < 7; ++i) {
    char str[4];

    htmlTagInput(FPSTR(inputTypeCheckbox), FPSTR(NULL), itoa(1 << i, str, 10), F("onchange=\"weekChanged(this)\""));
    _client->print(weekdayName(str, i));
    _client->write(charLF);
  }
  _client->print(F("</div>\n\
<div id=\"date\" class=\"hidden\">\n"));
  printf(*_client, F("<select name=\"%S\" size=1>\n"), FPSTR(paramScheduleDay));
  for (uint8_t i = 1; i <= 31; ++i) {
    printf(*_client, F("<option value=\"%d\">%d</option>\n"), i, i);
  }
  printf(*_client, F("<option value=\"%d\">Last</option>\n"), Schedule::LASTDAYOFMONTH);
  _client->print(F("</select>\n\
day\n\
<span id=\"month\" class=\"hidden\">of\n"));
  printf(*_client, F("<select name=\"%S\" size=1>\n"), FPSTR(paramScheduleMonth));
  for (uint8_t i = 1; i <= 12; ++i) {
    char str[4];

    printf(*_client, F("<option value=\"%d\">%s</option>\n"), i, monthName(str, i));
  }
  _client->print(F("</select>\n\
</span>\n\
<span id=\"year\" class=\"hidden\">"));
  htmlTagInputText(FPSTR(paramScheduleYear), F("2017"), 4, 4, F("onblur=\"checkNumber(this,2017,2099)\""));
  _client->print(F("</span>\n\
</div>\n\
<div id=\"relay\" class=\"hidden\">\n\
<label>Relay #</label>\n"));
  listRelays(FPSTR(paramScheduleRelay));
  _client->print(F("turn\n"));
  listActions(FPSTR(paramScheduleAction));
  _client->print(F("</div>\n"));
  htmlTagSimple(FPSTR(tagP), true);
  htmlTagInput(FPSTR(inputTypeSubmit), FPSTR(NULL), F("Update"), true);
  _client->print(F("</form>\n\
</div>\n\
</div>\n"));
  htmlPageEnd();
  endResponse();
}

void EthRFIDRelayApp::handleGetSchedule() {
  int8_t id = -1;

  if (hasArg(FPSTR(paramId)))
    id = atoi(arg(FPSTR(paramId)));

  if ((id >= 0) && (id < MAX_SCHEDULES)) {
    if (! startResponse(200, FPSTR(typeTextJson)))
      return;

    printf(*_client, F("{\"%S\":%d,\"%S\":%d,\"%S\":%d,\"%S\":%d,\"%S\":%d"), FPSTR(paramSchedulePeriod), schedules[id].period(), FPSTR(paramScheduleHour), schedules[id].hour(),
      FPSTR(paramScheduleMinute), schedules[id].minute(), FPSTR(paramScheduleSecond), schedules[id].second(), FPSTR(paramScheduleWeekdays), schedules[id].weekdays());
    printf(*_client, F(",\"%S\":%d,\"%S\":%d,\"%S\":%d,\"%S\":%d,\"%S\":%d}"), FPSTR(paramScheduleDay), schedules[id].day(), FPSTR(paramScheduleMonth), schedules[id].month(),
      FPSTR(paramScheduleYear), schedules[id].year(), FPSTR(paramScheduleRelay), scheduleRelay[id].relay, FPSTR(paramScheduleAction), scheduleRelay[id].action);
    endResponse();
  } else
    send(204, FPSTR(typeTextJson), FPSTR(strEmpty)); // No content
}

void EthRFIDRelayApp::handleSetSchedule() {
  char* _argName;
  char* _argValue;
  int8_t id = -1;
  Schedule::period_t period = Schedule::NONE;
  int8_t hour = -1;
  int8_t minute = -1;
  int8_t second = -1;
  uint8_t weekdays = 0;
  int8_t day = 0;
  int8_t month = 0;
  int16_t year = 0;
  int8_t relay = 0;
  action_t action = ACTION_OFF;

  for (uint8_t i = 0; i < args(); ++i) {
    _argName = (char*)argName(i);
    _argValue = (char*)arg(i);

    if (equals_P(_argName, paramId))
      id = atoi(_argValue);
    else if (equals_P(_argName, paramSchedulePeriod))
      period = (Schedule::period_t)atoi(_argValue);
    else if (equals_P(_argName, paramScheduleHour))
      hour = constrain(atoi(_argValue), 0, 23);
    else if (equals_P(_argName, paramScheduleMinute))
      minute = constrain(atoi(_argValue), 0, 59);
    else if (equals_P(_argName, paramScheduleSecond))
      second = constrain(atoi(_argValue), 0, 59);
    else if (equals_P(_argName, paramScheduleWeekdays))
      weekdays = atoi(_argValue);
    else if (equals_P(_argName, paramScheduleDay))
      day = constrain(atoi(_argValue), 1, Schedule::LASTDAYOFMONTH);
    else if (equals_P(_argName, paramScheduleMonth))
      month = constrain(atoi(_argValue), 1, 12);
    else if (equals_P(_argName, paramScheduleYear))
      year = constrain(atoi(_argValue), 2017, 2099);
    else if (equals_P(_argName, paramScheduleRelay))
      relay = constrain(atoi(_argValue), 0, MAX_RELAYS - 1);
    else if (equals_P(_argName, paramScheduleAction))
      action = (action_t)atoi(_argValue);
    else
      printf(_log, F("Unknown schedule parameter \"%s\"!\n"), _argName);
  }

  if ((id >= 0) && (id < MAX_SCHEDULES)) {
    if (period == Schedule::NONE)
      schedules[id].clear();
    else
      schedules[id].set(period, hour, minute, second, weekdays, day, month, year);
    scheduleRelay[id].relay = relay;
    scheduleRelay[id].action = action;

    if (! startResponse(200, FPSTR(typeTextHtml)))
      return;

    htmlPageStart(F("Store Schedule"));
    printf(*_client, F("<meta http-equiv=\"refresh\" content=\"1;URL=%S\">\n"), FPSTR(pathScheduleConfig));
    htmlPageStdStyle();
    htmlPageBody();
    _client->print(F("Configuration updated successfully.\n"));
    htmlPageEnd();
    endResponse();
  } else
    send(204, FPSTR(typeTextHtml), FPSTR(strEmpty)); // No content
}
#endif

void EthRFIDRelayApp::handleLastKey() {
  const int32_t keyTimeout = 1000;
  static uint32_t lastTime;

  if (((int32_t)millis() - (int32_t)lastTime < keyTimeout) && (! isKeyEmpty(lastKey))) {
    if (! startResponse(200, FPSTR(typeTextJson)))
      return;

    _client->print(F("{\""));
    _client->print(FPSTR(jsonKey));
    _client->print(F("\":\""));
    {
      char str[UID_LENGTH * 2 + 1];

      _client->print(keyToStr(str, lastKey));
    }
    _client->print(F("\"}"));
    endResponse();
  } else {
    send(204, FPSTR(typeTextJson), FPSTR(strEmpty)); // No content
  }

  clearKey(lastKey);
  lastTime = millis();
}

void EthRFIDRelayApp::handleKeysConfig() {
  static const char codePattern[] PROGMEM = "form.%S.value=data.%S;\n";

#ifdef USEAUTHORIZATION
  if (! authenticate(authUser, authPassword))
    return requestAuthentication();
#endif

  if (! startResponse(200, FPSTR(typeTextHtml)))
    return;

  htmlPageStart(F("Keys Setup"));
  htmlPageStyleStart();
  htmlPageStdStyle(false);
  _client->print(F(".modal{\n\
display:none;\n\
position:fixed;\n\
z-index:1;\n\
left:0;\n\
top:0;\n\
width:100%;\n\
height:100%;\n\
overflow:auto;\n\
background-color:rgb(0,0,0);\n\
background-color:rgba(0,0,0,0.4);\n\
}\n\
.modal-content{\n\
background-color:#fefefe;\n\
margin:15% auto;\n\
padding:20px;\n\
border:1px solid #888;\n\
width:320px;\n\
}\n\
.close{\n\
color:#aaa;\n\
float:right;\n\
font-size:28px;\n\
font-weight:bold;\n\
}\n\
.close:hover,\n\
.close:focus{\n\
color:black;\n\
text-decoration:none;\n\
cursor:pointer;\n\
}\n\
.hidden{\n\
display:none;\n\
}\n"));
  htmlPageStyleEnd();
  htmlPageScriptStart();
  htmlPageStdScript(false);
  _client->print(F("var currentInput=null;\n\
var timeoutId;\n\
function refreshData(){\n\
if(currentInput){\n\
var request=getXmlHttpRequest();\n"));
  printf(*_client, F("request.open('GET','%S?dummy='+Date.now(),true);\n"), FPSTR(pathLastKey));
  _client->print(F("request.onreadystatechange=function(){\n\
if(request.readyState==4){\n\
if(request.status==200){\n\
var data=JSON.parse(request.responseText);\n"));
  printf(*_client, F("currentInput.value=data.%S;\n"), FPSTR(jsonKey));
  _client->print(F("}\n\
timeoutId=setTimeout(refreshData,500);\n\
}\n\
}\n\
request.send(null);\n\
}\n\
}\n"));
  printf(*_client, F("function loadData(form){\n\
var request=getXmlHttpRequest();\n\
request.open('GET','%S?%S='+form.id.value+'&dummy='+Date.now(),false);\n"), FPSTR(pathGetKey), FPSTR(paramId));
  _client->print(F("request.send(null);\n\
if(request.status==200){\n\
var data=JSON.parse(request.responseText);\n"));
  printf(*_client, FPSTR(codePattern), FPSTR(paramKey), FPSTR(paramKey));
  printf(*_client, FPSTR(codePattern), FPSTR(paramKeyRelay), FPSTR(paramKeyRelay));
  printf(*_client, F("var radios=document.getElementsByName('%S');\n"), FPSTR(paramKeyAction));
  printf(*_client, F("for(var i=0;i<radios.length;i++){\n\
if(radios[i].value==data.%S)radios[i].checked=true;\n\
}\n\
}\n\
}\n"), FPSTR(paramKeyAction));
  _client->print(F("function openForm(form,id){\n\
form.id.value=id;\n\
loadData(form);\n\
document.getElementById('form').style.display='block';\n\
}\n\
function closeForm(){\n\
document.getElementById('form').style.display='none';\n\
}\n\
function checkUID(src){\n\
var value=src.value;\n\
value=value.toUpperCase();\n"));
  printf(*_client, F("for(var i=0;i<%d;i++){\n"), UID_LENGTH * 2);
  _client->print(F("if(i>=value.length)\n\
value+='0';\n\
else{\n\
if((value[i]<'0')||((value[i]>'9')&&((value[i]<'A')||(value[i]>'F'))))\n\
value=value.substring(0,i)+'0'+value.substring(i+1);\n\
}\n\
}\n\
src.value=value;\n\
}\n\
function gotFocus(control){\n\
currentInput=control;\n\
refreshData();\n\
}\n\
function lostFocus(control){\n\
currentInput=null;\n\
clearTimeout(timeoutId);\n\
checkUID(control);\n\
}\n"));
  htmlPageScriptEnd();
  htmlPageBody();
  htmlTagStart(FPSTR(tagTable), F("cols=3"), true);
  htmlTag(FPSTR(tagCaption), F("<h3>Keys Setup</h3>"), true);
  htmlTagStart(FPSTR(tagTR));
  htmlTag(FPSTR(tagTH), F("#"));
  htmlTag(FPSTR(tagTH), F("Key UID"));
  htmlTag(FPSTR(tagTH), F("Relay"));
  htmlTagEnd(FPSTR(tagTR), true);
  for (uint16_t i = 0; i < MAX_KEYS; ++i) {
    htmlTagStart(FPSTR(tagTR));
    htmlTagStart(FPSTR(tagTD), FPSTR(attrAlignCenter));
    printf(*_client, F("<a href=\"#\" onclick=\"openForm(document.form,%d)\">%d</a>"), i, i + 1);
    htmlTagEnd(FPSTR(tagTD));
    htmlTagStart(FPSTR(tagTD));
    {
      char keystr[UID_LENGTH * 2 + 1];

      _client->print(keyToStr(keystr, keys[i].uid));
    }
    htmlTagEnd(FPSTR(tagTD));
    htmlTagStart(FPSTR(tagTD));
    if (! isKeyEmpty(keys[i].uid)) {
      if (keys[i].relay < MAX_RELAYS) {
        _client->print(keys[i].relay + 1);
        if (*relays[keys[i].relay].relayName) {
          _client->print(F(" ("));
          _client->print(relays[keys[i].relay].relayName);
          _client->write(')');
        }
        if (keys[i].action == ACTION_TOGGLE)
          _client->print(F(" toggle"));
        else
          _client->print(keys[i].action == ACTION_ON ? F(" on") : F(" off"));
      }
    }
    htmlTagEnd(FPSTR(tagTD));
    htmlTagEnd(FPSTR(tagTR), true);
  }
  htmlTagEnd(FPSTR(tagTable), true);
  htmlTagSimple(FPSTR(tagP), true);
  _client->print(F("<i>Don't forget to save changes!</i>\n"));
  htmlTagSimple(FPSTR(tagP), true);
  {
    char str[80];

    sprintf(str, sizeof(str), F("onclick=\"location.href='%S?reboot=0'\""), FPSTR(pathStore));
    htmlTagInput(FPSTR(inputTypeButton), FPSTR(NULL), F("Save"), str, true);
  }
  btnBack();
  _client->print(F("\n\
<div id=\"form\" class=\"modal\">\n\
<div class=\"modal-content\">\n\
<span class=\"close\" onclick=\"closeForm()\">&times;</span>\n"));
  printf(*_client, F("<form name=\"form\" method=\"GET\" action=\"%S\" onsubmit=\"closeForm()\">\n"), FPSTR(pathSetKey));
  htmlTagInput(FPSTR(inputTypeHidden), FPSTR(paramId), F("-1"), true);
  htmlTagInput(FPSTR(inputTypeHidden), FPSTR(paramKeyMaster), F("0"), true);
  htmlTag(FPSTR(tagLabel), F("Key UID:"));
  htmlTagBR(true);
  htmlTagInputText(FPSTR(paramKey), FPSTR(strEmpty), UID_LENGTH * 2, UID_LENGTH * 2, F("onfocus=\"gotFocus(this)\" onblur=\"lostFocus(this)\""), true);
  htmlTagBR(true);
  htmlTag(FPSTR(tagLabel), F("Relay #"));
  htmlTagBR(true);
  listRelays(FPSTR(paramKeyRelay));
  _client->print(F("turn\n"));
  listActions(FPSTR(paramKeyAction));
  htmlTagSimple(FPSTR(tagP), true);
  htmlTagInput(FPSTR(inputTypeSubmit), FPSTR(NULL), F("Update"), true);
  htmlTagInput(FPSTR(inputTypeButton), FPSTR(NULL), F("Set as master"), F("onclick=\"form.master.value=1;form.submit()\""), true);
  _client->print(F("</form>\n\
</div>\n\
</div>\n"));
  htmlPageEnd();
  endResponse();
}

void EthRFIDRelayApp::handleGetKey() {
  int16_t id = -1;

  if (hasArg(FPSTR(paramId)))
    id = atoi(arg(FPSTR(paramId)));

  if ((id >= 0) && (id < MAX_KEYS)) {
    if (! startResponse(200, FPSTR(typeTextJson)))
      return;

    {
      char keystr[UID_LENGTH * 2 + 1];

      printf(*_client, F("{\"%S\":\"%s\",\"%S\":%d,\"%S\":%d}"), FPSTR(paramKey), keyToStr(keystr, keys[id].uid),
        FPSTR(paramKeyRelay), keys[id].relay, FPSTR(paramKeyAction), keys[id].action);
    }
    endResponse();
  } else
    send(204, FPSTR(typeTextJson), FPSTR(strEmpty)); // No content
}

void EthRFIDRelayApp::handleSetKey() {
  char* _argName;
  char* _argValue;
  int16_t id = -1;
  key_t key;
  boolean isMaster = false;

  for (uint8_t i = 0; i < args(); ++i) {
    _argName = (char*)argName(i);
    _argValue = (char*)arg(i);

    if (equals_P(_argName, paramId))
      id = atoi(_argValue);
    else if (equals_P(_argName, paramKey))
      strToKey(_argValue, key.uid);
    else if (equals_P(_argName, paramKeyRelay))
      key.relay = constrain(atoi(_argValue), 0, MAX_RELAYS - 1);
    else if (equals_P(_argName, paramKeyAction))
      key.action = (action_t)atoi(_argValue);
    else if (equals_P(_argName, paramKeyMaster))
      isMaster = constrain(atoi(_argValue), 0, 1);
    else
      printf(_log, F("Unknown key parameter \"%s\"!\n"), _argName);
  }

  if ((id >= 0) && (id < MAX_KEYS)) {
    if (isMaster) {
      copyKey(masterKey, key.uid, UID_LENGTH);
      clearKey(keys[id].uid);
      keys[id].relay = 0;
      keys[id].action = ACTION_OFF;
    } else {
      copyKey(keys[id].uid, key.uid, UID_LENGTH);
      keys[id].relay = key.relay;
      keys[id].action = key.action;
    }

    if (! startResponse(200, FPSTR(typeTextHtml)))
      return;

    htmlPageStart(F("Store Keys"));
    printf(*_client, F("<meta http-equiv=\"refresh\" content=\"1;URL=%S\">\n"), FPSTR(pathKeysConfig));
    htmlPageStdStyle();
    htmlPageBody();
    _client->print(F("Configuration updated successfully.\n"));
    htmlPageEnd();
    endResponse();
  } else
    send(204, FPSTR(typeTextHtml), FPSTR(strEmpty)); // No content
}

#ifdef USEDS1820
void EthRFIDRelayApp::handleSensorConfig() {
#ifdef USEAUTHORIZATION
  if (! authenticate(authUser, authPassword))
    return requestAuthentication();
#endif

  if (! startResponse(200, FPSTR(typeTextHtml)))
    return;

  htmlPageStart(F("Sensors Setup"));
  htmlPageStdStyle();
  htmlPageScriptStart();
  _client->print(F("function checkNumber(src,minval,maxval){\n\
var value=parseInt(src.value);\n\
if(isNaN(value))\n\
value=minval;\n\
if(value<minval)\n\
value=minval;\n\
if(value>maxval)\n\
value=maxval;\n\
src.value=value.toString();\n\
}\n\
function emptyNaN(src){\n\
if(isNaN(src.value))\n\
src.value='';\n\
}\n"));
  htmlPageScriptEnd();
  htmlPageBody();
  printf(*_client, F("<form name=\"sensors\" action=\"%S\" method=\"GET\">\n"), FPSTR(pathStore));
  htmlTag(FPSTR(tagH3), F("DS18x20 Setup"), true);
  htmlTag(FPSTR(tagLabel), F("GPIO:"));
  htmlTagBR(true);
  listGPIO(FPSTR(paramDSGPIO), true, dsPin);
  htmlTagBR(true);
  htmlTagStart(FPSTR(tagTable), F("cols=7"), true);
  htmlTagStart(FPSTR(tagTR));
  htmlTag(FPSTR(tagTH), F("#"));
  htmlTag(FPSTR(tagTH), F("Min. temp.<sup>*</sup>"));
  htmlTag(FPSTR(tagTH), F("Relay"));
  htmlTag(FPSTR(tagTH), F("Turn"));
  htmlTag(FPSTR(tagTH), F("Max. temp.<sup>*</sup>"));
  htmlTag(FPSTR(tagTH), F("Relay"));
  htmlTag(FPSTR(tagTH), F("Turn"));
  htmlTagEnd(FPSTR(tagTR), true);
  for (uint8_t i = 0; i < MAX_DS1820; ++i) {
    char name[32];
    char value[16];

    htmlTagStart(FPSTR(tagTR));
    htmlTag(FPSTR(tagTD), FPSTR(attrAlignCenter), itoa(i + 1, value, 10), true);
    htmlTagStart(FPSTR(tagTD));
    strcpy_P(name, paramDSMinTemp);
    strcat(name, itoa(i, value, 10));
    if (isnan(dsMinTemp[i]))
      *value = '\0';
    else
      dtostrf(dsMinTemp[i], 6, 2, value);
    htmlTagInputText(name, value, 12, 12, F("onblur=\"emptyNaN(this)\""), true);
    htmlTagEnd(FPSTR(tagTD), true);
    htmlTagStart(FPSTR(tagTD));
    strcpy_P(name, paramDSMinTempRelay);
    strcat(name, itoa(i, value, 10));
    listRelays(name, dsMinTempTarget[i].relay);
    htmlTagEnd(FPSTR(tagTD), true);
    htmlTagStart(FPSTR(tagTD));
    strcpy_P(name, paramDSMinTempAction);
    strcat(name, itoa(i, value, 10));
    listActions(name, dsMinTempTarget[i].action);
    htmlTagEnd(FPSTR(tagTD), true);
    htmlTagStart(FPSTR(tagTD));
    strcpy_P(name, paramDSMaxTemp);
    strcat(name, itoa(i, value, 10));
    if (isnan(dsMaxTemp[i]))
      *value = '\0';
    else
      dtostrf(dsMaxTemp[i], 6, 2, value);
    htmlTagInputText(name, value, 12, 12, F("onblur=\"emptyNaN(this)\""), true);
    htmlTagEnd(FPSTR(tagTD), true);
    htmlTagStart(FPSTR(tagTD));
    strcpy_P(name, paramDSMaxTempRelay);
    strcat(name, itoa(i, value, 10));
    listRelays(name, dsMaxTempTarget[i].relay);
    htmlTagEnd(FPSTR(tagTD), true);
    htmlTagStart(FPSTR(tagTD));
    strcpy_P(name, paramDSMaxTempAction);
    strcat(name, itoa(i, value, 10));
    listActions(name, dsMaxTempTarget[i].action);
    htmlTagEnd(FPSTR(tagTD));
    htmlTagEnd(FPSTR(tagTR), true);
  }
  htmlTagEnd(FPSTR(tagTable), true);
  _client->print(F("<sup>*</sup> - leave blank if not used\n"));
  htmlTagInput(FPSTR(inputTypeHidden), FPSTR(paramReboot), F("1"), true);
  htmlTagSimple(FPSTR(tagP), true);
  htmlTagInput(FPSTR(inputTypeSubmit), FPSTR(NULL), F("Save"), true);
  btnBack();
  _client->print(F("</form>\n"));
  htmlPageEnd();
  endResponse();
}
#endif

void EthRFIDRelayApp::btnNavigator() {
  htmlTagSimple(FPSTR(tagP), true);
  btnEthernet();
  btnTime();
  btnMQTT();
  btnRelay();
  btnControl();
#ifdef MAX_SCHEDULES
  btnSchedule();
#endif
  btnKeys();
#ifdef USEDS1820
  btnSensor();
#endif
  btnLog();
  btnRestart();
}

void EthRFIDRelayApp::btnRelay() {
  btnToGo(PSTR("Relay"), pathRelayConfig);
}

void EthRFIDRelayApp::btnControl() {
  btnToGo(PSTR("Control"), pathControlConfig);
}

#ifdef MAX_SCHEDULES
void EthRFIDRelayApp::btnSchedule() {
  btnToGo(PSTR("Schedule"), pathScheduleConfig);
}
#endif

void EthRFIDRelayApp::btnKeys() {
  btnToGo(PSTR("Keys"), pathKeysConfig);
}

#ifdef USEDS1820
void EthRFIDRelayApp::btnSensor() {
  btnToGo(PSTR("Sensors"), pathSensorConfig);
}
#endif

void EthRFIDRelayApp::mqttResubscribe() {
  char topic[80];
  char value[2];

  for (uint8_t i = 0; i < MAX_RELAYS; ++i) {
    if (relays[i].relayPin != -1) {
      if (*mqttClient)
        sprintf(topic, sizeof(topic), F("/%s%S/%d"), mqttClient, FPSTR(mqttRelayTopic), i + 1);
      else
        sprintf(topic, sizeof(topic), F("%S/%d"), FPSTR(mqttRelayTopic), i + 1);
      itoa(digitalRead(relays[i].relayPin) == relays[i].relayLevel, value, 10);
      mqttPublish(topic, value);
    }
  }

  if (*mqttClient)
    sprintf(topic, sizeof(topic), F("/%s%S/#"), mqttClient, FPSTR(mqttRelayTopic));
  else
    sprintf(topic, sizeof(topic), F("%S/#"), FPSTR(mqttRelayTopic));
  mqttSubscribe(topic);
}

void EthRFIDRelayApp::mqttCallback(const char* topic, const uint8_t* payload, uint16_t length) {
  static const char strUnexpectedTopic[] PROGMEM = "Unexpected topic!";

  EthWebServerApp::mqttCallback(topic, payload, length);

  String _mqttTopic = FPSTR(mqttRelayTopic);
  char* topicBody = (char*)topic;

  if (*mqttClient)
    topicBody += strlen(mqttClient) + 1; // Skip "/ClientName" from topic
  if (startsWith_P(topicBody, mqttRelayTopic)) {
    topicBody += strlen_P(mqttRelayTopic);
    if (*topicBody++ == charSlash) {
      uint8_t id = 0;

      while ((*topicBody >= '0') && (*topicBody <= '9')) {
        id *= 10;
        id += *topicBody++ - '0';
      }
      if ((id > 0) && (id <= MAX_RELAYS)) {
        --id;
        if (*payload == '0')
          switchRelay(id, false);
        else if (*payload == '1')
          switchRelay(id, true);
        else {
          if (relays[id].relayPin != -1) {
            char value[2];

            itoa(digitalRead(relays[id].relayPin) == relays[id].relayLevel, value, 10);
            mqttPublish(topic, value);
          }
        }
      } else
        _log.println(FPSTR(strWrongRelay));
    } else
      _log.println(FPSTR(strUnexpectedTopic));
  } else
    _log.println(FPSTR(strUnexpectedTopic));
}

void EthRFIDRelayApp::listUsedGPIO() {
  bool start = true;

  for (uint8_t i = 0; i < MAX_RELAYS; ++i) {
    if (relays[i].relayPin != -1) {
      if (start)
        start = false;
      else
        _client->write(',');
      _client->print(relays[i].relayPin);
    }
    if (relays[i].btnPin != -1) {
      if (start)
        start = false;
      else
        _client->write(',');
      _client->print(relays[i].btnPin);
    }
  }
#ifdef USEDS1820
  if (dsPin != -1) {
    if (! start)
      _client->write(',');
    _client->print(dsPin);
  }
#endif
}

void EthRFIDRelayApp::_listGPIO(const char* name, bool withNone, int8_t current, bool progmem) {
  _client->print(F("<select name=\""));
  if (progmem)
    _client->print(FPSTR(name));
  else
    _client->print(name);
  _client->print(F("\" size=1>\n"));
  if (withNone) {
    _client->print(F("<option value=\"-1\""));
    if (current == -1)
      _client->print(F(" selected"));
    _client->write(charGreater);
    _client->print(FPSTR(strNone));
    _client->print(F("</option>\n"));
  }
  for (uint8_t i = MIN_PIN_NO; i <= MAX_PIN_NO; ++i) {
    if ((i != ETH_SS_PIN) && (i != PIN_SPI_MOSI) && (i != PIN_SPI_MISO) && (i != PIN_SPI_SCK) && (i != RFID_RST_PIN) && (i != RFID_SS_PIN)
#ifdef SD_SS_PIN
      && (i != SD_SS_PIN)
#endif
#if defined(USEDS3231) || defined(USEAT24C32)
      && (i != PIN_WIRE_SDA) && (i != PIN_WIRE_SCL)
#endif
#ifdef LED_PIN
      && (i != LED_PIN)
#endif
      ) {
      _client->print(F("<option value=\""));
      _client->print(i);
      _client->write(charQuote);
      if (current == i)
        _client->print(F(" selected"));
      else {
        bool used = false;

        for (uint8_t j = 0; j < MAX_RELAYS; ++j) {
          if ((relays[j].relayPin == i) || (relays[j].btnPin == i)) {
            used = true;
            break;
          }
        }
#ifdef USEDS1820
        if (! used) {
          used = dsPin == i;
        }
#endif
        if (used)
          _client->print(F(" disabled"));
      }
      _client->write(charGreater);
      _client->print(i);
      _client->print(F("</option>\n"));
    }
  }
  _client->print(F("</select>\n"));
}

void EthRFIDRelayApp::_listRelays(const char* name, uint8_t current, bool progmem) {
  _client->print(F("<select name=\""));
  if (progmem)
    _client->print(FPSTR(name));
  else
    _client->print(name);
  _client->print(F("\" size=1>\n"));
  for (uint8_t i = 0; i < MAX_RELAYS; ++i) {
    _client->print(F("<option value=\""));
    _client->print(i);
    _client->write(charQuote);
    if (current == i)
      _client->print(F(" selected"));
    else {
      if (relays[i].relayPin == -1)
        _client->print(F(" disabled"));
    }
    _client->write(charGreater);
    _client->print(i + 1);
    if (*relays[i].relayName) {
      _client->print(F(" ("));
      _client->print(relays[i].relayName);
      _client->write(')');
    }
    _client->print(F("</option>\n"));
  }
  _client->print(F("</select>\n"));
}

void EthRFIDRelayApp::_listActions(const char* name, action_t current, bool progmem) {
  static const char strChecked[] PROGMEM = "checked";

  PGM_P extra;
  char str[2];

  if (current == ACTION_OFF)
    extra = strChecked;
  else
    extra = NULL;
  itoa(ACTION_OFF, str, 10);
  if (progmem)
    htmlTagInput(FPSTR(inputTypeRadio), FPSTR(name), str, FPSTR(extra));
  else
    htmlTagInput(FPSTR(inputTypeRadio), name, str, FPSTR(extra));
  _client->print(F("OFF\n"));
  if (current == ACTION_ON)
    extra = strChecked;
  else
    extra = NULL;
  itoa(ACTION_ON, str, 10);
  if (progmem)
    htmlTagInput(FPSTR(inputTypeRadio), FPSTR(name), str, FPSTR(extra));
  else
    htmlTagInput(FPSTR(inputTypeRadio), name, str, FPSTR(extra));
  _client->print(F("ON\n"));
  if (current == ACTION_TOGGLE)
    extra = strChecked;
  else
    extra = NULL;
  itoa(ACTION_TOGGLE, str, 10);
  if (progmem)
    htmlTagInput(FPSTR(inputTypeRadio), FPSTR(name), str, FPSTR(extra));
  else
    htmlTagInput(FPSTR(inputTypeRadio), name, str, FPSTR(extra));
  _client->print(F("TOGGLE\n"));
}

void EthRFIDRelayApp::switchRelay(uint8_t id, bool on) {
  if ((id >= MAX_RELAYS) || (relays[id].relayPin == -1))
    return;

  if ((digitalRead(relays[id].relayPin) == relays[id].relayLevel) != on) {
    if (relays[id].relayAutoRelease) {
      if (on)
        _autoRelease[id] = millis() + relays[id].relayAutoRelease * 1000;
      else
        _autoRelease[id] = 0;
    }

    digitalWrite(relays[id].relayPin, relays[id].relayLevel == on);

    if (_pubSubClient && _pubSubClient->connected()) {
      char topic[80];
      char value[2];

      if (*mqttClient)
        sprintf(topic, sizeof(topic), F("/%s%S/%d"), mqttClient, FPSTR(mqttRelayTopic), id + 1);
      else
        sprintf(topic, sizeof(topic), F("%S/%d"), FPSTR(mqttRelayTopic), id + 1);
      itoa(on, value, 10);
      mqttPublish(topic, value);
    }
  }
}

void EthRFIDRelayApp::toggleRelay(uint8_t id) {
  switchRelay(id, digitalRead(relays[id].relayPin) != relays[id].relayLevel);
}

bool EthRFIDRelayApp::debounceRead(uint8_t id, uint32_t debounceTime) {
  if ((id >= MAX_RELAYS) || (relays[id].btnPin == -1))
    return false;

  if (! debounceTime)
    return (digitalRead(relays[id].btnPin) == relays[id].btnLevel);

  if (digitalRead(relays[id].btnPin) == relays[id].btnLevel) { // Button pressed
    uint32_t maxTime = millis() + debounceTime;

    while (millis() < maxTime) {
      if (digitalRead(relays[id].btnPin) != relays[id].btnLevel)
        return false;
      delay(1);
    }

    return true;
  }

  return false;
}

#ifdef MAX_SCHEDULES
int16_t EthRFIDRelayApp::readScheduleConfig(uint16_t offset) {
  if (offset) {
    Schedule::period_t period;
    int8_t hour;
    int8_t minute;
    int8_t second;
    union {
      uint8_t weekdays;
      struct {
        int8_t day;
        int8_t month;
        int16_t year;
      };
    } date;

    for (uint8_t i = 0; i < MAX_SCHEDULES; ++i) {
      offset += _readData(offset, &period, sizeof(period));
      offset += _readData(offset, &hour, sizeof(hour));
      offset += _readData(offset, &minute, sizeof(minute));
      offset += _readData(offset, &second, sizeof(second));
      offset += _readData(offset, &date, sizeof(date));
      offset += _readData(offset, &scheduleRelay[i], sizeof(scheduleRelay[i]));

      if ((period == Schedule::NONE) || (scheduleRelay[i].relay >= MAX_RELAYS))
        schedules[i].clear();
      else
        schedules[i].set(period, hour, minute, second, date.weekdays, date.day, date.month, date.year);
    }
  }

  return offset;
}

int16_t EthRFIDRelayApp::writeScheduleConfig(uint16_t offset) {
  if (offset) {
    Schedule::period_t period;
    int8_t hour;
    int8_t minute;
    int8_t second;
    union {
      uint8_t weekdays;
      struct {
        int8_t day;
        int8_t month;
        int16_t year;
      };
    } date;

    for (uint8_t i = 0; i < MAX_SCHEDULES; ++i) {
      period = schedules[i].period();
      hour = schedules[i].hour();
      minute = schedules[i].minute();
      second = schedules[i].second();
      if (period == Schedule::WEEKLY) {
        date.weekdays = schedules[i].weekdays();
      } else {
        date.day = schedules[i].day();
        date.month = schedules[i].month();
        date.year = schedules[i].year();
      }

      offset += _writeData(offset, &period, sizeof(period));
      offset += _writeData(offset, &hour, sizeof(hour));
      offset += _writeData(offset, &minute, sizeof(minute));
      offset += _writeData(offset, &second, sizeof(second));
      offset += _writeData(offset, &date, sizeof(date));
      offset += _writeData(offset, &scheduleRelay[i], sizeof(scheduleRelay[i]));
    }
  }

  return offset;
}
#endif

void EthRFIDRelayApp::publishKey() {
  if (_pubSubClient && _pubSubClient->connected()) {
    char topic[80];
    char value[UID_LENGTH * 2 + 1];

    if (*mqttClient)
      sprintf(topic, sizeof(topic), F("/%s%S"), mqttClient, FPSTR(mqttRFIDTopic));
    else
      strcpy_P(topic, mqttRFIDTopic);
    keyToStr(value, lastKey);
    mqttPublish(topic, value);
  }
}

void EthRFIDRelayApp::clearKey(uint8_t* key) {
  for (uint8_t i = 0; i < UID_LENGTH; ++i)
    key[i] = 0;
}

void EthRFIDRelayApp::copyKey(uint8_t* dest, const uint8_t* src, uint8_t len) {
  for (uint8_t i = 0; i < UID_LENGTH; ++i) {
    if (i < len)
      dest[i] = src[i];
    else
      dest[i] = 0;
  }
}

bool EthRFIDRelayApp::compareKeys(const uint8_t* key1, const uint8_t* key2) {
  for (uint8_t i = 0; i < UID_LENGTH; ++i) {
    if (key1[i] != key2[i])
      return false;
  }

  return true;
}

bool EthRFIDRelayApp::isKeyEmpty(const uint8_t* key) {
  for (uint8_t i = 0; i < UID_LENGTH; ++i) {
    if (key[i])
      return false;
  }

  return true;
}

char* EthRFIDRelayApp::keyToStr(char* str, const uint8_t* key) {
  char* strptr = str;

  for (uint8_t i = 0; i < UID_LENGTH; ++i) {
    uint8_t digit = key[i] >> 4;
    if (digit > 9)
      *strptr++ = 'A' + digit - 10;
    else
      *strptr++ = '0' + digit;
    digit = key[i] & 0x0F;
    if (digit > 9)
      *strptr++ = 'A' + digit - 10;
    else
      *strptr++ = '0' + digit;
  }
  *strptr = '\0';

  return str;
}

static uint8_t hexDigit(char c) {
  if ((c >= '0') && (c <= '9'))
    return (c - '0');
  if ((c >= 'a') && (c <= 'f'))
    return (c - 'a' + 10);
  if ((c >= 'A') && (c <= 'F'))
    return (c - 'A' + 10);
  return 0;
}

void EthRFIDRelayApp::strToKey(const char* str, uint8_t* key) {
  uint8_t len = strlen(str);

  for (uint8_t i = 0; i < UID_LENGTH; ++i) {
    if (i * 2 + 1 < len) {
      key[i] = hexDigit(str[i * 2]) * 16 + hexDigit(str[i * 2 + 1]);
    } else
      key[i] = 0;
  }
}

EthRFIDRelayApp app;

void setup() {
  Serial.begin(115200);

  app.setup();
}

void loop() {
  app.loop();
}
