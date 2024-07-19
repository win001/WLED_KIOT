
#include "wled.h"
// #include <AsyncJson.h>
// #include <ESPAsyncTCP.h>
// #include <ESPAsyncWebServer.h>
#include <Hash.h>
// #include <Ticker.h>
#include <map>
// #include <vector>
// #include <FS.h>

#if EMBEDDED_WEB
#include "static/index.html.gz.h"
#endif

#if ASYNC_TCP_SSL_ENABLED & WEB_SSL_ENABLED
#include "static/server.cer.h"
#include "static/server.key.h"
#endif

AsyncWebServer *_server;
Ticker deferred;
unsigned long lastRequest = 0;

typedef struct
{
    char *url;
    char *key;
    apiGetCallbackFunction getFn = NULL;
    apiPostCallbackFunction postFn = NULL;
} web_api_t;
std::vector<web_api_t> _apis;
char _last_modified[50];

std::vector<after_config_parse_callback_f> _after_config_parse_callbacks;

void afterConfigParseRegister(after_config_parse_callback_f callback) {
    _after_config_parse_callbacks.push_back(callback);
}

void test_after_config_parse_callbacks() {
    for (unsigned char i = 0; i < _after_config_parse_callbacks.size(); i++) {
        (_after_config_parse_callbacks[i])();
    }
}

void postConfigurationUpdated() {
#if OTA_SUPPORT
    _otaConfigure();
#endif
}

// from is the source who is calling this function = web/mqtt/hclient
// buffer is used in case of config coming from web. we need rto send some reply.
void processConfig(JsonObject config, const char *from, char *buffer, size_t bufferSize) {
    DEBUG_MSG_P(PSTR("[CONFIG] Parsing config data \n"));
    bool save = false;
    bool changedMQTT = false;
    bool restart = false;
    bool changedWifi = false;
    bool wifiSupplied = false;
    bool preConfigured = true;
    String apiKey = getSetting("apiKey", "");
    StaticJsonDocument<1024> resRoot;

    if (!apiKey.length()) {
        preConfigured = false;
    }

    for (auto kv : config) {
        bool changed = false;
        String key = String(kv.key().c_str());

        JsonVariant value = kv.value();

        // KIOT Filter Keys here.
        // We cant let api update all the keys.
        if (key.equals("filename")) continue;

        if (key.equals("aesKey") || key.equals("apiSecret") || key.equals("apiKey") || key.equals("devPin") || key.equals("httpPass")) {
            continue;
        }

        // Handling Special Case of Wifi Here.
        // TODO - 2.0.0 - password field for wifi will be coming as encrypted.

        if (key.equals("wifi")) {
            wifiSupplied = true;
            if (!value.is<JsonArray>()) {
                // Throw Error
            } else {
                JsonArray arr = value.as<JsonArray>();
                if (configStoreWifi(arr)) changed = true;
            }
        } else {
            // Store values for everything else other than wifi
            if (value.is<JsonArray>()) {
                JsonArray arr = value.as<JsonArray>();
                if (configStore(key, arr)) changed = true;
            } else {
                if (configStore(key, value.as<String>())) {
                    changed = true;
                    // DEBUG_MSG_P(PSTR("[DEBUG] Changed set to tru for the key : %s"), key.c_str());
                }
            }
        }

        // Update flags if value has changed
        if (changed) {
            save = true;
            if (key.startsWith("mqtt") || key.equals("homeId"))
                changedMQTT = true;
            if (key.startsWith("wifi")) changedWifi = true;
            // Handling Sensors Settings
        }
    }
    if (strcmp(from, "web") == 0) {
        if (!preConfigured) {
            String apiKey = generateApiKey();
            setSetting("apiKey", apiKey);
            save = true;
            char buffer1[250] = "";
            getApiKeyChecksum(buffer1);
            DEBUG_MSG_P(PSTR("[DEBUG] Length of buffer1 - %d \n"), strlen(buffer1));

            resRoot["cs"] = buffer1;
            #if (RELAY_PROVIDER == RELAY_PROVIDER_KU) && (KU_SERIAL_SUPPORT)
                // TODO_S1 handle all the config according to your need or removed
                resRoot["res_v"] = 2;
                resRoot["rn"] = 5;//relayCount();
                resRoot["dn"] = 0;//dimmerCount();
                JsonArray relayType = resRoot.createNestedArray("rT");
                JsonArray relayFeature = resRoot.createNestedArray("rF");
                JsonArray dimmingSteps = resRoot.createNestedArray("dS");
                uint8_t relay_type = 0;
                // for(int i=0; i<relayCount(); i++) {
                //     relay_type = getRelayType(i+1);
                //     relayType.add(relay_type);
                //     if(relay_type == 2) {
                //         dimmingSteps.add(getDimmingSteps(i+1));
                //     }
                //     relayFeature.add(getRelayFeature(i+1));
                // }
                for(int i=0; i<5; i++) {
                    relay_type = 1;//getRelayType(i+1);
                    relayType.add(relay_type);
                    if(relay_type == 2) {
                        dimmingSteps.add(getDimmingSteps(i+1));
                    }
                    relayFeature.add(1);
                }
                resRoot["ir"] = IR_SUPPORT;
                JsonArray power = resRoot.createNestedArray("pwr");
                #if POWER_HAS_ACTIVE
                    power.add("p");
                #endif
                power.add("p");
            #endif
        }
    }

    if (save) {
        // Persist settings
        saveSettings();
    }
    if (wifiSupplied) {
        restart = true;
    }
    if (restart) {
        deferredReset(2000, CUSTOM_RESET_WEB); // TODO_S2
    } else {
        if (save) {
            // Callbacks
            for (unsigned char i = 0; i < _after_config_parse_callbacks.size(); i++) {
                (_after_config_parse_callbacks[i])();
            }

            if (changedMQTT) {
                deferred.once_ms(100, mqttReset); // TODO_S2
            }
        }
    }
    String output;
    resRoot["message"] = "success";
    serializeJson(resRoot, output);
    DEBUG_MSG_P(PSTR("%s \n"), output.c_str());
    snprintf(buffer, bufferSize, output.c_str());
}

