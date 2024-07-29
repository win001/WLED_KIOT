#include "wled.h"

/*
 * MQTT communication protocol for home automation
 */
/* COMMENTED
#ifdef WLED_ENABLE_MQTT
#define MQTT_KEEP_ALIVE_TIME 60    // contact the MQTT broker every 60 seconds

void parseMQTTBriPayload(char* payload)
{
  if      (strstr(payload, "ON") || strstr(payload, "on") || strstr(payload, "true")) {bri = briLast; stateUpdated(CALL_MODE_DIRECT_CHANGE);}
  else if (strstr(payload, "T" ) || strstr(payload, "t" )) {toggleOnOff(); stateUpdated(CALL_MODE_DIRECT_CHANGE);}
  else {
    uint8_t in = strtoul(payload, NULL, 10);
    if (in == 0 && bri > 0) briLast = bri;
    bri = in;
    stateUpdated(CALL_MODE_DIRECT_CHANGE);
  }
}


void onMqttConnect(bool sessionPresent)
{
  //(re)subscribe to required topics
  char subuf[38];

  if (mqttDeviceTopic[0] != 0) {
    strlcpy(subuf, mqttDeviceTopic, 33);
    mqtt.subscribe(subuf, 0);
    strcat_P(subuf, PSTR("/col"));
    mqtt.subscribe(subuf, 0);
    strlcpy(subuf, mqttDeviceTopic, 33);
    strcat_P(subuf, PSTR("/api"));
    mqtt.subscribe(subuf, 0);
  }

  if (mqttGroupTopic[0] != 0) {
    strlcpy(subuf, mqttGroupTopic, 33);
    mqtt.subscribe(subuf, 0);
    strcat_P(subuf, PSTR("/col"));
    mqtt.subscribe(subuf, 0);
    strlcpy(subuf, mqttGroupTopic, 33);
    strcat_P(subuf, PSTR("/api"));
    mqtt.subscribe(subuf, 0);
  }

  usermods.onMqttConnect(sessionPresent);

  doPublishMqtt = true;
  DEBUG_PRINTLN(F("MQTT ready"));
}


void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  static char *payloadStr;

  DEBUG_PRINT(F("MQTT msg: "));
  DEBUG_PRINTLN(topic);

  // paranoia check to avoid npe if no payload
  if (payload==nullptr) {
    DEBUG_PRINTLN(F("no payload -> leave"));
    return;
  }

  if (index == 0) {                       // start (1st partial packet or the only packet)
    if (payloadStr) delete[] payloadStr;  // fail-safe: release buffer
    payloadStr = new char[total+1];       // allocate new buffer
  }
  if (payloadStr == nullptr) return;      // buffer not allocated

  // copy (partial) packet to buffer and 0-terminate it if it is last packet
  char* buff = payloadStr + index;
  memcpy(buff, payload, len);
  if (index + len >= total) { // at end
    payloadStr[total] = '\0'; // terminate c style string
  } else {
    DEBUG_PRINTLN(F("Partial packet received."));
    return; // process next packet
  }
  DEBUG_PRINTLN(payloadStr);

  size_t topicPrefixLen = strlen(mqttDeviceTopic);
  if (strncmp(topic, mqttDeviceTopic, topicPrefixLen) == 0) {
    topic += topicPrefixLen;
  } else {
    topicPrefixLen = strlen(mqttGroupTopic);
    if (strncmp(topic, mqttGroupTopic, topicPrefixLen) == 0) {
      topic += topicPrefixLen;
    } else {
      // Non-Wled Topic used here. Probably a usermod subscribed to this topic.
      usermods.onMqttMessage(topic, payloadStr);
      delete[] payloadStr;
      payloadStr = nullptr;
      return;
    }
  }

  //Prefix is stripped from the topic at this point

  if (strcmp_P(topic, PSTR("/col")) == 0) {
    colorFromDecOrHexString(col, payloadStr);
    colorUpdated(CALL_MODE_DIRECT_CHANGE);
  } else if (strcmp_P(topic, PSTR("/api")) == 0) {
    if (!requestJSONBufferLock(15)) {
      delete[] payloadStr;
      payloadStr = nullptr;
      return;
    }
    if (payloadStr[0] == '{') { //JSON API
      deserializeJson(doc, payloadStr);
      deserializeState(doc.as<JsonObject>());
    } else { //HTTP API
      String apireq = "win"; apireq += '&'; // reduce flash string usage
      apireq += payloadStr;
      handleSet(nullptr, apireq);
    }
    releaseJSONBufferLock();
  } else if (strlen(topic) != 0) {
    // non standard topic, check with usermods
    usermods.onMqttMessage(topic, payloadStr);
  } else {
    // topmost topic (just wled/MAC)
    parseMQTTBriPayload(payloadStr);
  }
  delete[] payloadStr;
  payloadStr = nullptr;
}


void publishMqtt()
{
  doPublishMqtt = false;
  if (!WLED_MQTT_CONNECTED) return;
  DEBUG_PRINTLN(F("Publish MQTT"));

  #ifndef USERMOD_SMARTNEST
  char s[10];
  char subuf[38];

  sprintf_P(s, PSTR("%u"), bri);
  strlcpy(subuf, mqttDeviceTopic, 33);
  strcat_P(subuf, PSTR("/g"));
  mqtt.publish(subuf, 0, retainMqttMsg, s);         // optionally retain message (#2263)

  sprintf_P(s, PSTR("#%06X"), (col[3] << 24) | (col[0] << 16) | (col[1] << 8) | (col[2]));
  strlcpy(subuf, mqttDeviceTopic, 33);
  strcat_P(subuf, PSTR("/c"));
  mqtt.publish(subuf, 0, retainMqttMsg, s);         // optionally retain message (#2263)

  strlcpy(subuf, mqttDeviceTopic, 33);
  strcat_P(subuf, PSTR("/status"));
  mqtt.publish(subuf, 0, true, "online");          // retain message for a LWT

  char apires[1024];                                // allocating 1024 bytes from stack can be risky
  XML_response(nullptr, apires);
  strlcpy(subuf, mqttDeviceTopic, 33);
  strcat_P(subuf, PSTR("/v"));
  mqtt.publish(subuf, 0, retainMqttMsg, apires);   // optionally retain message (#2263)
  #endif
}


//HA autodiscovery was removed in favor of the native integration in HA v0.102.0

bool initMqtt()
{
  if (!mqttEnabled || mqttServer[0] == 0 || !WLED_CONNECTED) return false;

  if (mqtt == nullptr) {
    mqtt = new AsyncMqttClient();
    mqtt.onMessage(onMqttMessage);
    mqtt.onConnect(onMqttConnect);
  }
  if (mqtt.connected()) return true;

  DEBUG_PRINTLN(F("Reconnecting MQTT"));
  IPAddress mqttIP;
  if (mqttIP.fromString(mqttServer)) //see if server is IP or domain
  {
    mqtt.setServer(mqttIP, mqttPort);
  } else {
    mqtt.setServer(mqttServer, mqttPort);
  }
  mqtt.setClientId(mqttClientID);
  if (mqttUser[0] && mqttPass[0]) mqtt.setCredentials(mqttUser, mqttPass);

  #ifndef USERMOD_SMARTNEST
  strlcpy(mqttStatusTopic, mqttDeviceTopic, 33);
  strcat_P(mqttStatusTopic, PSTR("/status"));
  mqtt.setWill(mqttStatusTopic, 0, true, "offline"); // LWT message
  #endif
  mqtt.setKeepAlive(MQTT_KEEP_ALIVE_TIME);
  mqtt.connect();
  return true;
}
#endif
COMMENTED
*/

