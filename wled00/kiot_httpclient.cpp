#if HTTP_CLIENT_SUPPORT
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
unsigned long last_called = 0;
#if HTTP_CONF_SUPPORT
bool fetchSecurityConf = false;
bool fetchMqttConf = false;
// TEMPORARY
bool fetchRPCaction = false;
bool fetchCustomConfig = false;
#endif


String _exractParam(String& authReq, const String& param, const char delimit) {
    int _begin = authReq.indexOf(param);
    if (_begin == -1) {
        return "";
    }
    return authReq.substring(_begin + param.length(), authReq.indexOf(delimit, _begin + param.length()));
}

String _getDigestAuth(String& authReq, const String& username, const String& password, const String& uri, unsigned int counter) {
    // extracting required parameters for RFC 2069 simpler Digest
    String realm = _exractParam(authReq, "realm=\"", '"');
    String nonce = _exractParam(authReq, "nonce=\"", '"');
    String cNonce = _getCNonce(8);

    char nc[9];
    snprintf(nc, sizeof(nc), "%08x", counter);

    // parameters for the RFC 2617 newer Digest
    MD5Builder md5;
    md5.begin();
    md5.add(username + ":" + realm + ":" + password);  // md5 of the user:realm:user
    md5.calculate();
    String h1 = md5.toString();

    md5.begin();
    md5.add(String("GET:") + uri);
    md5.calculate();
    String h2 = md5.toString();

    md5.begin();
    md5.add(h1 + ":" + nonce + ":" + String(nc) + ":" + cNonce + ":" + "auth" + ":" + h2);
    md5.calculate();
    String response = md5.toString();

    String authorization = "Digest username=\"" + username + "\", realm=\"" + realm + "\", nonce=\"" + nonce +
                           "\", uri=\"" + uri + "\", algorithm=\"MD5\", qop=auth, nc=" + String(nc) + ", cnonce=\"" + cNonce + "\", response=\"" + response + "\"";
    // Serial.println(authorization);

    return authorization;
}

String _httpGetRaw(String url, String query) {
    HTTPClient http;
    // DEBUG_MSG_P(PSTR("[HTTP_CLIENT] Url : %s \n"), (url + query).c_str());
    http.begin(getSetting("httpServer", HTTPSERVER_HOST) + url + query);
    int httpCode = http.GET();
    if (httpCode > 0) {
        // DEBUG_MSG_P(PSTR("[HTTP_CLIENT] GET ... code : %d\n"), httpCode);

        if (httpCode == HTTP_CODE_OK) {
            // strncpy(buffer, http.getString().c_str(), max_len);
            return http.getString();
        } else {
            // DEBUG_MSG_P(PSTR("[HTTP_CLIENT] Unknown Error \n"));
            return "";
        }
    } else {
        // DEBUG_MSG_P(PSTR("[HTTP] GET... failed, error: %s\n"), http.errorToString(httpCode).c_str());
        sendGratuitousARP();
    }
    return "";
}

String _getCNonce(const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    String s = "";

    for (int i = 0; i < len; ++i) {
        s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return s;
}

String _httpGetRaw(String url, String query, bool auth) {
    if (!auth) {
        return _httpGetRaw(url, query);
    }

    HTTPClient http;
    DEBUG_MSG_P(PSTR("[HTTP_CLIENT] Url : %s \n"), (url + query).c_str());
    http.begin(getSetting("httpServer", HTTPSERVER_HOST) + url + query);
    const char* keys[] = {"WWW-Authenticate"};
    http.collectHeaders(keys, 1);

    int httpCode = http.GET();

    if (httpCode > 0) {
        // DEBUG_MSG_P(PSTR("[HTTP_CLIENT] GET ... code : %d\n"), httpCode);
        String authReq = http.header("WWW-Authenticate");
        // DEBUG_MSG_P(PSTR("[DEBUG] AuthReq: %s \n"), authReq.c_str());
        String authorization = _getDigestAuth(authReq, getSetting("identifier"), getSetting("httpPass"), url + query, 1);

        http.end();
        http.begin(getSetting("httpServer", HTTPSERVER_HOST) + url + query);
        http.addHeader("Authorization", authorization);
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            return http.getString();

        } else {
            DEBUG_MSG_P(PSTR("[HTTP_CLIENT] Unknown Error. STATUS CODE : %d \n"), httpCode);
            return "";
        }

    } else {
        DEBUG_MSG_P(PSTR("[HTTP] GET... failed, error: %s\n"), http.errorToString(httpCode).c_str());
        return "";
    }
    return "";
}

