

#define MQTT_DEBUG    1
#define ESP_NAME      "DBELL"
#define DBELL_VERSION "0.0.1"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>

#define WIFI_SSID       "WIFISSIDHERE"
#define WIFI_PASSWORD   "YourSecretPasswordHere"
#define WIFI_IP         IPAddress (192,168,5,1)
#define WIFI_GW         IPAddress (192,168,5,254)
#define WIFI_DNS        IPAddress (8,8,8,8)
#define WIFI_NETMASK    IPAddress (255,255,255,0)
#define MQTT_HOST       IPAddress (192, 168, 5, 100)
#define MQTT_PORT       1883
#define MQTT_USER       "doorbell" 
#define MQTT_PASSWORD   "CrazyComplexPassword"
#define MQTT_IN_TOPIC   "/home/doorbell/in"
#define MQTT_OUT_TOPIC  "/home/doorbell/out"
#define MQTT_BTN_TOPIC  "/home/doorbell/button"
#define BELL_PIN        5

#include "helpers.h"
#include "audio.h"