/*

MQTT MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

// #include <ArduinoJson.h>
// #include <Ticker.h>
// #include <WiFi.h>

// #include <vector>

const char* mqtt_user = 0;
const char* mqtt_pass = 0;

#if MQTT_USE_ASYNC
// #include <AsyncMqttClient.h> // added in wled.h
// AsyncMqttClient mqtt;
#else
#include <PubSubClient.h>
WiFiClient mqttWiFiClient;
PubSubClient mqtt(mqttWiFiClient);
bool _mqtt_connected = false;
#endif

bool _mqtt_enabled = MQTT_ENABLED;
bool _mqtt_use_json = false;

String mqttTopic;
bool _mqttForward;
String _mqttUser;
String _mqttPass;
String _mqttWill;
String _mqttServer;
String _mqttClientId;
String _mqttGetter;
uint16_t _mqtt_port;
unsigned char _mqtt_qos = MQTT_QOS;
bool _mqtt_retain = MQTT_RETAIN;
unsigned long _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MIN;
unsigned long _mqtt_keepalive = MQTT_KEEPALIVE;
String _mqtt_setter;
String _mqtt_getter;
unsigned char _mqtt_connect_try = 0;

// unsigned long mqttConnectedTime = 0;

// std::vector<void (*)(unsigned int, const char *, const char *)> _mqtt_callbacks;
std::vector<mqtt_callback_f> _mqtt_callbacks;
// #if MQTT_SKIP_RETAINED
unsigned long mqttConnectedAt = 0;
int wifiDisconnectedCount = 0;
// #endif

typedef struct {
    char* topic;
    char* message;
} mqtt_message_t;
std::vector<mqtt_message_t> _mqtt_queue;
Ticker mqttFlushTicker;
// unsigned long schedule_heartbeat = 0; TODO_S2 move to global in wled.h

template <typename T> void _mqttApplySetting(T& current, T& updated) {
    if (current != updated) {
        current = std::move(updated);
        // We are handling disconnect separately, as and when we need
        // mqttDisconnect();
    }
}

template <typename T> void _mqttApplySetting(T& current, const T& updated) {
    if (current != updated) {
        current = updated;
        // We are handling disconnect separately, as and when we need
        // mqttDisconnect();
    }
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void _mqtt_config_keys(DynamicJsonDocument& root, char* topic,
                       bool nested_under_topic) {
    strncpy_P(topic, MQTT_TOPIC_CONFIG_MQTT, MAX_CONFIG_TOPIC_SIZE);
    if (nested_under_topic) {
        JsonObject data = root.createNestedObject(topic);
        data["mqttTopic"] = getSetting("mqttTopic", MQTT_TOPIC);
        // data["mqttServer"] = getSetting("mqttServer", MQTT_SERVER);
        // data["mqttPort"] = getSetting("mqttPort", MQTT_PORT);
        // data["mqttUseJson"] = getSetting("mqttUseJson", MQTT_USE_JSON);
        // data["mqttGetter"] = getSetting("mqttGetter", MQTT_USE_GETTER);
        // data["mqttSetter"] = getSetting("mqttSetter", MQTT_USE_SETTER);
        data["mqttUser"] = getSetting("mqttUser", "");

    } else {
        root["mqttTopic"] = getSetting("mqttTopic", MQTT_TOPIC);
        root["mqttServer"] = getSetting("mqttServer", MQTT_SERVER);
        root["mqttPort"] = getSetting("mqttPort", MQTT_PORT);
        // root["mqttUseJson"] = getSetting("mqttUseJson", MQTT_USE_JSON);
        // root["mqttGetter"] = getSetting("mqttGetter", MQTT_USE_GETTER);
        // root["mqttSetter"] = getSetting("mqttSetter", MQTT_USE_SETTER);
        root["mqttUser"] = getSetting("mqttUser", "");
    }

    return;
}

bool mqttConnected() { return mqtt.connected(); }

void mqttDisconnect() {
#if MQTT_USE_ASYNC
    mqtt.disconnect(true);
#endif
}
void mqttConfigure() {
    {
        // Replace identifier
        mqttTopic = getSetting("mqttTopic", MQTT_TOPIC);
        // In our case identifier is the username itself.
        // If mqttUser is not supplied then look for identifier.
        String identifier = getSetting("mqttUser", "");
        if (!identifier.length()) {
            identifier = getSetting("identifier", "");
        }
        // DEBUG_MSG_P(PSTR("MQTT Topic - %s \n"), mqttTopic.c_str());
        mqttTopic.replace("{identifier}", identifier);
        mqttTopic.replace("{hostname}", getSetting("hostname"));
        if (!mqttTopic.endsWith("/")) mqttTopic = mqttTopic + "/";

        // _mqtt_qos = getSetting("mqttQoS", MQTT_QOS).toInt();
        _mqtt_retain = getSetting("mqttRetain", MQTT_RETAIN).toInt() == 1;
        _mqtt_keepalive = getSetting("mqttKeep", MQTT_KEEPALIVE).toInt();
        if (getSetting("mqttClientID").length() == 0)
            delSetting("mqttClientID");

        // Enable
        _mqtt_enabled = getSetting("mqttEnabled", MQTT_ENABLED).toInt() == 1;
        _mqtt_use_json =
            (getSetting("mqttUseJson", MQTT_USE_JSON).toInt() == 1);
        _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MIN;

        String server = getSetting("mqttServer", MQTT_SERVER);
        _mqttApplySetting(_mqttServer, server);
        String mqttGetter = getSetting("mqttGetter", MQTT_USE_GETTER);
        _mqttApplySetting(_mqttGetter, mqttGetter);
        // DEBUG_MSG_P(PSTR("MQTT Topic - %s \n"), mqttTopic.c_str());

        if (!_mqtt_enabled) return;
    }
    // MQTT Options
    {
        String user = getSetting("mqttUser", "");
        String pass = getSetting("mqttPassword", "");
        unsigned char qos = getSetting("mqttQoS", MQTT_QOS).toInt();
        bool retain = getSetting("mqttRetain", MQTT_RETAIN).toInt() == 1;
        unsigned long keepalive = getSetting("mqttKeep", MQTT_KEEPALIVE).toInt();
        String id = String("dev_") + getSetting("identifier");
        uint16_t port = getSetting("mqttPort", MQTT_PORT).toInt();
        String will = mqttTopic + String(MQTT_TOPIC_STATUS);
        _mqttApplySetting(_mqttUser, user);
        _mqttApplySetting(_mqttPass, pass);
        _mqttApplySetting(_mqtt_qos, qos);
        _mqttApplySetting(_mqtt_retain, retain);
        _mqttApplySetting(_mqtt_keepalive, keepalive);
        _mqttApplySetting(_mqttClientId, id);
        _mqttApplySetting(_mqtt_port, port);
        _mqttApplySetting(_mqttWill, will);
    }
}

bool mqttForward() { return _mqttForward; }

void mqttRegister(mqtt_callback_f callback) {
    _mqtt_callbacks.push_back(callback);
}

String mqttSubtopic(char* topic) {
    String response;

    String t = String(topic);
    String mqttSetter = getSetting("mqttSetter", MQTT_USE_SETTER);

    if (t.startsWith(mqttTopic) && t.endsWith(mqttSetter)) {
        response = t.substring(mqttTopic.length(), t.length() - mqttSetter.length());
    } else if (t.startsWith(mqttTopic) && !t.endsWith(mqttSetter)) {
        response = t.substring(mqttTopic.length(), t.length());
    }
    return response;
}
void _mqttFlush() {
    if (_mqtt_queue.size() == 0) return;

    DynamicJsonDocument root(512);
    for (unsigned char i = 0; i < _mqtt_queue.size(); i++) {
        mqtt_message_t element = _mqtt_queue[i];
        root[element.topic] = element.message;
    }

    // if (ntpSynced()) root[MQTT_TOPIC_TIME] = ntpDateTime();
    // root[MQTT_TOPIC_HOSTNAME] = getSetting("hostname", HOSTNAME);
    // root[MQTT_TOPIC_IP] = getIP();

    String output;
    serializeJson(root, output);

    String path = mqttTopic + String(MQTT_TOPIC_BEAT) + _mqttGetter;
    if (mqttSendRaw(path.c_str(), output.c_str(), false, MQTT_RETAIN)) {
        // MQTT First beat has been sent.
        mqtt_firstbeat_reported = true;
        wifi_connected_beat = true;
        reset_reason_reported = true;
    }

    for (unsigned char i = 0; i < _mqtt_queue.size(); i++) {
        mqtt_message_t element = _mqtt_queue[i];
        free(element.topic);
        free(element.message);
    }
    _mqtt_queue.clear();
}

// bool mqttSendRaw(const char * topic, const char * message, bool retain = MQTT_RETAIN) {
//     // Encrypting the message
//     // DEBUG_MSG_P(PSTR("Millis Before Sending Request : "));
//     // DEBUG_MSG_P(PSTR("Millis Before Sending Request : "));
//     // Serial.println(millis());
//     // DEBUG_MSG_P(PSTR("[DEBUG] RelayString : %c "), message);
//     return mqttSendRaw(topic,message,false);
// }
bool mqttSendRaw(const char* topic, const char* message, bool dont_encrypt = false, bool retain = MQTT_RETAIN) {
    if (canSendMqttMessage()) {
        // Serial.println(message);
        DEBUG_MSG_P(PSTR("%s \n"), message);
        if (dont_encrypt) {
#if MQTT_USE_ASYNC
            DEBUG_MSG_P(PSTR("[MQTT] Sending on  %s \n"), topic);
            return (bool)mqtt.publish(topic, _mqtt_qos, retain, message);
#else
            return (bool)mqtt.publish(topic, message, retain);
#endif
        } else {
            // Serial.print(message);
            // Serial.print(",");
            // Serial.print(strlen(message));
            // Serial.print(",");
            // Serial.print(getCipherlength(strlen(message)));
            // Serial.print(",");

            char data_encoded[getBase64Len(getCipherlength(strlen(message)) + 1)];
            memset(data_encoded, '\0', sizeof(data_encoded));
            // DEBUG_MSG_P(PSTR("[DEBUG] Encoded Message Size : %d \n"),sizeof(data_encoded));

            char iv_encoded[30] = "";
            // char device_id[14] = "";
            // char device_id_b64[21] = "";
            // DEBUG_MSG_P(PSTR("Sending Message \n"));
            // Serial.print("Message to Encrypt: ");
            // Serial.println(message);

            // strcpy(device_id, getSetting("identifier").c_str());
            // base64_encode(device_id_b64, device_id, strlen(device_id));

            // Serial.println(sizeof(data_encoded)+sizeof(iv_encoded)+sizeof(device_id_b64)+35);
            encryptMsg(message, data_encoded, iv_encoded);
            // Serial.print("IV b64: ");
            // Serial.println(iv_encoded);
            // Serial.println(strlen(data_encoded));
            // Serial.print("Encoded : ");
            // Serial.println(data_encoded);

            /* Only for debugging PuRpose
            */
            // Serial.println("DECRYPTING BEGINS");
            // char data_decoded[1000] = "";
            // decryptMsg(data_encoded,&data_decoded[0],strlen(data_encoded), iv_encoded); Serial.println(data_decoded);
            // Serial.println("DECRYPTION ENDS");
            /* Debug end */
            // char * buffer;
            // buffer = (char * ) malloc();
            int buffer_size = sizeof(data_encoded) + sizeof(iv_encoded) + 35;
            char buffer[buffer_size];
            memset(buffer, 0, buffer_size);

            snprintf_P(buffer, buffer_size, "{\"meta\":\"%s\",\"payload\":\"%s\"}", iv_encoded, data_encoded);

            DEBUG_MSG_P(PSTR("[MQTT] Sending on  %s \n"), topic);
            // Serial.println(message);
            // Serial.println("\n");
            // Serial.println(buffer);

