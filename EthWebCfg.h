#ifndef __ETHWEBCFG_H
#define __ETHWEBCFG_H

#define SD_SS_PIN 4 // Пин, к которому подключен SS для SD-карты (закомментируйте, если отсутствует)
#ifdef SD_SS_PIN
//#define USESDCARD // Закомментируйте, если поддержка SD-карты не нужна
#endif

#define ETH_SS_PIN 10 // Пин SS для Ethernet-шилда

#define USEDS3231 // Закомментируйте, если RTC DS3231 не используется
#define USEAT24C32 // Закомментируйте, если I2C EEPROM AT24C32 не используется

#define USEAUTHORIZATION // Закомментируйте, если не нужна обязательная авторизация
#define USEMQTT // Закомментируйте, если не нужна поддержка MQTT

#endif