String _httpPostRaw(String url, char* body, size_t size) {
    HTTPClient http;
    DEBUG_MSG_P(PSTR("[HTTP_CLIENT] Url : %s \n"), (url).c_str());
    http.begin(getSetting("httpServer", HTTPSERVER_HOST) + url);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST((uint8_t*)body, size);
    DEBUG_MSG_P(PSTR("[DEBUG] BODY : %s"), body);
    if (httpCode > 0) {
        DEBUG_MSG_P(PSTR("[HTTP_CLIENT] POST ... code : %d\n"), httpCode);

        if (httpCode == HTTP_CODE_OK) {
            // strncpy(buffer, http.getString().c_str(), max_len);
            return http.getString();
        } else {
            DEBUG_MSG_P(PSTR("[HTTP_CLIENT] Unknown Error \n"));
            return "";
        }
    } else {
        DEBUG_MSG_P(PSTR("[HTTP] GET... failed, error: %s\n"), http.errorToString(httpCode).c_str());
        sendGratuitousARP();
    }
    return "";
}

#if HTTP_CONF_SUPPORT
bool getConfigFromServer(const char* action) {
    unsigned long static last_try = 0;
    if (!wifiConnected()) {
        return false;
    }
    if (millis() - last_try < HTTP_CONF_RETRY_GAP) {
        return false;
    }
    DEBUG_MSG_P(PSTR("[DEBUG] Will Fetch Conf from server"));
    // Serial.println(getFreeHeap());
    // Serial.println(millis());
    if (strcmp(action, "mqttConf") == 0) {
        char queryBuffer[100] = "";
        snprintf_P(queryBuffer, sizeof(queryBuffer) - 1, "?query=%s&cv=%d", action, SETTINGS_VERSION);
        String payload = _httpGetRaw(getSetting("configURL", SERVER_CONFIG_URL), String(queryBuffer), true);
        // Serial.println(getFreeHeap());
        // Serial.println(millis());
        /* 
        // If Get config is a post call. 

        char bodyBuffer[150] = "";
        StaticJsonBuffer<150> jsonBuffer1;
        JsonObject& root1 = jsonBuffer1.createObject();
        root1["query"] = "securityConf";
        root1.printTo(bodyBuffer, sizeof(bodyBuffer));
        size_t size = strlen(bodyBuffer);

        String payload = _httpPostRaw(getSetting("configURL", SERVER_CONFIG_URL), bodyBuffer, size); 
        */

        last_try = millis();

        if (payload.length() < 1) {
            return false;
        }
        DEBUG_MSG_P(PSTR("[HTTP_CLIENT] Payload: %s\n"), payload.c_str());
        DynamicJsonBuffer jsonBuffer(400);
        JsonObject& root = jsonBuffer.parseObject(payload);
        char resp[200] = "";
        decryptMsg(root["msg"], resp, strlen(root["msg"]), root["iv"]);
        DEBUG_MSG_P(PSTR("[HTTP_CLIENT] Decrypted Payload: %s\n"), resp);
        DynamicJsonBuffer jsonBuffer2(400);
        JsonObject& root2 = jsonBuffer2.parseObject(resp);
        if (!root2.success()) {
            DEBUG_MSG_P(PSTR("[HTTP_CLIENT] Parsing Server Response failed, Invalid JSON \n"));
            return false;
        }
        char buffer[100] = "";
        processConfig(root2, "hclient", buffer, 100);
        // This delay is required - other wise mqtt will connect right now itself without first disconnecting
        // due to this code in processConfig -
        //  //////  if (changedMQTT)
        //     ////////// deferred.once_ms(100, mqttReset);
        delay(200);
        return true;
    } else if (strcmp(action, "customConfig") == 0) {
        char queryBuffer[100] = "";
        snprintf_P(queryBuffer, sizeof(queryBuffer) - 1, "/%s", action);
        String finalURL = getSetting("httpServer", HTTPSERVER_HOST) + getSetting("configURL", SERVER_CONFIG_URL);
        DEBUG_MSG_P(PSTR("\n[GET_URL] %s \n"),(finalURL).c_str());
        String payload = _httpGetRaw(getSetting("configURL", SERVER_CONFIG_URL), String(queryBuffer), true);
        last_try = millis();
        if (payload.length() < 1) {
            return false;
        }
        DEBUG_MSG_P(PSTR("[HTTP_CLIENT] Custom Config Payload: %s\n"), payload.c_str());
        DynamicJsonBuffer jsonBuffer(400);
        JsonObject& root = jsonBuffer.parseObject(payload);
        if (!root.success()) {
            DEBUG_MSG_P(PSTR("[HTTP_CLIENT] Parsing Server Response failed, Invalid JSON \n"));
            return false;
        }
        storeCustomConfig(root);
        delay(200);
        return true;
    } else {
        return false;
    }
}