#if MQTT_USE_ASYNC
            bool packetId = (bool)mqtt.publish(topic, _mqtt_qos, retain, buffer);
            // free(buffer);
            // DEBUG_MSG_P(PSTR("[DEBUG] Message Sent - %d \n"), packetId);
            return packetId;
#else
            bool packetId = (bool)mqtt.publish(topic, buffer, retain);
            // DEBUG_MSG_P(PSTR("[DEBUG] Message Sent - %d \n"), packetId);
            // free(buffer);
            return packetId;
#endif
        }
    } else {
        DEBUG_MSG_P(PSTR("Topic - %s, Message: %s\n Will actually not send "
                         "this message \n"),
                    topic, message);
        return false;
    }
}
// Important -  dont_enctrypt Doesn't work for stacked_message
bool mqttSend(const char* topic, const char* message, bool stack_msg,
              bool dont_enctrypt, bool useGetter,
              bool retain) {
    bool useJson = !stack_msg ? false : _mqtt_use_json;
    if (useJson) {
        mqtt_message_t element;
        element.topic = strdup(topic);
        element.message = strdup(message);
        _mqtt_queue.push_back(element);
        mqttFlushTicker.once_ms(100, _mqttFlush);
        return true;
        // DEBUG_MSG_P(PSTR("[DEBUG] Its mqtt stack message block, It should be
        // coming anywhere now \n"));
    } else {
        String path;
        if (!useGetter) {
            path = mqttTopic + String(topic);
        } else {
            path = mqttTopic + String(topic) + _mqttGetter;
        }

        return mqttSendRaw(path.c_str(), message, dont_enctrypt, retain);
    }
}
bool mqttSend(const char* topic, const char* message, bool stack_msg) {
    return mqttSend(topic, message, stack_msg, false);
}

