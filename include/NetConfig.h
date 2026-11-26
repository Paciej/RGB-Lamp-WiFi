#ifndef NET_CONFIG_H
#define NET_CONFIG_H

#include <Arduino.h>

struct NetConfig {
  char ssid[33]     = {0};
  char wifiPass[65] = {0};
  char host[64]     = {0};   // IP lub nazwa brokera
  uint16_t port     = 1883;
  char user[33]     = {0};
  char mqttPass[33] = {0};

  bool valid() const { return ssid[0] && host[0] && port; }
};

#endif