void getApiKeyChecksum(char *inpBuf) {
    char iv_encoded[30] = "";
    char message[60] = "";

    snprintf(message, 60, "{\"apiKey\": \"%s\"}", getSetting("apiKey").c_str());

    char data_encoded[getBase64Len(getCipherlength(strlen(message)) + 1)];
    memset(data_encoded, '\0', sizeof(data_encoded));

    DEBUG_MSG_P(PSTR("[DEBUG] Len : %d, Message : %s"), strlen(message), message);
    encryptMsg(message, data_encoded, iv_encoded);
    int buffer_size = sizeof(data_encoded) + sizeof(iv_encoded) + 25;

    DEBUG_MSG_P(PSTR("[DEBUG] BUffer Reserved : %d \n"), buffer_size);

    char buffer[buffer_size];
    memset(buffer, 0, buffer_size);
    snprintf(buffer, buffer_size, "{\"m\":\"%s\",\"p\":\"%s\"}", iv_encoded, data_encoded);
    // DEBUG_MSG_P(PSTR("[DEBUG] Buffer Len : %d, p: %s"), strlen(buffer),buffer);
    // int bufLen = strlen(buffer);
    // unsigned int base64len = getBase64Len(bufLen);
    // char b64data[base64len];
    // memset(b64data, 0, base64len);

    base64_encode(inpBuf, (char *)buffer, strlen(buffer));
}

void getInfo(JsonObject root) {
    root["app"] = APP_NAME;
    root["version"] = APP_VERSION;
    root["rev"] = APP_REVISION;
    root["settings_version"] = SETTINGS_VERSION;
    root["sensors_version"] = SENSORS_VERSION;
    root["buildDate"] = __DATE__;
    root["buildTime"] = __TIME__;
    root["manufacturer"] = String(MANUFACTURER);
    root["mac"] = WiFi.macAddress();
    root["device"] = String(DEVICE);
    root["hostname"] = getSetting("hostname", HOSTNAME);
    // TODO_S1 implement all the below functions
    // root["network"] = getNetwork();
    // root["deviceip"] = getIP();
    // root["heap"] = getFreeHeap();
    // root["uptime"] = getUptime();
    // root["rssi"] = WiFi.RSSI();
    // root["distance"] = wifiDistance(WiFi.RSSI());
    // root["mqttStatus"] = mqttConnected();
    // root["heap"] = getFreeHeap();
    // root["loadaverage"] = systemLoadAverage();
#if 0//ADC_MODE_VALUE == ADC_VCC
    root["vcc"] = ESP.getVcc();
#endif
#if NTP_SUPPORT
    if (ntpSynced())
        root["now"] = now();
#endif

    // root["maxNetworks"] = WIFI_MAX_NETWORKS;
    // JsonArray& wifi = root.createNestedArray("wifi");
    // for (byte i = 0; i < WIFI_MAX_NETWORKS; i++)
    // {
    //     if (getSetting("ssid" + String(i)).length() == 0)
    //         break;
    //     JsonObject& network = wifi.createNestedObject();
    //     network["ssid"] = getSetting("ssid" + String(i));
    //     // network["pass"] = getSetting("pass" + String(i));
    //     network["ip"] = getSetting("ip" + String(i));
    //     network["gw"] = getSetting("gw" + String(i));
    //     network["mask"] = getSetting("mask" + String(i));
    //     network["dns"] = getSetting("dns" + String(i));
    // }

    // root["relayMode"] = getSetting("relayMode", RELAY_BOOT_MODE);
    // root["relayPulseMode"] = getSetting("relayPulseMode", RELAY_PULSE_MODE);
    // root["relayPulseTime"] = getSetting("relayPulseTime", RELAY_PULSE_TIME);
    // if (relayCount() > 1) {
    //     root["multirelayVisible"] = 1;
    //     root["relaySync"] = getSetting("relaySync", RELAY_SYNC);
    // }

    // root["webPort"] = getSetting("webPort", WEB_PORT).toInt();

    // root["apiEnabled"] = getSetting("apiEnabled").toInt() == 1;
    // root["apiKey"] = getSetting("apiKey");

    // root["tmpUnits"] = getSetting("tmpUnits", TMP_UNITS).toInt();
    /*
#if ALEXA_SUPPORT
    // root["alexaVisible"] = 1;
    root["alexaEnabled"] = getSetting("alexaEnabled", ALEXA_ENABLED).toInt() == 1;
#endif
*/

    /*
#if TEMP_SENS_SUPPORT
    root["SnS1P"] =  getSetting("SnS1P", (SENS_SLOT1_PRE));
    root["SnTST"] = getSetting("SnTST", TEMPERATURE_SENSOR_TYPE);
#endif
*/

    /*
#if ANALOG_SUPPORT
    root["SnS2P"] =  getSetting("SnS2P", (SENS_SLOT2_PRE));
    root["SnAST"] = getSetting("SnAST", ANALOG_SENS_TYPE);
#endif
*/

    /*
#if I2C_SUPPORT
    root["SnS3P"] =  getSetting("SnS3P", (SENS_SLOT3_PRE));
    root["SnIST"] = getSetting("SnIST", I2C_SENS_TYPE);
#endif
*/

    // #if ENABLE_DS18B20
    //     root["dsVisible"] = 1;
    //     root["dsTmp"] = getDSTemperatureStr();
    // #endif

    // #ifdef DHT_SUPPORT
    //     root["dhtSlot"] = 1;
    //     root["have_dht"] = getSetting("have_dht", SENS_SLOT1_PRE);
    // // root["dhtTmp"] = getDHTTemperature();
    // // root["dhtHum"] = getDHTHumidity();
    // #endif
    // #ifdef PIR
    //     root["pirSlot"] = 1;
    //     root["have_pir"] = getSetting("have_pir", HAVE_PIR);
    // #endif
    // #ifdef ANALOG_SUPPORT
    //     root["anlgSlot"] = 1;
    //     // root["have_anlg"] = getSetting("have_anlg", SENS_SLOT2_PRE);
    // #endif
    // #ifdef DIGITAL_SENSOR
    //     root["digiSlot"] = 1;
    //     root["have_digi"] = getSetting("have_digi", HAVE_DIGI_SENSOR);
    // #endif

    // #if ENABLE_RF
    //     root["rfVisible"] = 1;
    //     root["rfChannel"] = getSetting("rfChannel", RF_CHANNEL);
    //     root["rfDevice"] = getSetting("rfDevice", RF_DEVICE);
    // #endif
    // #if ENABLE_EMON
    //     root["emonVisible"] = 1;
    //     root["emonPower"] = getPower();
    //     root["emonMains"] = getSetting("emonMains", EMON_MAINS_VOLTAGE);
    //     root["emonRatio"] = getSetting("emonRatio", EMON_CURRENT_RATIO);
    // #endif
    // #if ENABLE_ANALOG
    //     root["analogVisible"] = 1;
    //     root["analogValue"] = getAnalog();
    // #endif
    // #if ENABLE_POW
    //     root["powVisible"] = 1;
    //     root["powActivePower"] = getActivePower();
    //     root["powApparentPower"] = getApparentPower();
    //     root["powReactivePower"] = getReactivePower();
    //     root["powVoltage"] = getVoltage();
    //     root["powCurrent"] = getCurrent();
    //     root["powPowerFactor"] = getPowerFactor();
    // #endif
    // }
}
// -----------------------------------------------------------------------------
// WEBSERVER
// -----------------------------------------------------------------------------

