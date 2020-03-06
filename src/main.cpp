#include "include.h"

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;
char mqttRecvBuf[512];

bool otaInProgress = false;

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onWifiConnect(const WiFiEventStationModeConnected& event) {
  Serial.println("Connected to Wi-Fi, configuring IP");
  WiFi.config(WIFI_IP, WIFI_DNS, WIFI_GW, WIFI_NETMASK);
  ArduinoOTA.begin();
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void mqttCmdAbout() {

    char uptimeBuffer[15];
    getUptimeDhms(uptimeBuffer, sizeof(uptimeBuffer));

    String freeSpace;
    prettyBytes(ESP.getFreeSketchSpace(), freeSpace);

    String sketchSize;
    prettyBytes(ESP.getSketchSize(), sketchSize);

    String chipSize;
    prettyBytes(ESP.getFlashChipRealSize(), chipSize);

    String freeHeap;
    prettyBytes(ESP.getFreeHeap(), freeHeap);

    Serial.println(F("Preparing about..."));

    DynamicJsonDocument jsonRoot(512);
    
    jsonRoot[F("version")] = DBELL_VERSION;
    jsonRoot[F("sdkVersion")] = ESP.getSdkVersion();
    jsonRoot[F("coreVersion")] = ESP.getCoreVersion();
    jsonRoot[F("resetReason")] = ESP.getResetReason();
    jsonRoot[F("ssid")] = WiFi.SSID();
    jsonRoot[F("ip")] = WiFi.localIP().toString();
    jsonRoot[F("gateway")] = WiFi.gatewayIP().toString();
    jsonRoot[F("staMac")] = WiFi.macAddress();
    jsonRoot[F("apMac")] = WiFi.softAPmacAddress();
    jsonRoot[F("chipId")] = String(ESP.getChipId(), HEX);
    jsonRoot[F("chipSize")] = chipSize;
    jsonRoot[F("sketchSize")] = sketchSize;
    jsonRoot[F("freeSpace")] = freeSpace;
    jsonRoot[F("freeHeap")] = freeHeap;
    jsonRoot[F("uptime")] = uptimeBuffer;
    jsonRoot[F("defaultGain")] = defaultGain;

    String mqttMsg;
    serializeJsonPretty(jsonRoot, mqttMsg);
    Serial.println(mqttMsg.c_str());

    mqttClient.publish(MQTT_OUT_TOPIC, 0, false, mqttMsg.c_str());
}

void mqttCmdList() {

    DynamicJsonDocument array(512);
 
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
        array.add(dir.fileName());
    }

    String mqttMsg;
    serializeJsonPretty(array,mqttMsg);

    mqttClient.publish(MQTT_OUT_TOPIC, 0, false, mqttMsg.c_str());
}


void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  mqttClient.subscribe(MQTT_IN_TOPIC, 2);
#ifdef MQTT_DEBUG
  mqttClient.publish(MQTT_IN_TOPIC, 0, true, "DBELL Booted - Selftest");
#endif
/*
  Serial.println("Publishing at QoS 0");
  uint16_t packetIdPub1 = mqttClient.publish("test/lol", 1, true, "test 2");
  Serial.print("Publishing at QoS 1, packetId: ");
  Serial.println(packetIdPub1);
  uint16_t packetIdPub2 = mqttClient.publish("test/lol", 2, true, "test 3");
  Serial.print("Publishing at QoS 2, packetId: ");
  Serial.println(packetIdPub2);
*/
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (reason == AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT) {
    Serial.println("Bad server fingerprint.");
  }

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
#ifdef MQTT_DEBUG
  Serial.print("[MQTT] Subscribe acknowledged: ");
  Serial.print("  packetId: ");
  Serial.print(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
#endif
}

void onMqttUnsubscribe(uint16_t packetId) {
#ifdef MQTT_DEBUG
  Serial.print("[MQTT] Unsubscribe acknowledged: ");
  Serial.print("  packetId: ");
  Serial.println(packetId);
#endif
}