bool mqttSend(const char* topic, const char* message) {
    return mqttSend(topic, message, false, false);
}

bool mqttSend(const char* topic, unsigned int index, const char* message,
              bool force) {
    char buffer[strlen(topic) + 5];
    sprintf(buffer, "%s/%d", topic, index);
    return mqttSend(buffer, message, force);
}
bool mqttSend(const char* topic, unsigned int index, const char* message) {
    return mqttSend(topic, index, message, false);
}

void mqttSubscribeRaw(const char* topic, int QOS) {
    if (mqtt.connected() && (strlen(topic) > 0)) {
        DEBUG_MSG_P(PSTR("[MQTT] Subscribing to %s with qos : %d \n"), topic,
                    QOS);
        mqtt.subscribe(topic, QOS);
    }
}

void mqttSubscribe(const char* topic) { mqttSubscribe(topic, _mqtt_qos); }
void mqttSubscribe(const char* topic, int QOS) {
    String mqttSetter = getSetting("mqttSetter", MQTT_USE_SETTER);
    String path = mqttTopic + String(topic) + mqttSetter;
    // DEBUG_MSG_P(PSTR("[DEBUG] Subscription Topic %s"), path.c_str());
    mqttSubscribeRaw(path.c_str(), QOS);
}

void mqttUnsubscribeRaw(const char* topic) {
    if (mqtt.connected() && (strlen(topic) > 0)) {
#if MQTT_USE_ASYNC
        unsigned int packetId = mqtt.unsubscribe(topic);
        DEBUG_MSG_P(PSTR("[MQTT] Unsubscribing to %s (PID %d)\n"), topic,
                    packetId);
#else
        _mqtt.unsubscribe(topic);
        DEBUG_MSG_P(PSTR("[MQTT] Unsubscribing to %s\n"), topic);
#endif
    }
}