AsyncWebServer *webServer() {
    return _server;
}
unsigned int webPort() {
#if ASYNC_TCP_SSL_ENABLED & WEB_SSL_ENABLED
    return 443;
#else
    return getSetting("webPort", WEB_PORT).toInt();
#endif
}
void webLogRequest(AsyncWebServerRequest *request) {
    // DEBUG_MSG_P(PSTR("[WEBSERVER] Request: %s %s\n"),
    // request->methodToString(), request->url().c_str());
}
/* 
bool _authenticate(AsyncWebServerRequest* request)
{
    String password = getSetting("adminPass", ADMIN_PASS);
    char httpPassword[password.length() + 1];
    password.toCharArray(httpPassword, password.length() + 1);
    return request->authenticate(HTTP_USERNAME, httpPassword);
} */

bool _authAPI(AsyncWebServerRequest *request, bool dontRespond) {
#if API_AUTH_DISABLED
    return true;
#endif

    if (getSetting("apiEnabled").toInt() == 0) {
        DEBUG_MSG_P(PSTR("[WEBSERVER] HTTP API is not enabled\n"));
        if (!dontRespond) {
            request->send(403);
        }
        return false;
    }
    String apiKeySaved = getSetting("apiKey", "");
    if (!apiKeySaved.length()) {
        return true;
    }
    if (
        !request->hasParam("apikey", (request->method() == HTTP_PUT)) || !request->hasParam("meta1", (request->method() == HTTP_PUT)) || !request->hasParam("meta", (request->method() == HTTP_PUT))) {
        DEBUG_MSG_P(PSTR("[WEBSERVER] Missing apikey or other security parameter\n"));
        if (!dontRespond) {
            // request->send(403);
            respondUnauthorized(request);
        }
        return false;
    }

    AsyncWebParameter *p = request->getParam("apikey", (request->method() == HTTP_PUT));
    String meta1 = request->getParam("meta1", (request->method() == HTTP_PUT))->value();
    String meta = request->getParam("meta", (request->method() == HTTP_PUT))->value();
    String apiKey = p->value();
    char msg_to_be_hashed[meta1.length() + apiKey.length() + 5];
    strcpy(msg_to_be_hashed, (meta1 + "," + apiKey).c_str());

    bool hashVerified = compareHash(msg_to_be_hashed, meta.c_str());

    if (!apiKey.equals(getSetting("apiKey", "")) || !hashVerified) {
        DEBUG_MSG_P(PSTR("[WEBSERVER] Wrong apikey parameter\n"));
        if (!dontRespond) {
            // request->send(403);
            respondUnauthorized(request);
        }
        return false;
    }
    // Do the hash comparision.
    // Here we'll check if the timestamp (in the var meta1 ) recieved in this
    // request is older than 1 second or not.
    // If its older than 1 sec reject this request.
    unsigned long temp = strtoul(meta1.c_str(), NULL, 10);
    // Serial.println(temp);
    if (temp == lastRequest || temp < lastRequest) {
        DEBUG_MSG_P(PSTR("[WEBSERVER] Unauthorised request Session expired\n"));
        if (!dontRespond) {
            // request->send(403);
            respondUnauthorized(request);
        }
        return false;
    }
    lastRequest = temp;
    return true;
}

bool _authAPI(AsyncWebServerRequest *request) {
    return _authAPI(request, false);
}

bool _asJson(AsyncWebServerRequest *request) {
    bool asJson = false;
    if (request->hasHeader("Accept")) {
        AsyncWebHeader *h = request->getHeader("Accept");
        asJson = h->value().equals("application/json");
    }

    return asJson;
}

