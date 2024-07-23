#include "wled.h"

void kiotWledSetup() {
    loopRegister(kiotWledLoop);
    kiotWledConfigure();
    afterConfigParseRegister(kiotWledConfigure);    
    mqttRegister(kiotWledMQTTCallback);
}


void kiotWledLoop() {
    // if (millis() - last_kiot_ping > 30000) {
    //     last_kiot_ping = millis();
    //     kiotPing();
    // }
}

void kiotWledConfigure() {
    // setConfigKeys(_kiot_config_keys);
    // DEBUG_MSG_P(PSTR("[HTTP_CLIENT] Unknown Error. STATUS CODE : %d \n"), httpCode);

    settingsRegisterCommand(F("PRINTFILES"), [](Embedis* e) {
        #ifdef WLED_DEBUG
        printFileContentFS("/presets.json");
        printFileContentFS("/tmp.json");
        printFileContentFS("/cfg.json");
        printFileContentFS("/wsec.json");
        DEBUG_MSG_P(PSTR("DONE \n"));
        #endif        
    });

    settingsRegisterCommand(F("KIOTMQTTFETCH"), [](Embedis* e) {
        setFetchMqttConf(true);
        SERIAL_SEND_P(PSTR("Done\n"));
    }); 

    settingsRegisterCommand(F("PING"), [](Embedis* e) {
        activeHomePong(true);
        DEBUG_MSG_P(PSTR("Done\n"));
    });     
}


void kiotWledMQTTCallback(unsigned int type, const char *topic, const char *payload, bool retained, bool isEncrypted) {
    // String mqtt_group_color = getSetting("mqttGroupColor");
    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_COLOR_RGB);
        DEBUG_MSG_P(PSTR("MQTT_TOPIC_COLOR_RGB Subscribe Done\n"));
    }

    if (type == MQTT_MESSAGE_EVENT) {
        DEBUG_MSG_P(PSTR("Message event from rgb topc \n"));
        String t = mqttSubtopic((char *)topic);
        if (!t.startsWith(MQTT_TOPIC_COLOR_RGB)) return;
        StaticJsonDocument<256> _doc_;
        DeserializationError error = deserializeJson(_doc_, reinterpret_cast<const char*>(payload));
        JsonObject root = _doc_.as<JsonObject>();
        if (error || root.isNull()) {
            DEBUG_MSG_P(PSTR("[ERROR] parseObject() failed for MQTT OR NULL \n"));
            Serial.println(reinterpret_cast<const char*>(payload));
            return;
        }

        serializeJson(root, Serial); //debug TODO_S1 remove it after testing
        DEBUG_MSG_P(PSTR(" \n"));
        bool verboseResponse = false;

        if (root["v"] && root.size() == 1) {
          //if the received value is just "{"v":true}", send only to this client
          verboseResponse = true;
        } else if (root.containsKey("lv")) {
        //   wsLiveClientId = root["lv"] ? client->id() : 0;
        } else {
          DEBUG_MSG_P(PSTR("Message received successfully and taking action \n"));  
          verboseResponse = deserializeState(root);
        }
        // releaseJSONBufferLock(); // will clean fileDoc
        DEBUG_MSG_P(PSTR("interfaceUpdateCallMode %d\n"), interfaceUpdateCallMode);
        DEBUG_MSG_P(PSTR("verboseResponse %d\n"), verboseResponse);
        activeHomePong(true);
        if (!interfaceUpdateCallMode) { // individual client response only needed if no WS broadcast soon
          if (verboseResponse) {
            // sendDataWs(client);
            // activeHomePong(true);
            // needReportHome(true);
          } else {
            // we have to send something back otherwise WS connection closes
            // client->text(F("{\"success\":true}"));
          }
          // force broadcast in 500ms after updating client
          //lastInterfaceUpdate = millis() - (INTERFACE_UPDATE_COOLDOWN -500); // ESP8266 does not like this
        }        

    }
}

void kiotWledPing(JsonObject &root) {
  JsonObject state = root.createNestedObject("state");
  serializeState(state);
  JsonObject info  = root.createNestedObject("info");
  // enabling this causes a crash due to stack overflow TODO_S1
  // serializeInfo(info);
}