// -----------------------------------------------------------------------------
// MQTT Callbacks
// -----------------------------------------------------------------------------

void _mqttCallback(unsigned int type, const char* topic, const char* payload, bool retained, bool isEncrypted) {
    if (type == MQTT_CONNECT_EVENT) {
        // Subscribe to internal action topics
        // We need not subscribe to this topic as we are using config topic
        // itself for sending rpc actions as well.
        // mqttSubscribe(MQTT_TOPIC_ACTION, 2);
        resetMqttBeatFlags();
        setHeartBeatSchedule(millis());
        _mqtt_connect_try = 0;
    }

    if (type == MQTT_MESSAGE_EVENT) {
        if (retained) {
            return;
        }

        /*
        String t = mqttSubtopic((char*)topic);
        if (t.startsWith(MQTT_TOPIC_ACTION)) {
            #if SAVE_CRASH_INFO
            if (strcmp(payload, "CRASH") == 0) {
                // deferredReset(1000, CUSTOM_RESET_MQTT);
                Serial.println("Will Crash Now");
                delay(2000);
                Serial.println("Oops");
            }
            else if (strcmp(payload, "sendcrashd") == 0){
                sendCrashDump(true);
            }
            #endif
        }
        */
    }
}

void resetMqttBeatFlags() {
    // Reset all flags for all the first connection things to mqtt
    mqtt_firstbeat_reported = false;
}

void setHeartBeatSchedule(unsigned long heart_beat) {
    schedule_heartbeat = heart_beat;
}

void _mqttOnConnect() {
    // mqttConnectedTime=millis();
    DEBUG_MSG_P(PSTR("[MQTT] Connected!\n"));
    _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MIN;
    // TEMPORARY
    wifiDisconnectedCount = 0;

    // #if MQTT_SKIP_RETAINED
    mqttConnectedAt = millis();
    // #endif
    // Build MQTT topics
    // mqttConfigure();
    // Clean subscriptions
    mqttUnsubscribeRaw("#");

    // Send connect event to subscribers
    for (unsigned char i = 0; i < _mqtt_callbacks.size(); i++) {
        (_mqtt_callbacks[i])(MQTT_CONNECT_EVENT, NULL, NULL, false, false);
    }

    // setLedPattern(LED_PAT_MQTT_CONNECTED); // TODO_S2 related to wifi status led
    // ledPattern = 6;
    // dontCheckWiFiStatus = true; // TODO_S2 related to wifi status led
}