bool askRPCFromServer(){
        static unsigned long last_try = 0;
        if(!wifiConnected()){
            return false;
        }
        if(millis() - last_try < (int)HTTP_FETCH_RPC_RETRY_GAP){
            return false;
        }
        DEBUG_MSG_P(PSTR("Will Fetch RPC Action from server "));
        // TODO: Right not we are not sending any query param, later lets send something in query param for exactly why are we asking rpc From Server. 
        String payload = _httpGetRaw(getSetting("rpcUrl", SERVER_RPC_URL), EMPTY_STRING, true);
        last_try = millis();
        if (payload.length() < 1) {
            return false;
        }
        DEBUG_MSG_P(PSTR("[HTTP_CLIENT] Payload: %s\n"), payload.c_str());
        DynamicJsonBuffer jsonBuffer(400);
        JsonObject& root = jsonBuffer.parseObject(payload);
        char resp[200] = "";
        decryptMsg(root["msg"], resp, strlen(root["msg"]), root["iv"]);
        DEBUG_MSG_P(PSTR("[HTTP_CLIENT] Decrypted Payload: %s\n"), resp);
        DynamicJsonBuffer jsonBuffer2(200);
        JsonObject& root2 = jsonBuffer2.parseObject(resp);
        if (!root2.success()) {
            DEBUG_MSG_P(PSTR("[HTTP_CLIENT] Parsing Server Response failed, Invalid JSON \n"));
            return true;
        }
        const char * action = root2["action"];
        if(action){
            if(root.containsKey("meta")){
                JsonObject& meta = root["meta"];
                _processAction(action, meta);
            }else{
                _processAction(action);
            }
        }else{
            DEBUG_MSG_P(PSTR("[DBEUG] NO RPC ACTION SUPPLIED \n"));
        }
        return true;
}

void _checkConfRequirement() {
    if (fetchSecurityConf) {
        if (getConfigFromServer("securityConf")) {
            fetchSecurityConf = false;
        }
    }

    if (fetchMqttConf) {
        if (getConfigFromServer("mqttConf")) {
            fetchMqttConf = false;
        }
    }
    // TEMPORARY
    if(fetchRPCaction){
        if(askRPCFromServer()){
            fetchRPCaction = false;
        }
    }

    if(fetchCustomConfig) {
        if (getConfigFromServer("customConfig")) {
            setFetchCustomConfig(false);
        }
    }
}

void setFetchMqttConf(bool val) {
    fetchMqttConf = val;
}
void setFetchRPCAction(bool val){
    fetchRPCaction = val;
}
void setFetchCustomConfig(bool val){
    fetchCustomConfig = val;
}
#endif

void httpClientSetup() {
    loopRegister(httpClientLoop);
}

void httpClientLoop() {
#if HTTP_CONF_SUPPORT
    _checkConfRequirement();
#endif
}

#endif