void parseMqttMessage(char *payload) {
    StaticJsonDocument<1024> json;
    deserializeJson(json, payload);

    if (payload[0] == '0') {
        return;
    }

    if (json.isNull()) {
        Serial.println("[MQTT] Unable to parse message...");
//        mqttClient.publish(MQTT_OUT_TOPIC, 0, false, PSTR("{event:\"jsonInBuffer.parseObject(payload) failed\"}"));
        return;
    }

    serializeJson(json, Serial);
    Serial.println();

    // Simple commands
    if (json.containsKey("cmd")) {
        if (strcmp("break", json["cmd"]) == 0) {            // Break current action: {cmd:"break"}
            stopPlaying();
            /*
            if (ledActionInProgress) {
                msgPriority = 0;
                ledDefault();
            }
            */
        } else if (strcmp("restart", json["cmd"]) == 0) {   // Restart ESP: {cmd:"restart"}
            ESP.restart();
            delay(500);
        } else if (strcmp("about", json["cmd"]) == 0) {     // About: {cmd:"about"}
            mqttCmdAbout();
        } else if (strcmp("list", json["cmd"]) == 0) {      // List SPIFFS files: {cmd:"list"}
            mqttCmdList();
        }
        return;
    }

/*
    // Set max brightness: {bright:255}
    if (json.containsKey("bright")) {
        uint8_t b = json["bright"].as<uint8_t>();
        if (b >= 0 && b <= 255) {
            FastLED.setBrightness(b);
        }
    }
*/
    // Set default gain: {"gain":1.2}
    if (json.containsKey("gain")) {
        float g = json["gain"].as<float>();
        if (g > 0.01 && g < 3.0) {
            defaultGain = g;
        }
    }

    // Set once gain: {oncegain:1.2}
    if (json.containsKey("oncegain")) {
        float g = json["oncegain"].as<float>();
        if (g > 0.01 && g < 3.0) {
            onceGain = g;
        }
    }

    // Set new MP3 source
    // - from stream: {"mp3":"http://www.universal-soundbank.com/sounds/7340.mp3"}
    // - from SPIFFS: {"mp3":"/mp3/song.mp3"}
    if (json.containsKey("mp3")) {
        strlcpy(audioSource, json["mp3"], sizeof(audioSource));
        newAudioSource = true;
    }
/*
    // Set new MP3 source from TTS proxy {"tts":"May the force be with you"}
    if (json.containsKey("tts")) {
        tts(json["tts"], json.containsKey("voice") ? json["voice"] : String());
    }

    // Set new Rtttl source {"rtttl":"Xfiles:d=4,o=5,b=160:e,b,a,b,d6,2b."}
    if (json.containsKey("rtttl")) {
        strlcpy(audioSource, json["rtttl"], sizeof(audioSource));
        newAudioSource = true;
    }

    // Set new message priority : {"led":"Blink",color:"0xff0000",delay:50,priority:9}
    // LED pattern is considered only if msg priority is >= to previous msg priority
    // This is to avoid masking an important LED alert with a minor one
    if (json.containsKey("priority")) {
        uint8_t thisPriority = json["priority"].as<uint8_t>();
        if (thisPriority < msgPriority) {
            return;
        }
        msgPriority = thisPriority;
    }

    // Set led pattern: {"led":"Blink",color:"0xff0000",delay:50}
    if (json.containsKey("led")) {
        uint32_t d = 100;
        if (json.containsKey("delay")) {
            d = json["delay"].as<uint32_t>();
        }

        int c = 0xFFFFFF;
        if (json.containsKey("color")) {
            c = (int) strtol(json["color"], nullptr, 0);
        }

        if (strcmp("Rainbow", json["led"]) == 0) {
            ledRainbow(d);
        } else if (strcmp("Blink", json["led"]) == 0) {
            ledBlink(d, c);
        } else if (strcmp("Sine", json["led"]) == 0) {
            ledSine(d, c);
        } else if (strcmp("Pulse", json["led"]) == 0) {
            ledPulse(d, c);
        } else if (strcmp("Disco", json["led"]) == 0) {
            ledDisco(d);
        } else if (strcmp("Solid", json["led"]) == 0) {
            ledSolid(c);
        } else if (strcmp("Off", json["led"]) == 0) {
            ledOff();
        } else {
            ledDefault();
        }
    }
*/
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
#ifdef MQTT_DEBUG
  Serial.print("[MQTT] Publish received: ");
  Serial.print("  topic: ");
  Serial.print(topic);
/*
  Serial.print("  qos: ");
  Serial.println(properties.qos);
  Serial.print("  dup: ");
  Serial.println(properties.dup);
*/
  Serial.print("  retain: ");
  Serial.print(properties.retain);
  Serial.print("  len: ");
  Serial.print(len);
  Serial.print("  index: ");
  Serial.print(index);
  Serial.print("  total: ");
  Serial.println(total);
  Serial.print("[MQTT] Message: ");
  if (len > 511) {
    len = 511;
  }
  memset(mqttRecvBuf, 0, len+1);
  memcpy(mqttRecvBuf, payload, len);
  Serial.println(mqttRecvBuf);
#endif
          
parseMqttMessage(mqttRecvBuf);
}