void _mqttOnDisconnect() {
    // dontCheckWiFiStatus = false; // TODO_S1 related to wifi status led

    // checkWifiForLedStatus(); // TODO_S1 related to wifi status led

    // Send disconnect event to subscribers - NOTE: This is not very reliable
    // though. Sometimes, On force close, i think this may not be called.
    for (unsigned char i = 0; i < _mqtt_callbacks.size(); i++) {
        (_mqtt_callbacks[i])(MQTT_DISCONNECT_EVENT, NULL, NULL, false, false);
    }
}
std::unique_ptr<char[]> mqttPayloadBuffer;

void _mqttOnMessage(char* topic, char* payload, int length) {
    _mqttOnMessage(topic, payload, length, 0, length);
}

void _mqttOnMessage(char* topic, char* payload, unsigned int len, int index, int total) {
    bool retained = false;
    // DEBUG_MSG_P(PSTR("[DEBUG] Mesage HEYAAHH ! \n"));

    DEBUG_MSG_P(PSTR("[MQTT] Received %s \n"), topic);

#if MQTT_SKIP_RETAINED
    if (millis() - mqttConnectedAt < MQTT_SKIP_TIME) {
        DEBUG_MSG_P(PSTR(" - Retaind Message\n"));
        retained = true;
        // Only Config Topics should be handled
        String t = mqttSubtopic((char*)topic);
        if (!t.startsWith(MQTT_TOPIC_CONFIG)) return;
    }
#endif

    // Check system topics
    // String t = mqttSubtopic((char *) topic);

    if (mqttPayloadBuffer == nullptr || index == 0) mqttPayloadBuffer = std::unique_ptr<char[]>(new char[total + 1]);  // empty the buffer

    memcpy(mqttPayloadBuffer.get() + index, payload, len);  // copy the content into it

    DEBUG_MSG_P(PSTR("[DEBUG] Index : %d, Len : %d, Total : %d \n"), index, len, total);

    if (index + len != total) return;  // return if payload buffer is not complete
    mqttPayloadBuffer[total] = '\0';

    // Total is the length of mqttPayloadBuffer

    bool isEncrypted = false;

    if (total > 4) {
        if (mqttPayloadBuffer[0] == 'E' && mqttPayloadBuffer[1] == 'N' &&
            mqttPayloadBuffer[2] == 'C' && mqttPayloadBuffer[3] == ':') {
            isEncrypted = true;
        }
        // mqttPayloadBuffer = mqttPayloadBuffer.get()+4;
    }

    if (isEncrypted) {
        String MessageStr = "";
        // int bufferSize = 1024;
        // int requiredSize = JSON_OBJECT_SIZE(3) + total;
        int bufferSize = JSON_OBJECT_SIZE(3) + total + 10;

        // if(requiredSize < 512){
        //     bufferSize = 512;
        // }else if(requiredSize < 1024) {
        //     bufferSize = 1024;
        // }else if(requiredSize < 1536) {
        //     bufferSize = 1536;
        // }else if(requiredSize < 2048) {
        //     bufferSize = 2048;
        // }else{
        //     bufferSize = 3072;
        // }
            
        //     10;  // Total is for the input duplication , and since we are only
                 // having 2 objects inside - payload and meta - we are taking 3
                 // just for safety purpose.
        // DEBUG_MSG_P(PSTR("[DEBUG] JsonBuffer Length : %d \n"), bufferSize);
        DynamicJsonDocument root(bufferSize);

        DeserializationError error =
            deserializeJson(root, mqttPayloadBuffer.get() + 4);
        if (!error) {
            int payloadLength = strlen(root["payload"]);
            // DEBUG_MSG_P(PSTR("[DEBUG] Payload Length : %d \n"),
            // payloadLength);

            char data_decrypted[payloadLength + 1];  
                                     // Maximum size of data-decrypted can go up
                                     // to payload size only, because cipher
                                     // length weill always be bigger then the
                                     // actual message.
            memset(&data_decrypted[0], 0, payloadLength + 1);
            data_decrypted[0] = '\0';

            decryptMsg((const char*)root["payload"], data_decrypted, payloadLength, (const char*)root["meta"]);
            for (unsigned char i = 0; i < _mqtt_callbacks.size(); i++) {
                (_mqtt_callbacks[i])(MQTT_MESSAGE_EVENT, topic, data_decrypted, retained, isEncrypted);
            }
        }

        else {
            DEBUG_MSG_P(PSTR("[ERROR] parseObject() failed for MQTT NOw \n"));
        }

    } else {
        // DEBUG_MSG_P(PSTR("[DEBUG] Message Recieved : %s \n"),
        // mqttPayloadBuffer.get());
        for (unsigned char i = 0; i < _mqtt_callbacks.size(); i++) {
            (_mqtt_callbacks[i])(MQTT_MESSAGE_EVENT, topic, mqttPayloadBuffer.get(), retained, isEncrypted);
        }
    }

    // releasing the buffer
    char* buffPoint = mqttPayloadBuffer.release();
    delete buffPoint;
}
void mqttStatusBeat() {}

