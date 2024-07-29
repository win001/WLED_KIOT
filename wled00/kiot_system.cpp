#include "wled.h"
// -----------------------------------------------------------------------------

unsigned long _loopDelay = 0;
unsigned int _heartbeat_interval = 0;
unsigned short int _load_average = 100;
bool need_to_report_ping_fault = false;

bool _serialDisabled = false;

// -----------------------------------------------------------------------------

#if SYSTEM_CHECK_ENABLED

// Call this method on boot with start=true to increase the crash counter
// Call it again once the system is stable to decrease the counter
// If the counter reaches SYSTEM_CHECK_MAX then the system is flagged as
// unstable setting _systemOK = false;
//
// An unstable system will only have serial access, WiFi in AP mode and OTA

bool _systemStable = true;

void systemCheck(bool stable) {
    unsigned char value = EEPROM.read(EEPROM_CRASH_COUNTER);
    if (stable) {
        value = 0;
        DEBUG_MSG_P(PSTR("[MAIN] System OK\n"));
    } else {
        if (++value > SYSTEM_CHECK_MAX) {
            _systemStable = false;
            value = 0;
            DEBUG_MSG_P(PSTR("[MAIN] System UNSTABLE\n"));
        }
    }
    EEPROM.write(EEPROM_CRASH_COUNTER, value);
    // EEPROM.commit();
    saveSettings();
}

bool systemCheck() { return _systemStable; }

void systemCheckLoop() {
    static bool checked = false;
    if (!checked && (millis() > SYSTEM_CHECK_TIME)) {
        // Check system as stable
        systemCheck(true);
        checked = true;
    }
}

#endif

// -----------------------------------------------------------------------------

void _settings_config_keys(DynamicJsonDocument& root, char* topic,
                           bool nested_under_topic) {
    strncpy_P(topic, MQTT_TOPIC_GENERAL, MAX_CONFIG_TOPIC_SIZE);
    if (nested_under_topic) {
        JsonObject data = root.createNestedObject(topic);
        data["hb_int"] = getSetting("hb_int", HEARTBEAT_INTERVAL).toInt();
        // General Settings
        data["homeId"] = getSetting("homeId", "");

        data["otaE"] = getSetting("otaE", NOFUSS_ENABLED).toInt() == 1 ? 1 : 0;
        data["otaS"] = getSetting("otaS", NOFUSS_SERVER);
        data["otaI"] = getSetting("otaI", NOFUSS_INTERVAL).toInt();

    } else {
        root["hb_int"] = getSetting("hb_int", HEARTBEAT_INTERVAL).toInt();

        // General Settings
        root["homeId"] = getSetting("homeId", "");

        root["otaE"] = getSetting("otaE", NOFUSS_ENABLED).toInt() == 1 ? 1 : 0;
        root["otaS"] = getSetting("otaS", NOFUSS_SERVER);
        root["otaI"] = getSetting("otaI", NOFUSS_INTERVAL).toInt();
    }
}

void reportPingFaultEvent(bool value) { need_to_report_ping_fault = value; }

bool needToReportPingFaultEvent() { return need_to_report_ping_fault; }

void systemLoop() {
// Check system stability
#if SYSTEM_CHECK_ENABLED
    systemCheckLoop();
#endif
    nowstamp = millis();

#if HEARTBEAT_ENABLED
    // Heartbeat
    static unsigned long last = 0;
    bool send_heartbeat_now = false;

    if ((last == 0) || (nowstamp - last > _heartbeat_interval * 1000)) {
        send_heartbeat_now = true;
    }
#endif  // HEARTBEAT_ENABLED

    if (schedule_heartbeat &&
        nowstamp - schedule_heartbeat >= MQTT_STABLE_TIME) {
#if HEARTBEAT_ENABLED
        send_heartbeat_now = true;
#endif

#if MQTT_ACTIVE_STATE_REPORT
        activeHomePong(true);
#endif
        schedule_heartbeat = 0;
    }
#if HEARTBEAT_ENABLED
    if (send_heartbeat_now) {
        heartbeat_new();
        last = millis();
    }
#endif
    // -------------------------------------------------------------------------
    // Load Average calculation
    // -------------------------------------------------------------------------

    static unsigned long last_loadcheck = 0;
    static unsigned long load_counter_temp = 0;
    load_counter_temp++;

    if (millis() - last_loadcheck > LOADAVG_INTERVAL) {
        static unsigned long load_counter = 0;
        static unsigned long load_counter_max = 1;

        load_counter = load_counter_temp;
        load_counter_temp = 0;
        if (load_counter > load_counter_max) {
            load_counter_max = load_counter;
        }
        _load_average = 100 - (100 * load_counter / load_counter_max);
        last_loadcheck = millis();

        // unsigned int frags = ESP.getHeapFragmentation();
        // DEBUG_MSG_P(PSTR(frags));
        // Serial.println(frags);
    }

    // -------------------------------------------------------------------------
    // User requested reset
    // -------------------------------------------------------------------------
    if (checkNeedsReset()) {
        resetActual();
    }

    // -------------------------------------------------------------------------
    // Check for wifi pin fault event.
    // -------------------------------------------------------------------------
    if (needToReportPingFaultEvent()) {
        if (reportEventToMQTT(EVENT_PING_FAULT, "")) {
            reportPingFaultEvent(false);
        }
    }
    // Power saving delay
    // delay(_loopDelay); TODO_S2
}

unsigned long systemLoadAverage() { return _load_average; }

bool serialDisabled(){
    return _serialDisabled;
}
void serialDisabled(bool value){
    _serialDisabled = value;
}

void systemSetup() {
    // EEPROM.begin(EEPROM_SIZE);
    DEBUG_MSG_P(PSTR("[SYSTEM] initialising system.cpp \n"));
    // TODO_S2 serial is already initialized in by wled code (initialising serial two time will cause crash)
// #if DEBUG_SERIAL_SUPPORT
//     DEBUG_PORT.begin(SERIAL_BAUDRATE);
// #if DEBUG_ESP_WIFI
//     DEBUG_PORT.setDebugOutput(true);
// #endif
//     Serial.println("After serial begin\n");
// #elif defined(SERIAL_BAUDRATE)
//     Serial.begin(SERIAL_BAUDRATE);
// #endif
    

// Question system stability
#if SYSTEM_CHECK_ENABLED
    systemCheck(false);
#endif

    setConfigKeys(_settings_config_keys);

    // Cache loop delay value to speed things (recommended max 250ms)
    _loopDelay = atol(getSetting("loopDelay", LOOP_DELAY_TIME).c_str());
    _heartbeat_interval = getSetting("hb_int", HEARTBEAT_INTERVAL).toInt();
    loopRegister(systemLoop);
}