void onMqttPublish(uint16_t packetId) {
#ifdef MQTT_DEBUG
  Serial.print("Publish acknowledged: ");
  Serial.print("  packetId: ");
  Serial.println(packetId);
#endif
}

void onOTAStart() {
  String type;

  if (ArduinoOTA.getCommand() == U_FLASH) {
    type = "sketch";
  } else { // U_FS
    type = "filesystem";
  }

  Serial.println("[OTA] updating " + type);
  otaInProgress = true;
}

void onOTAEnd() {
  Serial.println("\n[OTA] completed");
}

void onOTAProgress(unsigned int progress, unsigned int total) {
  Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
}

void onOTAError(ota_error_t error) {
  Serial.printf("[OTA] Error[%u]: ", error);
  if (error == OTA_AUTH_ERROR) {
    Serial.println("Auth Failed");
  } else if (error == OTA_BEGIN_ERROR) {
    Serial.println("Begin Failed");
  } else if (error == OTA_CONNECT_ERROR) {
    Serial.println("Connect Failed");
  } else if (error == OTA_RECEIVE_ERROR) {
    Serial.println("Receive Failed");
  } else if (error == OTA_END_ERROR) {
    Serial.println("End Failed");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  preallocateBuffer = malloc(preallocateBufferSize);
  preallocateCodec = malloc(preallocateCodecSize);
  if (!preallocateBuffer || !preallocateCodec) {
    Serial.printf_P(PSTR("FATAL ERROR:  Unable to preallocate %d bytes for app\n"), preallocateBufferSize+preallocateCodecSize);
    while (1) delay(1000); // Infinite halt
  }


  wifiConnectHandler = WiFi.onStationModeConnected(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(MQTT_USER, MQTT_PASSWORD);
  ArduinoOTA.setHostname(ESP_NAME);
  ArduinoOTA.onStart(onOTAStart);
  ArduinoOTA.onEnd(onOTAEnd);
  ArduinoOTA.onProgress(onOTAProgress);
  ArduinoOTA.onError(onOTAError);
  connectToWifi();
  pinMode(BELL_PIN,INPUT_PULLUP);
  SPIFFS.begin();
}



bool btnState;
bool oldBtnState;

void loop() {
  ArduinoOTA.handle();
  if (otaInProgress) {
    return;
  }

  btnState = (digitalRead(BELL_PIN) == 1);
  if (btnState != oldBtnState) {
      if (!btnState) {
          Serial.println("[BELL] Button pressed");
          mqttClient.publish(MQTT_BTN_TOPIC, 0, false, "{\"bell\":true}");
      }
      oldBtnState = btnState;
  }

    // HANDLE MP3 
  if (newAudioSource) {
    playAudio();
  } else if (mp3 && mp3->isRunning()) {
    if (!mp3->loop()) {
      stopPlaying();
    }
  }
}