void _mqttConnect() {
    // Do not connect if disabled
    if (!_mqtt_enabled) return;

    // Do not connect if already connected
    if (mqtt.connected()) return;

    // Check if MqttConf exists - if not then set FetchMqttConf
    if (!_mqttServer.length() || !_mqttUser.length() || !_mqttPass.length() || !_mqtt_port) {
        // Mqtt Conf Does not exist.
        // Please fetch conf again
        // DEBUG_MSG_P(PSTR("[DEBUG] Coming here \n"));
        mqttConfigure();
        setFetchMqttConf(true);
        return;
    }

    // Check reconnect interval
    static unsigned long last = 0;
    if (millis() - last < _mqtt_reconnect_delay) return;
    last = millis();

    // Increase the reconnect delay
    _mqtt_reconnect_delay += MQTT_RECONNECT_DELAY_STEP;
    if (_mqtt_reconnect_delay > MQTT_RECONNECT_DELAY_MAX) {
        _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MAX;
    }

// Last option: reconnect to wifi after MQTT_MAX_TRIES attemps in a row
#if MQTT_MAX_TRIES > 0
    if (++_mqtt_connect_try > MQTT_MAX_TRIES) {
        if (wifiDisconnectedCount > 10) {
            DEBUG_MSG_P(
                PSTR("[MQTT] CODE RED. Maximum wifi disonnect has also "
                     "reached. Asking Server for what to do next ? "));
            setFetchRPCAction(
                true);  // Although this function is async and will fetch it in
                        // next loop. But we are checking positive action. so if
                        // it fails, it'll keep retrying.
            wifiDisconnectedCount = 0;
        } else {
            DEBUG_MSG_P(
                PSTR("[MQTT] MQTT_MAX_TRIES met, disconnecting from WiFi\n"));
            _mqtt_connect_try = 0;
            // TEMPORARY
            wifiDisconnectedCount++;
            // wifiDisconnectSafe(); TODO_S1
            return;
        }
    }
#endif

    // mqtt.disconnect();

    /*
    char* host = strdup(h.c_str());
    if (_mqttUser) free(_mqttUser);
    if (_mqttPass) free(_mqttPass);
    if (_mqttWill) free(_mqttWill);
    if (_mqttClientId) free(_mqttClientId);

    _mqttClientId = strdup((String("dev_") + getSetting("identifier")).c_str());
    _mqttUser = strdup(User.c_str());
    _mqttPass = strdup(Pass.c_str());
    _mqttWill = strdup((mqttTopic + MQTT_TOPIC_STATUS).c_str());
    // DEBUG_MSG_P(PSTR("MQTT Will : %s"), _mqttWill);

    DEBUG_MSG_P(PSTR("[MQTT] Connecting to broker at %s:%d"), host, port);
    mqtt.setServer(host, port); */

#if MQTT_USE_ASYNC
    mqtt.setServer(_mqttServer.c_str(), _mqtt_port);
    mqtt.setClientId(_mqttClientId.c_str());
    mqtt.setKeepAlive(_mqtt_keepalive);
    mqtt.setCleanSession(false);
    mqtt.setWill(_mqttWill.c_str(), _mqtt_qos, true, MQTT_STATUS_OFFLINE);

    if (_mqttUser.length() > 0 && _mqttPass.length() > 0) {
        DEBUG_MSG_P(PSTR("[MQTT] Connecting as user %s\n"), _mqttUser.c_str());
        mqtt.setCredentials(_mqttUser.c_str(), _mqttPass.c_str());
    }
#if ASYNC_TCP_SSL_ENABLED

    bool secure = getSetting("mqttUseSSL", MQTT_SSL_ENABLED).toInt() == 1;
    mqtt.setSecure(secure);
    if (secure) {
        DEBUG_MSG_P(PSTR("[MQTT] Using SSL\n"));
        unsigned char fp[20] = {0};

        // Not adding Fingerprint.
        // Doing thisin favor of not verifyng fingerprint for now. But the
        // traffic still remains encrypted.

        if (sslFingerPrintArray(getSetting("mqttFP", MQTT_SSL_FINGERPRINT).c_str(), fp)) {
            mqtt.addServerFingerprint(fp);
        } else {
            DEBUG_MSG_P(PSTR("[MQTT] Wrong fingerprint\n"));
        }
    }

#endif  // ASYNC_TCP_SSL_ENABLED
    DEBUG_MSG_P(PSTR("[MQTT] Connecting to broker at %s:%u\n"), _mqttServer.c_str(), _mqtt_port);
    DEBUG_MSG_P(PSTR("[MQTT] Client ID: %s\n"), _mqttClientId.c_str());
    DEBUG_MSG_P(PSTR("[MQTT] QoS: %d\n"), _mqtt_qos);
    DEBUG_MSG_P(PSTR("[MQTT] Retain flag: %d\n"), _mqtt_retain ? 1 : 0);
    DEBUG_MSG_P(PSTR("[MQTT] Keepalive time: %ds\n"), _mqtt_keepalive);
    DEBUG_MSG_P(PSTR("[MQTT] Will topic: %s\n"), _mqttWill.c_str());
    mqtt.connect();

#else

    bool response;

    if ((strlen(_mqttUser) > 0) && (strlen(_mqttPass) > 0)) {
        DEBUG_MSG_P(PSTR(" as user '%s'\n"), _mqttUser);
        response = mqtt.connect(getIdentifier().c_str(), _mqttUser, _mqttPass, _mqttWill, _mqtt_qos, _mqtt_retain, "0");

    } else {
        DEBUG_MSG_P(PSTR("\n"));
        response = mqtt.connect(getIdentifier().c_str(), _mqttWill, _mqtt_qos, _mqtt_retain, "0");
    }

    if (response) {
        _mqttOnConnect();
        _mqtt_connected = true;
    } else {
        DEBUG_MSG_P(PSTR("[MQTT] Connection failed\n"));
    }

#endif
}
void _mqttOnPublish(uint16_t packtId) {
    DEBUG_MSG_P(PSTR("MQtt Message Sent: %d \n"), packtId);
}

