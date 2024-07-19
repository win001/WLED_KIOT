#include "wled.h"

#if MQTT_ACTIVE_STATE_REPORT || MQTT_REPORT_EVENT_TO_HOME
unsigned long last_home_ping = 0, last_home_pong = 0;
#endif

/* void _home_config_keys(DynamicJsonDocument& root, char * topic, bool nested_under_topic) {
    strncpy_P(topic, MQTT_TOPIC_ALEXA, MAX_CONFIG_TOPIC_SIZE);
    root["homeId"] = getSetting("homeId", "");
} */
bool _reportHome = false;
void relayString(JsonObject& root) { // TODO_S2
    JsonArray relay = root.createNestedArray("relays");
    for (unsigned char i = 1; i <= 5; i++) {
        relay.add(1);
    }
    return;
}
void homeSetupMQTT() {
    mqttRegister(homeMQTTCallback);
}

void homeReportSetup() {
    homeSetupMQTT();
    // Moved to general Settings in system.ino
    // setConfigKeys(_home_config_keys);
}
void homePong() {
    String homeId = getSetting("homeId");
    if (!homeId.length()) {
        return;
    }
    if (!canSendMqttMessage()) return;

#if MQTT_ACTIVE_STATE_REPORT
    last_home_pong = nowstamp;
#endif


    char buffer[homeId.length() + 15];
    char messageBuffer[512] = "";

    // String pingString = getDeviceStatus();

    StaticJsonDocument<1024> jsonBuffer;
    JsonObject root = jsonBuffer.to<JsonObject>();

// #if RELAY_COUNT > 0 || RELAY_PROVIDER == RELAY_PROVIDER_KU
#if 1 // TODO_S2
    JsonObject relayObj = root.createNestedObject(MQTT_TOPIC_RELAY);
    relayString(relayObj);
#endif

// #if RELAY_PROVIDER == RELAY_PROVIDER_KU && CUSTOM_DP_SUPPORT
//     root[MQTT_TOPIC_CUSTOM_DP] = jsonBuffer.to<JsonObject>();
//     customDpString(&root[MQTT_TOPIC_CUSTOM_DP]);
// #endif

// #if MESH_GATEWAY_SUPPORT
//     sendMeshDeviceStatuses();
// #endif
    serializeJson(root, messageBuffer, sizeof(messageBuffer));
    sprintf(buffer, "%s/%s", homeId.c_str(), MQTT_TOPIC_PING);
    mqttSend(buffer, messageBuffer, false, DONT_ENCRYPT_HOME_PING, true, MQTT_RETAIN);    

    return;
}

void reportEventToHome(String name, String value) {
    DEBUG_MSG_P(PSTR("[Home] Message - %s, %s \n"), name.c_str(), value.c_str());
    String homeId = getSetting("homeId");
    if (!homeId.length()) {
        return;
    }
    if (!canSendMqttMessage()) return;

    char buffer[homeId.length() + 15];
    char messageBuffer[200] = "";

    StaticJsonDocument<256> root;
    JsonObject event = root.createNestedObject(String(MQTT_TOPIC_EVENT));
    event["name"] = name;
    event["val"] = value;
    serializeJson(root, messageBuffer, sizeof(messageBuffer));
    sprintf(buffer, "%s/%s", homeId.c_str(), MQTT_TOPIC_PING);
    mqttSend(buffer, messageBuffer, false, DONT_ENCRYPT_HOME_PING, true, MQTT_RETAIN);
}

void homeLoop() {
#if MQTT_ACTIVE_STATE_REPORT
    activeHomePong(false);
#endif
}
void homeSetup() {
    loopRegister(homeLoop);
}

/* 

String getDeviceStatus(){
    StaticJsonDocument<500> root; 
    #if defined(ENABLE_HLW8012) && (ENABLE_HLW8012)
        root[MQTT_TOPIC_POWER] = jsonBuffer.createObject();
        getPowerString(root[MQTT_TOPIC_POWER]);
    #endif
    #ifndef NO_RELAYS
        root[MQTT_TOPIC_RELAY] = relayString();
    #endif
    #ifndef NO_SENSORS
        root[MQTT_TOPIC_SENSOR] = sensorString(true);
    #endif
    String output;
    serializeJson(root, output);
    return output;
}

*/

void homeMQTTCallback(unsigned int type, const char* topic, const char* payload, bool retained, bool isEncrypted) {
    String homeId = getSetting("homeId");
    if (homeId == "") {
        return;
    }
    if (type == MQTT_CONNECT_EVENT) {
        char buffer[strlen(MQTT_TOPIC_PING) + homeId.length() + 5];
        snprintf_P(buffer, sizeof(buffer), PSTR("%s/%s"), homeId.c_str(), MQTT_TOPIC_PING);
        // DEBUG_MSG_P(PSTR("[DEBUG] MqttTopicHomePing %s \n"),buffer);
        mqttSubscribe(buffer);

#if MQTT_ACTIVE_STATE_REPORT
        last_home_ping = millis();
#endif
        // homePong();
        return;
    }

    if (type == MQTT_MESSAGE_EVENT) {
        if (retained && !MQTT_HANDLE_RET_PING) {
            return;
        }

        // Match topic
        String t = mqttSubtopic((char*)topic);
        if (!t.startsWith(homeId)) return;
        if (t.endsWith(MQTT_TOPIC_PING)) {
#if MQTT_ACTIVE_STATE_REPORT
            last_home_ping = millis();
#endif
        DEBUG_MSG_P(PSTR("[DEBUG] Mess- %s\n"), payload);
        homePong();
        return;  
    }     

    }
}
void needReportHome(bool report) {
    _reportHome = report;
}
bool needReportHome() {
    return _reportHome;
}

#if MQTT_ACTIVE_STATE_REPORT || MQTT_REPORT_EVENT_TO_HOME

void activeHomePong(bool immidiate) {
    // If immidiate set to true then send HomePing weather or not home_ping is active.
    if (immidiate) {
        homePong();
    } else if (last_home_ping != 0 && (nowstamp - last_home_ping) < MQTT_ACTIVE_STATE_TIMEOUT) {
        if ((nowstamp - last_home_pong) > MQTT_ACTIVE_STATE_HOME_PONG_INTERVAL || needReportHome()) {
            homePong();
            needReportHome(false);
            // last_home_pong = nowstamp;
        }
    }
}

#endif