ArRequestHandlerFunction _bindAPI(unsigned int apiID) {
    return [apiID](AsyncWebServerRequest *request) {
        webLogRequest(request);
        // Also block all the apis if api Keys does not exist.
        // since authApi returns true if there is no API Key. That is beginning
        if (!getSetting("apiKey").length()) {
            // request->send(403);
            respondUnauthorized(request);
            return;
        }
        if (!_authAPI(request))
            return;

        bool asJson = _asJson(request);

        web_api_t api = _apis[apiID];
        if (api.postFn != NULL) {
            if (request->hasParam("value", request->method() == HTTP_POST)) {
                bool is_dimmer = false;
                if (request->hasParam("is_dimmer", request->method() == HTTP_POST)) {
                    is_dimmer = true;
                }
                AsyncWebParameter *val = request->getParam("value", request->method() == HTTP_POST);
                AsyncWebParameter *key = request->getParam("key", request->method() == HTTP_POST);
                // char * keyname = key ? key->value()).c_str(): "";
                (api.postFn)((key ? key->value().c_str() : "0"), (val->value()).c_str(), is_dimmer);
            }
        }

        char value[150];
        (api.getFn)(value, 150);
        // jump over leading spaces
        char *p = value;
        while ((unsigned char)*p == ' ')
            ++p;
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", p);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    };
}

void apiRegister(
    const char *url, const char *key, apiGetCallbackFunction getFn, apiPostCallbackFunction postFn) {
    // Store it
    web_api_t api;
    char buffer[40];
    snprintf(buffer, 39, "/api/%s", url);
    api.url = strdup(buffer);
    api.key = strdup(key);
    api.getFn = getFn;
    api.postFn = postFn;
    _apis.push_back(api);

    // Bind call
    unsigned int methods = HTTP_GET;
    if (postFn != NULL)
        methods += HTTP_POST;

    _server->on(buffer, methods, _bindAPI(_apis.size() - 1));
}

void _onAPIs(AsyncWebServerRequest *request) {
    // webLogRequest(request);

    if (!getSetting("apiKey").length()) {
        // request->send(403);
        respondUnauthorized(request);
        return;
    }

    if (!_authAPI(request))
        return;

    String output;
    StaticJsonDocument<1024> root;
    for (unsigned int i = 0; i < _apis.size(); i++) {
        DEBUG_MSG_P(PSTR("[DEBUG] Api Key - %s \n"), _apis[i].key);
        root[_apis[i].key] = _apis[i].url;
    }
    serializeJson(root, output);
    DEBUG_MSG_P(PSTR("[DEBUG] Output : %s \n"), output.c_str());

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void _processAction(String action, JsonObject meta) {
    if (action.equals("reset")) {
        // TODO_S2
        deferredReset(1000, CUSTOM_RESET_RPC);
    }
    else if (action.equals("reconnect")) {
        // Let the HTTP request return and disconnect after 100ms
        // TODO_S2
        deferred.once_ms(500, wifiDisconnect);
    }
#if NOFUSS_SUPPORT
    else if (action.equals("checkForUpdate")) {
        // TODO_S1
        // checkUpdateFlag = true; TODO from nofuss
    }
#endif
    else if (action.equals("delDevice")) {
        // If a user is deleting device from his phone, we need to delete few things from the
        // device as well so as to disconnect from the network and not report to server
        if (meta.containsKey("apiKey")) {
            String apiKey = meta["apiKey"];
            if (getSetting("apiKey").equals(apiKey)) {
                _deleteDevice();
            }
        }
    } else if (action.equals("forceDel")){
        // Do not check for meta here, Just delete. 
        _deleteDevice();
    }else if (action.equals("factoryset")) {
        settingsFactoryReset();
    } else if (action.equals("disconnect")) {
        deferred.once_ms(300, []() {
            // _wifiConfigure(); TODO_S1
            // wifiDisconnect();
        });
    } else if (action.equals("hotspot")) {
        deferred.once_ms(300, []() {
            // createLongAP(false); TODO_S1
        });
    } else if (action.equals("sb")) {
        setReportSettings(); // TODO_S2
        resetMqttBeatFlags(); // Required
        setHeartBeatSchedule(millis());
    } 
    else if (action.equals("snsrst")) {
        // Reset Sensors Configuration -
        // Delte all sensor related settings
        // call sensor after configure function.
        // report Sensor Settings
    } else if(action.equals("wifihr")) {
         // WIFI Hard Reset
        // setWifiHardReset(true); TODO_S1
    }
#if SENSOR_SUPPORT
    else if (action.equals("report_sns")) {
        setReportSensors(true);
    } 
#endif
#if 0//SAVE_CRASH_DUMP //TODO_S1
    else if (action.equals("crashd")) {
        sendCrashDump(true);
    } 
    else if (action.equals("crash")) {
        // deferredReset(1000, CUSTOM_RESET_MQTT);
        DEBUG_MSG_P(PSTR("Will Crash Now"));
        delay(2000);
    }
#endif
    else if (action.equals("custom_action")) {
        int mode_val = 0;
        int mode_type = 0;
        int mode_DpId = 0;
        if (meta.containsKey("m_val")) {
            mode_val = meta["m_val"];
        }
        if (meta.containsKey("m_type")) {
            mode_type = meta["m_type"];
        }
        if (meta.containsKey("m_CmdId")) {
            mode_DpId = meta["m_CmdId"];
        }
        // KUSetCustomSettingDP(mode_DpId, mode_type, mode_val); TODO_S2
    } else if (action.equals("custom_config")) {
        setFetchCustomConfig(true);
    }
    else {
        DEBUG_MSG_P(PSTR("[DEBUG] Unknown action in RPC"));
    }
}

void _processAction(String action) {
    StaticJsonDocument<50> root;
    JsonObject blank = root.to<JsonObject>();
    _processAction(action, blank);
}

/*

void _onAuth(AsyncWebServerRequest* request)
{
    webLogRequest(request);
    if (!_authenticate(request))
        return request->requestAuthentication();

    IPAddress ip = request->client()->remoteIP();
    unsigned long now = millis();
    unsigned short index;
    for (index = 0; index < WS_BUFFER_SIZE; index++)
    {
        if (_ticket[index].ip == ip)
            break;
        if (_ticket[index].timestamp == 0)
            break;
        if (now - _ticket[index].timestamp > WS_TIMEOUT)
            break;
    }
    if (index == WS_BUFFER_SIZE)
    {
        request->send(429);
    }
    else
    {
        _ticket[index].ip = ip;
        _ticket[index].timestamp = now;
        request->send(204);
    }
}
*/

void _onGetConfig(AsyncWebServerRequest *request) {
    if (!getSetting("apiKey").length()) {
        // request->send(403);
        respondUnauthorized(request);
        return;
    }
    if (!_authAPI(request))
        return;

    // webLogRequest(request);

    // AsyncJsonResponse* response = new AsyncJsonResponse();
    DEBUG_MSG_P(PSTR("Free Heap 1 : %d"), getFreeHeap());
    DynamicJsonDocument root(512);
    root["app"] = APP_NAME;
    root["version"] = APP_VERSION;
    root["rev"] = APP_REVISION;
    root["settings_version"] = SETTINGS_VERSION;
    root["sensors_version"] = SENSORS_VERSION;
    root["device"] = DEVICE;

    getSupportedConfigWeb(root);
    // DEBUG_MSG_P(PSTR("Free Heap 2 : %d"), getFreeHeap());

    String output;
    serializeJson(root, output);

    // DEBUG_MSG_P(PSTR("Free Heap 3 : %d"), getFreeHeap());

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

// Right nowe not using this function anywhere Sice it actually dumps all settings including security keys

void _onGetConfigAll(AsyncWebServerRequest *request) {
    webLogRequest(request);
    // Disabling Login for sometime.
    // if (!_authenticate(request)) return request->requestAuthentication();

    if (!getSetting("apiKey").length()) {
        // request->send(403);
        respondUnauthorized(request);
        return;
    }

    if (!_authAPI(request))
        return;

    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();

    root["app"] = APP_NAME;
    root["version"] = APP_VERSION;
    root["rev"] = APP_REVISION;
    settingsGetJson(root);
    response->setLength();
    request->send(response);
}
#if SENSOR_SUPPORT
void _onGetSensorConfig(AsyncWebServerRequest *request) {
    webLogRequest(request);
    // AsyncJsonResponse* response = new AsyncJsonResponse();
    DynamicJsonBuffer jsonBuffer(500);
    JsonObject &root = jsonBuffer.createObject();
    char topic[MAX_CONFIG_TOPIC_SIZE] = "";
    _sensors_config_keys(jsonBuffer, root, topic, false);
    String output;
    root.printTo(output);
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}
#endif

bool configStore(String key, String value) {
    // HTTP port
    if (key == "webPort") {
        if ((value.toInt() == 0) || (value.toInt() == 80)) {
            return delSetting(key);
        }
    }

    if (value != getSetting(key)) {
        return setSetting(key, value);
    }

    return false;
}

bool configStore(String key, JsonArray value) {
    bool changed = false;

    unsigned char index = 0;
    for (JsonVariant element : value) {
        if (configStore(key + String(index), element.as<String>()))
            changed = true;
        index++;
    }

    // Delete further values
    for (unsigned char i = index; i < SETTINGS_MAX_LIST_COUNT; i++) {
        if (!delSetting(key, i))
            break;
        changed = true;
    }

    return changed;
}

bool configStoreWifiOld(JsonArray wifiArray) {
    unsigned char index = 0;
    bool changed = false;
    // DEBUG_MSG_P(PSTR("[DEBUG] Reached Here \n"));
    for (JsonVariant element : wifiArray) {
        JsonObject wifiObj = element.as<JsonObject>();

        String ssid = wifiObj["ssid"].as<String>();
        String pass = wifiObj["pass"].as<String>();
        if (configStore(String("ssid") + index, ssid))
            changed = true;
        if (!pass.equals("null")) {
            DEBUG_MSG_P(PSTR("[DEBUG] pass - for the network received is - %s \n "), pass.c_str());
            if (configStore(String("pass") + index, pass))
                changed = true;
        }
        index++;
    }

    // Delete further values
    for (unsigned char i = index; i < WIFI_MAX_NETWORKS; i++) {
        if (!delSetting("ssid", i))
            break;
        DEBUG_MSG_P(PSTR("[DEBUG] Deleting key : pass%c \n"), i);
        delSetting("pass", i);
        changed = true;
    }
    return changed;
}

bool configStoreWifi(JsonArray wifiArray) {
    unsigned char index = 0;
    bool changed = false;
    // DEBUG_MSG_P(PSTR("[DEBUG] Reached Here \n"));

    std::map<std::string, std::string> _wifis;
    for (uint8_t j = 0; j < WIFI_MAX_NETWORKS; j++) {
        String ssid = getSetting("ssid", j, NULL);
        if (ssid.isEmpty()) {
            break;
        }
        _wifis[ssid.c_str()] = getSetting("pass", j, NULL).c_str();
    }

    /* for(std::map<std::string,std::string>::iterator iter = _wifis.begin(); iter != _wifis.end(); ++iter)
    {
      std::string key =  iter->first;
      std::string value = iter->second;

      DEBUG_MSG_P(PSTR("[DEBUG] Added in Map - %s , %s \n"), &key[0], &value[0]);
      
    }
    DEBUG_MSG_P(PSTR("[DEBUG] Found the key temp: %d , value: %s\n"), _wifis.find("temp")!=_wifis.end(), &_wifis["temp"][0]);
     */
    for (JsonVariant element : wifiArray) {
        JsonObject wifiObj = element.as<JsonObject>();
        String ssid = wifiObj["ssid"].as<String>();
        String pass = wifiObj["pass"].as<String>();

        // Decrypt pass -
        char buffer[pass.length()];
        memset(buffer, 0, sizeof(buffer));

        decodeWifiPass(pass.c_str(), buffer, pass.length());
        pass = String((char *)buffer);
        // DEBUG_MSG_P(PSTR("[DEBUG] Wifi Pass : %s "), pass.c_str());

        // DEBUG_MSG_P(PSTR("[DEBUG] ssid:  %s , pass: %s \n"),ssid.c_str(), pass.c_str());
        if (_wifis.find(ssid.c_str()) != _wifis.end()) {
            if (configStore(String("ssid") + index, ssid))
                changed = true;
            if (!pass.equals("null")) {
                if (configStore(String("pass") + index, pass))
                    changed = true;
            } else {
                // Keep the old password
                configStore(String("pass") + index, String(&_wifis[ssid.c_str()][0]));
            }
        } else {
            if (configStore(String("ssid") + index, ssid))
                changed = true;
            if (configStore(String("pass") + index, pass))
                changed = true;
        }

        /* for( uint8_t j = 0; j<WIFI_MAX_NETWORKS ;j++){
            String ssid_local = getSetting("ssid", j, null);
            if(ssid_local.equal(ssid)){
                if(!pass.equal("null")){
                   if (configStore(String("pass") + index, pass)) changed = true; 
                }

            }
        } */

        index++;
    }

    // Delete further values
    for (unsigned char i = index; i < WIFI_MAX_NETWORKS; i++) {
        if (!delSetting("ssid", i))
            break;
        DEBUG_MSG_P(PSTR("[DEBUG] Deleting key : pass%c \n"), i);
        delSetting("pass", i);
        changed = true;
    }
    return changed;
}

void _onPostConfig(AsyncWebServerRequest *request) {
    webLogRequest(request);
    // Here we'll not check for the existense of apiKey - and allowing usrs to set config if apiKey does not exists.
    if (!_authAPI(request))
        return;

    short int params = request->params();
    if (params > 0 && request->getParam("body", true, false)) {
        // Serial.printf("_POST[BODY]: %s\n", request->getParam("body", true, false)->value().c_str());
        DynamicJsonDocument root(1024);

        DeserializationError error = deserializeJson(
            root,
            (char *)request->getParam("body", true, false)->value().c_str());
        if (error) {
            DEBUG_MSG_P(PSTR("[DEBUG] Error parsing data\n"));
            responseBadRequest(request, "Invalid Body");
            // request->send(400, "application/json", "{\"message\":\"Invalid Body\"}");
            return;
        } else {
            if ((root.containsKey("config")) || root.containsKey("action")) {
                String message = "";
                serializeJson(root, message);
                DEBUG_MSG_P(PSTR("[DEBUG] Root Message : %s\n"), message.c_str());
                if (root.containsKey("config")) {
                    JsonObject config = root["config"];
                    if (!config) {
                        responseBadRequest(request, "Invalid Body");
                    } else {
                        char responseBuffer[512] = "";
                        processConfig(config, "web", responseBuffer, 512);
                        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", responseBuffer);
                        response->addHeader("Access-Control-Allow-Origin", "*");
                        request->send(response);
                        DEBUG_MSG_P(PSTR("Came Untill Here"));
                    }

                    // DEBUG_MSG_P(PSTR("[DEBUG] Ready To Send Response now"));

                }
                // Check actions
                else if (root.containsKey("action")) {
                    String action = root["action"];
                    /* DEBUG_MSG_P(PSTR("[WEBSOCKET] Requested action: %s\n"), action.c_str());

                    if (action.equals("reset")) {
                        deferredReset(300, CUSTOM_RESET_WEB);
                    }

                    else if (action.equals("reconnect")) {
                        // Let the HTTP request return and disconnect after 100ms
                        deferred.once_ms(300, wifiDisconnect);
                    } */
                    if (root.containsKey("meta")) {
                        JsonObject meta = root["meta"];
                        _processAction(action, meta);
                    } else {
                        _processAction(action);
                    }

                    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Success");
                    response->addHeader("Access-Control-Allow-Origin", "*");
                    request->send(response);
                }
            } else {
                responseBadRequest(request, "Invalid Body");
            }
            // /. if not config
        }
    } else {
        responseBadRequest(request, "Invalid Body");
    }
}
void _onGetInfo(AsyncWebServerRequest *request) {
    if (!getSetting("apiKey").length()) {
        // request->send(403);
        respondUnauthorized(request);
        return;
    }
    if (!_authAPI(request))
        return;

    DynamicJsonDocument duc(512);
    JsonObject root = duc.to<JsonObject>();
    getInfo(root);
    String output;
    serializeJson(root, output);
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

// This is unprotected api. Basically Open for everyone.
void _onPing(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->addHeader("Access-Control-Allow-Origin", "*");
    DynamicJsonDocument root(256);
    root["app"] = APP_NAME;
    root["version"] = APP_VERSION;
    root["rev"] = APP_REVISION;
    root["hostname"] = getSetting("hostname", HOSTNAME);
    root["device"] = DEVICE;
    root["deviceId"] = getSetting("identifier");
    root["cv"] = SETTINGS_VERSION;
    root["ssv"] = SENSORS_VERSION;

    serializeJson(root, *response);
    request->send(response);
}

#if EMBEDDED_WEB // TODO_S2
void _onHome(AsyncWebServerRequest *request) {
    webLogRequest(request);

    if (request->header("If-Modified-Since").equals(_last_modified)) {
        request->send(304);
    } else {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html_gz, index_html_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Last-Modified", _last_modified);
        request->send(response);
    }
}
#endif

#if ASYNC_TCP_SSL_ENABLED & WEB_SSL_ENABLED

int _onCertificate(void *arg, const char *filename, uint8_t **buf) {
    DEBUG_MSG_P(PSTR("[DEBUG] Coming Inside On Certificate \n"));

#if EMBEDDED_KEYS
    DEBUG_MSG_P(PSTR("[DEBUG] Coming Inside embedded Keys \n"));
    if (strcmp(filename, "server.cer") == 0) {
        uint8_t *nbuf = (uint8_t *)malloc(server_cer_len);
        memcpy_P(nbuf, server_cer, server_cer_len);
        *buf = nbuf;
        DEBUG_MSG_P(PSTR("[WEB] SSL File: %s - OK\n"), filename);
        return server_cer_len;
    }

    if (strcmp(filename, "server.key") == 0) {
        uint8_t *nbuf = (uint8_t *)malloc(server_key_len);
        memcpy_P(nbuf, server_key, server_key_len);
        *buf = nbuf;
        DEBUG_MSG_P(PSTR("[WEB] SSL File: %s - OK\n"), filename);
        return server_key_len;
    }

    DEBUG_MSG_P(PSTR("[WEB] SSL File: %s - ERROR\n"), filename);
    *buf = 0;
    return 0;

#else

    File file = SPIFFS.open(filename, "r");
    if (file) {
        size_t size = file.size();
        uint8_t *nbuf = (uint8_t *)malloc(size);
        if (nbuf) {
            size = file.read(nbuf, size);
            file.close();
            *buf = nbuf;
            DEBUG_MSG_P(PSTR("[WEB] SSL File: %s - OK\n"), filename);
            return size;
        }
        file.close();
    }
    DEBUG_MSG_P(PSTR("[WEB] SSL File: %s - ERROR\n"), filename);
    *buf = 0;
    return 0;

#endif
}

#endif

void _onUpgrade(AsyncWebServerRequest *request) {
    if (!getSetting("apiKey").length()) {
        // request->send(403);
        respondUnauthorized(request);
        return;
    }

    if (!_authAPI(request))
        return;

    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", Update.hasError() ? "FAIL" : "Success");
    response->addHeader("Connection", "close");
    if (!Update.hasError()) {
        eepromRotate(true); // TODO_S2
        deferredReset(1000, CUSTOM_RESET_UPGRADE); // TODO_S2
    }
    request->send(response);
}

void _onUpgradeData(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data,
                    size_t len, bool final) {
    if (!getSetting("apiKey").length()) {
        return;
    }

    if (!_authAPI(request, true))
        return;

    if (!index) {
        // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
        eepromRotate(false); // TODO_S2

        DEBUG_MSG_P(PSTR("[UPGRADE] Start: %s\n"), filename.c_str());
        eepromBackup(0); //TODO_S2
        Update.runAsync(true);
        if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
            Update.printError(Serial);
        }
    }
    if (!Update.hasError()) {
        if (Update.write(data, len) != len) {
            Update.printError(Serial);
        }
    }
    if (final) {
        if (Update.end(true)) {
            DEBUG_MSG_P(PSTR("[UPGRADE] Success:  %u bytes\n"), index + len);
        } else {
            Update.printError(Serial);
        }
    } else {
        DEBUG_MSG_P(PSTR("[UPGRADE] Progress: %u bytes\r"), index + len);
    }
}
void apiSetup() {
// Displaying list of all availaible apis
#if DEVELOPEMEMT_MODE
    _server->on("/apis", HTTP_GET, _onAPIs);
#endif
}

void respondUnauthorized(AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse(403, "text/plain", "UNAUTHORIZED");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    return;
}

void responseBadRequest(AsyncWebServerRequest * request, const char * message){
    char buffer[64] = "";
    snprintf(buffer, 63, "{\"message\":\"%s\"}", message);
    AsyncWebServerResponse *response = request->beginResponse(404, "application/json", buffer);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    return;
}

void webSetup() {
    // Create server
    // Cache the Last-Modifier header value
    // Create server
    unsigned int port = webPort();
    _server = new AsyncWebServer(port);

    // Cache the Last-Modifier header value
    sprintf(_last_modified, "%s %s GMT", __DATE__, __TIME__);

    // Setup webserver
    // _server->addHandler(&ws);

    apiSetup();
    // Rewrites
    // _server->rewrite("/", "/index.html");
    // Serve home (basic authentication protection)
#if EMBEDDED_WEB
    // _server->on("/index.html", HTTP_GET, _onHome);
#endif

    _server->on("/upgrade", HTTP_POST, _onUpgrade, _onUpgradeData);
    _server->on("/config", HTTP_GET, _onGetConfig);
    _server->on("/config", HTTP_POST, _onPostConfig);
    _server->on("/info", HTTP_GET, _onGetInfo);
    _server->on("/scan_networks", HTTP_GET, _onWifiScanServer);
    // _server->on("/rpc", HTTP_GET, _onRPC);
    _server->on("/ping", HTTP_GET, _onPing);
    // _server->on("/auth", HTTP_GET, _onAuth);
#if IR_SUPPORT
    // This is custom API, I am trying to emulate api for IR actions as well.
    _server->on("/custom/ir", HTTP_POST, _onIRApi);
#endif
    // 404
    _server->onNotFound([](AsyncWebServerRequest *request) {
        request->send(404);
    });

// Run server
#if ASYNC_TCP_SSL_ENABLED & WEB_SSL_ENABLED
    _server->onSslFileRequest(_onCertificate, NULL);
    _server->beginSecure("server.cer", "server.key", NULL);
#else
    _server->begin();
#endif

    DEBUG_MSG_P(PSTR("[WEBSERVER] Webserver running on port %d\n"),
                port);

    // Enable API.
    setSetting("apiEnabled", API_ENABLED);
}

/*
// Not using this any more - merged in config api Itself. 

void _onRPC(AsyncWebServerRequest *request)
{
    webLogRequest(request);

    if (!_authAPI(request))
        return;

    // bool asJson = _asJson(request);
    int responseCode = 404;

    if (request->hasParam("action"))
    {
        AsyncWebParameter *p = request->getParam("action");
        String action = p->value();
        DEBUG_MSG_P(PSTR("[RPC] Action: %s\n"), action.c_str());
        responseCode = 200;
        _processAction(action);
    }

    if (responseCode == 200)
    {
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "success");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    }
    else
    {
        AsyncWebServerResponse *response = request->beginResponse(responseCode, "application/json", "error");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    }
}

*/

// TODO_S2 following functions taken from wifi.ino think where to put all these functions.

void _onWifiScanServer(AsyncWebServerRequest* request) {
    DEBUG_MSG_P(PSTR("[WEB] Request for scan wifi networks"));
    // No need for authenticating this api.
    // if (!_authAPI(request)) return;

    String wifiBuffer;
    DynamicJsonDocument root(1024);
    _wifiScan(root);
    serializeJson(root, wifiBuffer);

    AsyncWebServerResponse* response =
        request->beginResponse(200, "application/json", wifiBuffer);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);

    // Serial.println(wifiBuffer);
    // DEBUG_MSG_P(PSTR("+OK\n"));
}

void _wifiScan(DynamicJsonDocument& root) {
    DEBUG_MSG_P(PSTR("[WIFI] Start scanning\n"));

    /* #if WEB_SUPPORT
        String output;
    #endif */

    int result = WiFi.scanComplete();
    JsonArray networks = root.createNestedArray("networks");
    root["error"] = NULL;

    if (result == -2) {
        DEBUG_MSG_P(PSTR("[WIFI] Scan failed\n"));
        root["error"] = F("Scan failed");
        WiFi.scanNetworks(true);
    } else if (result == 0) {
        DEBUG_MSG_P(PSTR("[WIFI] No networks found\n"));
        root["error"] = F("No networks found");
        WiFi.scanNetworks(true);
    } else if (result >= 0xFA) {
        DEBUG_MSG_P(PSTR("[WIFI] Scan failed\n"));
        root["error"] = F("Please Re Scan");
        WiFi.scanNetworks(true);
    } else {
        DEBUG_MSG_P(PSTR("[WIFI] %d networks found:\n"), result);

        // Populate defined networks with scan data
        for (int8_t i = 0; i < result && i < WIFI_MAX_NETOWRK_DISCOVERY; ++i) {
            String ssid_scan;
            int32_t rssi_scan;
            uint8_t sec_scan;
            uint8_t* BSSID_scan;
            int32_t chan_scan;
            // char buffer[128];
            char bssid[22];
            bool hidden = false;

            WiFi.getNetworkInfo(i, ssid_scan, sec_scan, rssi_scan, BSSID_scan,
                                chan_scan, hidden);

            snprintf_P(bssid, sizeof(bssid),
                       PSTR("%02X:%02X:%02X:%02X:%02X:%02X"), BSSID_scan[0],
                       BSSID_scan[1], BSSID_scan[2], BSSID_scan[3],
                       BSSID_scan[4], BSSID_scan[5]);
            /*
            snprintf_P(buffer, sizeof(buffer),
                PSTR("BSSID: %02X:%02X:%02X:%02X:%02X:%02X SEC: %s RSSI: %3d CH:
            %2d SSID: %s"), BSSID_scan[1], BSSID_scan[2], BSSID_scan[3],
            BSSID_scan[4], BSSID_scan[5], BSSID_scan[6], (sec_scan !=
            ENC_TYPE_NONE ? "YES" : "NO "), rssi_scan, chan_scan, (char *)
            ssid_scan.c_str()
            );
            */
            StaticJsonDocument<256> temp;
            JsonObject network = temp.to<JsonObject>();
            network["bssid"] = bssid;
            network["rssi"] = rssi_scan;
            network["ssid"] = ssid_scan;
            // network["sec"] = sec_scan != ENC_TYPE_NONE ? 1 : 0;
            networks.add(network);

            // #if WEB_SUPPORT
            //     if (client_id > 0) output = output + String(buffer) +
            //     String("<br />");
            // #endif
        }

        WiFi.scanDelete();
        // if(WiFi.scanComplete() == -2){
        WiFi.scanNetworks(true);
        // }
    }

    // if (result == WIFI_SCAN_FAILED) {
    //     DEBUG_MSG_P(PSTR("[WIFI] Scan failed\n"));
    //     root["error"] = F("Scan failed");
    // } else if (result == 0) {
    //     DEBUG_MSG_P(PSTR("[WIFI] No networks found\n"));
    //     root["error"] = F("No networks found");
    // } else {

    // }

    // #if WEB_SUPPORT
    //     if (client_id > 0) {
    //         output = String("{\"scanResult\": \"") + output + String("\"}");
    //         wsSend(client_id, output.c_str());
    //     }
    // #endif

    // WiFi.scanDelete();
}

void _wifiScanFix() {
    int result = WiFi.scanComplete();
    if (result == -2 || result == 0) {
        DEBUG_MSG_P(PSTR("[WIFI] AP Scan failed, scan result : %d \n"), result);
        // WiFi.scanDelete();
        WiFi.scanNetworks(true);
    }

    // else{

    //     WiFi.scanDelete();
    //     if(WiFi.scanComplete() == -2){
    //         WiFi.scanNetworks();
    //     }
    // }
}