void mqttReset() {
    mqttConfigure();
    mqttDisconnect();
}
bool canSendMqttMessage() {
    return (mqtt.connected() && millis() - mqttConnectedAt > MQTT_STABLE_TIME);
}
void mqttSetup() {
    DEBUG_MSG_P(PSTR("[MQTT] Async %s, SSL %s \n"),
                MQTT_USE_ASYNC ? "ENABLED" : "DISABLED",
                ASYNC_TCP_SSL_ENABLED ? "ENABLED" : "DISABLED");

#if MQTT_USE_ASYNC
    mqtt.onConnect([](bool sessionPresent) { _mqttOnConnect(); });
    mqtt.onDisconnect([](AsyncMqttClientDisconnectReason reason) {
        if (reason == AsyncMqttClientDisconnectReason::TCP_DISCONNECTED) {
            DEBUG_MSG_P(PSTR("[MQTT] TCP Disconnected\n"));
        }
        if (reason == AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED) {
            DEBUG_MSG_P(PSTR("[MQTT] Identifier Rejected\n"));
        }
        if (reason == AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE) {
            DEBUG_MSG_P(PSTR("[MQTT] Server unavailable\n"));
        }
        if (reason == AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS) {
            DEBUG_MSG_P(PSTR("[MQTT] Malformed credentials\n"));
        }
        if (reason == AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED) {
            DEBUG_MSG_P(PSTR("[MQTT] Not authorized\n"));
        }
#if ASYNC_TCP_SSL_ENABLED
        if (reason == AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT) {
            DEBUG_MSG_P(PSTR("[MQTT] Bad fingerprint\n"));
        }
#endif

        _mqttOnDisconnect();
        // return;
    });
    mqtt.onMessage([](char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
        // DEBUG_MSG_P(PSTR("Message Length, Total %d \n"),len,total );
        _mqttOnMessage(topic, payload, len, index, total);
        // If message is empty i do not want to take any action - it may fuss with another things.
    });
    mqtt.onSubscribe([](uint16_t packetId, uint8_t qos) {
        // DEBUG_MSG_P(PSTR("[MQTT] Subscribe ACK for PID %d\n"), packetId);
    });
    mqtt.onPublish([](uint16_t packetId) {
        // DEBUG_MSG_P(PSTR("[MQTT] Publish ACK for PID %d\n"), packetId);
    });

#else
    mqtt.setCallback([](char* topic, byte* payload, unsigned int length) {
        _mqttOnMessage(topic, (char*)payload, length);
        return;
    });
#endif

    // Disconnect from mqtt if wifi is disconnected.
    // TODO_S1 - enabled this function 
    // wifiRegister([](justwifi_messages_t code, char* parameter) {
    //     if (code == MESSAGE_DISCONNECTED && mqttConnected()) {
    //         delay(50);  // This delay is neccesary in esp 32, other wise it is
    //                     // crashing in mqtt.disconnect
    //         mqttDisconnect();
    //     }
    //     // Because disconnect wont be fired if device was connected and wifi disappeared
    //     else if (code == MESSAGE_CONNECTING && mqttConnected()) {
    //         mqttDisconnect();
    //     }
    // });
    mqttConfigure();
    mqttRegister(_mqttCallback);
    setConfigKeys(_mqtt_config_keys);
    afterConfigParseRegister(mqttConfigure);
    loopRegister(mqttLoop);
}

void mqttLoop() {
    if (WiFi.status() != WL_CONNECTED) return;

#if MQTT_USE_ASYNC

    _mqttConnect();

#else  // not MQTT_USE_ASYNC

    if (_mqtt.connected()) {
        _mqtt.loop();

    } else {
        if (_mqtt_connected) {
            _mqttOnDisconnect();
            _mqtt_connected = false;
        }
        _mqttConnect();
    }

#endif
}
