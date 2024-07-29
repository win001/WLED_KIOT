#ifndef WLED_FCN_DECLARE_H
#define WLED_FCN_DECLARE_H

/*
 * All globally accessible functions are declared here
 */

//alexa.cpp
#ifndef WLED_DISABLE_ALEXA
void onAlexaChange(EspalexaDevice* dev);
void alexaInit();
void handleAlexa();
void onAlexaChange(EspalexaDevice* dev);
#endif

//button.cpp
void getStringFromJson(char* dest, const char* src, size_t len);
void shortPressAction(uint8_t b=0);
void longPressAction(uint8_t b=0);
void doublePressAction(uint8_t b=0);
bool isButtonPressed(uint8_t b=0);
void handleButton();
void handleIO();

//cfg.cpp
bool deserializeConfig(JsonObject doc, bool fromFS = false);
void deserializeConfigFromFS();
bool deserializeConfigSec();
void serializeConfig();
void serializeConfigSec();

template<typename DestType>
bool getJsonValue(const JsonVariant& element, DestType& destination) {
  if (element.isNull()) {
    return false;
  }

  destination = element.as<DestType>();
  return true;
}

template<typename DestType, typename DefaultType>
bool getJsonValue(const JsonVariant& element, DestType& destination, const DefaultType defaultValue) {
  if(!getJsonValue(element, destination)) {
    destination = defaultValue;
    return false;
  }

  return true;
}


//colors.cpp
// similar to NeoPixelBus NeoGammaTableMethod but allows dynamic changes (superseded by NPB::NeoGammaDynamicTableMethod)
class NeoGammaWLEDMethod {
  public:
    static uint8_t Correct(uint8_t value);      // apply Gamma to single channel
    static uint32_t Correct32(uint32_t color);  // apply Gamma to RGBW32 color (WLED specific, not used by NPB)
    static void calcGammaTable(float gamma);    // re-calculates & fills gamma table
    static inline uint8_t rawGamma8(uint8_t val) { return gammaT[val]; }  // get value from Gamma table (WLED specific, not used by NPB)
  private:
    static uint8_t gammaT[];
};
#define gamma32(c) NeoGammaWLEDMethod::Correct32(c)
#define gamma8(c)  NeoGammaWLEDMethod::rawGamma8(c)
uint32_t color_blend(uint32_t,uint32_t,uint16_t,bool b16=false);
uint32_t color_add(uint32_t,uint32_t, bool fast=false);
uint32_t color_fade(uint32_t c1, uint8_t amount, bool video=false);
inline uint32_t colorFromRgbw(byte* rgbw) { return uint32_t((byte(rgbw[3]) << 24) | (byte(rgbw[0]) << 16) | (byte(rgbw[1]) << 8) | (byte(rgbw[2]))); }
void colorHStoRGB(uint16_t hue, byte sat, byte* rgb); //hue, sat to rgb
void colorKtoRGB(uint16_t kelvin, byte* rgb);
void colorCTtoRGB(uint16_t mired, byte* rgb); //white spectrum to rgb
void colorXYtoRGB(float x, float y, byte* rgb); // only defined if huesync disabled TODO
void colorRGBtoXY(byte* rgb, float* xy); // only defined if huesync disabled TODO
void colorFromDecOrHexString(byte* rgb, char* in);
bool colorFromHexString(byte* rgb, const char* in);
uint32_t colorBalanceFromKelvin(uint16_t kelvin, uint32_t rgb);
uint16_t approximateKelvinFromRGB(uint32_t rgb);
void setRandomColor(byte* rgb);

//dmx.cpp
void initDMX();
void handleDMX();

//e131.cpp
void handleE131Packet(e131_packet_t* p, IPAddress clientIP, byte protocol);
void handleArtnetPollReply(IPAddress ipAddress);
void prepareArtnetPollReply(ArtPollReply* reply);
void sendArtnetPollReply(ArtPollReply* reply, IPAddress ipAddress, uint16_t portAddress);

//file.cpp
bool handleFileRead(AsyncWebServerRequest*, String path);
bool writeObjectToFileUsingId(const char* file, uint16_t id, JsonDocument* content);
bool writeObjectToFile(const char* file, const char* key, JsonDocument* content);
bool readObjectFromFileUsingId(const char* file, uint16_t id, JsonDocument* dest);
bool readObjectFromFile(const char* file, const char* key, JsonDocument* dest);
void updateFSInfo();
void closeFile();

//hue.cpp
void handleHue();
void reconnectHue();
void onHueError(void* arg, AsyncClient* client, int8_t error);
void onHueConnect(void* arg, AsyncClient* client);
void sendHuePoll();
void onHueData(void* arg, AsyncClient* client, void *data, size_t len);

//improv.cpp
enum ImprovRPCType {
  Command_Wifi = 0x01,
  Request_State = 0x02,
  Request_Info = 0x03,
  Request_Scan = 0x04
};

void handleImprovPacket();
void sendImprovRPCResult(ImprovRPCType type, uint8_t n_strings = 0, const char **strings = nullptr);
void sendImprovStateResponse(uint8_t state, bool error = false);
void sendImprovInfoResponse();
void startImprovWifiScan();
void handleImprovWifiScan();
void sendImprovIPRPCResult(ImprovRPCType type);

//ir.cpp
void applyRepeatActions();
byte relativeChange(byte property, int8_t amount, byte lowerBoundary = 0, byte higherBoundary = 0xFF);
void decodeIR(uint32_t code);
void decodeIR24(uint32_t code);
void decodeIR24OLD(uint32_t code);
void decodeIR24CT(uint32_t code);
void decodeIR40(uint32_t code);
void decodeIR44(uint32_t code);
void decodeIR21(uint32_t code);
void decodeIR6(uint32_t code);
void decodeIR9(uint32_t code);
void decodeIRJson(uint32_t code);

void initIR();
void handleIR();

//json.cpp
#include "ESPAsyncWebServer.h"
#include "src/dependencies/json/ArduinoJson-v6.h"
#include "src/dependencies/json/AsyncJson-v6.h"
#include "FX.h"

bool deserializeSegment(JsonObject elem, byte it, byte presetId = 0);
bool deserializeState(JsonObject root, byte callMode = CALL_MODE_DIRECT_CHANGE, byte presetId = 0);
void serializeSegment(JsonObject& root, Segment& seg, byte id, bool forPreset = false, bool segmentBounds = true);
void serializeState(JsonObject root, bool forPreset = false, bool includeBri = true, bool segmentBounds = true, bool selectedSegmentsOnly = false);
void serializeInfo(JsonObject root);
void serializeModeNames(JsonArray root);
void serializeModeData(JsonArray root);
void serveJson(AsyncWebServerRequest* request);
void serializeNodes(JsonObject root);
void serializeNetworks(JsonObject root);
void serializePalettes(JsonObject root, int page);
void setPaletteColors(JsonArray json, byte* tcp);
void setPaletteColors(JsonArray json, CRGBPalette16 palette);
#ifdef WLED_ENABLE_JSONLIVE
bool serveLiveLeds(AsyncWebServerRequest* request, uint32_t wsClient = 0);
#endif

//led.cpp
void setValuesFromSegment(uint8_t s);
void setValuesFromMainSeg();
void setValuesFromFirstSelectedSeg();
void resetTimebase();
void toggleOnOff();
void applyBri();
void applyFinalBri();
void applyValuesToSelectedSegs();
void colorUpdated(byte callMode);
void stateUpdated(byte callMode);
void updateInterfaces(uint8_t callMode);
void handleTransitions();
void handleNightlight();
byte scaledBri(byte in);

#ifdef WLED_ENABLE_LOXONE
//lx_parser.cpp
bool parseLx(int lxValue, byte* rgbw);
void parseLxJson(int lxValue, byte segId, bool secondary);
#endif

//mqtt.cpp
// bool initMqtt();
// void publishMqtt();

void mqttSetup();
void mqttLoop();
void resetMqttBeatFlags();
bool canSendMqttMessage();
void setHeartBeatSchedule(unsigned long heart_beat);
void mqttSubscribe(const char* topic);
void mqttSubscribe(const char* topic, int QOS);
void _mqttOnMessage(char* topic, char* payload, int length);
void _mqttOnMessage(char* topic, char* payload, unsigned int len, int index, int total);
bool mqttSend(const char* topic, const char* message, bool stack_msg, bool dont_enctrypt, bool useGetter = true, bool retain = MQTT_RETAIN);
bool mqttSend(const char* topic, const char* message, bool stack_msg);
bool mqttSend(const char* topic, const char* message);
bool mqttSend(const char* topic, unsigned int index, const char* message, bool force);
bool mqttSend(const char* topic, unsigned int index, const char* message);
void settingsMQTTCallback(unsigned int type, const char* topic,
                          const char* payload, bool retained,
                          bool isEncrypted);
void mqttReset();
void wifiDisconnect();
void resetActual();
void setReportSettings();

//home.cpp
void homeSetupMQTT();
void homeReportSetup();
void homePong();
void reportEventToHome(String name, String value);
void homeLoop();
void homeSetup();
void needReportHome(bool report);
bool needReportHome();
void activeHomePong(bool immidiate);
void homeMQTTCallback(unsigned int type, const char* topic, const char* payload, bool retained, bool isEncrypted);

//ntp.cpp
void handleTime();
void handleNetworkTime();
void sendNTPPacket();
bool checkNTPResponse();
void updateLocalTime();
void getTimeString(char* out);
bool checkCountdown();
void setCountdown();
byte weekdayMondayFirst();
void checkTimers();
void calculateSunriseAndSunset();
void setTimeFromAPI(uint32_t timein);

//overlay.cpp
void handleOverlayDraw();
void _overlayAnalogCountdown();
void _overlayAnalogClock();

//playlist.cpp
void shufflePlaylist();
void unloadPlaylist();
int16_t loadPlaylist(JsonObject playlistObject, byte presetId = 0);
void handlePlaylist();
void serializePlaylist(JsonObject obj);

//presets.cpp
void initPresetsFile();
void handlePresets();
bool applyPreset(byte index, byte callMode = CALL_MODE_DIRECT_CHANGE);
void applyPresetWithFallback(uint8_t presetID, uint8_t callMode, uint8_t effectID = 0, uint8_t paletteID = 0);
inline bool applyTemporaryPreset() {return applyPreset(255);};
void savePreset(byte index, const char* pname = nullptr, JsonObject saveobj = JsonObject());
inline void saveTemporaryPreset() {savePreset(255);};
void deletePreset(byte index);
bool getPresetName(byte index, String& name);

//remote.cpp
void handleRemote();

//set.cpp
bool isAsterisksOnly(const char* str, byte maxLen);
void handleSettingsSet(AsyncWebServerRequest *request, byte subPage);
bool handleSet(AsyncWebServerRequest *request, const String& req, bool apply=true);

//udp.cpp
void notify(byte callMode, bool followUp=false);
uint8_t realtimeBroadcast(uint8_t type, IPAddress client, uint16_t length, uint8_t *buffer, uint8_t bri=255, bool isRGBW=false);
void realtimeLock(uint32_t timeoutMs, byte md = REALTIME_MODE_GENERIC);
void exitRealtime();
void handleNotifications();
void setRealtimePixel(uint16_t i, byte r, byte g, byte b, byte w);
void refreshNodeList();
void sendSysInfoUDP();

//network.cpp
int getSignalQuality(int rssi);
void WiFiEvent(WiFiEvent_t event);

//um_manager.cpp
typedef enum UM_Data_Types {
  UMT_BYTE = 0,
  UMT_UINT16,
  UMT_INT16,
  UMT_UINT32,
  UMT_INT32,
  UMT_FLOAT,
  UMT_DOUBLE,
  UMT_BYTE_ARR,
  UMT_UINT16_ARR,
  UMT_INT16_ARR,
  UMT_UINT32_ARR,
  UMT_INT32_ARR,
  UMT_FLOAT_ARR,
  UMT_DOUBLE_ARR
} um_types_t;
typedef struct UM_Exchange_Data {
  // should just use: size_t arr_size, void **arr_ptr, byte *ptr_type
  size_t       u_size;                 // size of u_data array
  um_types_t  *u_type;                 // array of data types
  void       **u_data;                 // array of pointers to data
  UM_Exchange_Data() {
    u_size = 0;
    u_type = nullptr;
    u_data = nullptr;
  }
  ~UM_Exchange_Data() {
    if (u_type) delete[] u_type;
    if (u_data) delete[] u_data;
  }
} um_data_t;
const unsigned int um_data_size = sizeof(um_data_t);  // 12 bytes

class Usermod {
  protected:
    um_data_t *um_data; // um_data should be allocated using new in (derived) Usermod's setup() or constructor
  public:
    Usermod() { um_data = nullptr; }
    virtual ~Usermod() { if (um_data) delete um_data; }
    virtual void setup() = 0; // pure virtual, has to be overriden
    virtual void loop() = 0;  // pure virtual, has to be overriden
    virtual void handleOverlayDraw() {}                                      // called after all effects have been processed, just before strip.show()
    virtual bool handleButton(uint8_t b) { return false; }                   // button overrides are possible here
    virtual bool getUMData(um_data_t **data) { if (data) *data = nullptr; return false; }; // usermod data exchange [see examples for audio effects]
    virtual void connected() {}                                              // called when WiFi is (re)connected
    virtual void appendConfigData() {}                                       // helper function called from usermod settings page to add metadata for entry fields
    virtual void addToJsonState(JsonObject& obj) {}                          // add JSON objects for WLED state
    virtual void addToJsonInfo(JsonObject& obj) {}                           // add JSON objects for UI Info page
    virtual void readFromJsonState(JsonObject& obj) {}                       // process JSON messages received from web server
    virtual void addToConfig(JsonObject& obj) {}                             // add JSON entries that go to cfg.json
    virtual bool readFromConfig(JsonObject& obj) { return true; } // Note as of 2021-06 readFromConfig() now needs to return a bool, see usermod_v2_example.h
    virtual void onMqttConnect(bool sessionPresent) {}                       // fired when MQTT connection is established (so usermod can subscribe)
    virtual bool onMqttMessage(char* topic, char* payload) { return false; } // fired upon MQTT message received (wled topic)
    virtual void onUpdateBegin(bool) {}                                      // fired prior to and after unsuccessful firmware update
    virtual void onStateChange(uint8_t mode) {}                              // fired upon WLED state change
    virtual uint16_t getId() {return USERMOD_ID_UNSPECIFIED;}
};

class UsermodManager {
  private:
    Usermod* ums[WLED_MAX_USERMODS];
    byte numMods = 0;

  public:
    void loop();
    void handleOverlayDraw();
    bool handleButton(uint8_t b);
    bool getUMData(um_data_t **um_data, uint8_t mod_id = USERMOD_ID_RESERVED); // USERMOD_ID_RESERVED will poll all usermods
    void setup();
    void connected();
    void appendConfigData();
    void addToJsonState(JsonObject& obj);
    void addToJsonInfo(JsonObject& obj);
    void readFromJsonState(JsonObject& obj);
    void addToConfig(JsonObject& obj);
    bool readFromConfig(JsonObject& obj);
    void onMqttConnect(bool sessionPresent);
    bool onMqttMessage(char* topic, char* payload);
    void onUpdateBegin(bool);
    void onStateChange(uint8_t);
    bool add(Usermod* um);
    Usermod* lookup(uint16_t mod_id);
    byte getModCount() {return numMods;};
};

//usermods_list.cpp
void registerUsermods();

//usermod.cpp
void userSetup();
void userConnected();
void userLoop();

//util.cpp
// START eepromrotate related
void heartbeat_new();
bool reportEventToMQTT(const char *ev, const char *meta);
bool reportEventToMQTT(const char * ev, JsonObject& meta);
void eepromRotate(bool value);
String eepromSectors();
void eepromSectorsDebug();
uint32_t eepromCurrent();
void eepromBackup(uint32_t index) ;
void _eepromInitCommands();
void eepromSetup();
// END eepromrotate related

void wifiStatus();
void infoProduction();
void info();
void loopRegister(void (*callback)());
bool checkNeedsReset();
void deferredReset(unsigned long delay, unsigned char reason);
int getNumVal(const String* req, uint16_t pos);
void parseNumber(const char* str, byte* val, byte minv=0, byte maxv=255);
bool getVal(JsonVariant elem, byte* val, byte minv=0, byte maxv=255);
bool updateVal(const char* req, const char* key, byte* val, byte minv=0, byte maxv=255);
bool oappend(const char* txt); // append new c string to temp buffer efficiently
bool oappendi(int i);          // append new number to temp buffer efficiently
void sappend(char stype, const char* key, int val);
void sappends(char stype, const char* key, char* val);
void prepareHostname(char* hostname);
bool isAsterisksOnly(const char* str, byte maxLen);
bool requestJSONBufferLock(uint8_t module=255);
void releaseJSONBufferLock();
uint8_t extractModeName(uint8_t mode, const char *src, char *dest, uint8_t maxLen);
uint8_t extractModeSlider(uint8_t mode, uint8_t slider, char *dest, uint8_t maxLen, uint8_t *var = nullptr);
int16_t extractModeDefaults(uint8_t mode, const char *segVar);
void checkSettingsPIN(const char *pin);
uint16_t crc16(const unsigned char* data_p, size_t length);
um_data_t* simulateSound(uint8_t simulationId);
void enumerateLedmaps();
uint8_t get_random_wheel_index(uint8_t pos);
void printFileContentFS(const char* path);
long long char2LL(char *str);
void subString(char *source, char *destination, int start_index, int end_index);
int getBase64Len(int len);
int getCipherlength(int len);
String rightString(const char *s, size_t length);
unsigned int getFreeHeap();
unsigned int getInitialFreeHeap();
unsigned char resetReason();
void resetReason(unsigned char reason);
bool compareTrimmedStrings(const char* str1, const char* str2);
uint8_t trimmedLength(const char* str);
// RAII guard class for the JSON Buffer lock
// Modeled after std::lock_guard
class JSONBufferGuard {
  bool holding_lock;
  public:
    inline JSONBufferGuard(uint8_t module=255) : holding_lock(requestJSONBufferLock(module)) {};
    inline ~JSONBufferGuard() { if (holding_lock) releaseJSONBufferLock(); };
    inline JSONBufferGuard(const JSONBufferGuard&) = delete; // Noncopyable
    inline JSONBufferGuard& operator=(const JSONBufferGuard&) = delete;
    inline JSONBufferGuard(JSONBufferGuard&& r) : holding_lock(r.holding_lock) { r.holding_lock = false; };  // but movable
    inline JSONBufferGuard& operator=(JSONBufferGuard&& r) { holding_lock |= r.holding_lock; r.holding_lock = false; return *this; };
    inline bool owns_lock() const { return holding_lock; }
    explicit inline operator bool() const { return owns_lock(); };
    inline void release() { if (holding_lock) releaseJSONBufferLock(); holding_lock = false; }
};

#ifdef WLED_ADD_EEPROM_SUPPORT
//wled_eeprom.cpp
void applyMacro(byte index);
void deEEP();
void deEEPSettings();
void clearEEPROM();
#endif

//wled_math.cpp
#ifndef WLED_USE_REAL_MATH
  template <typename T> T atan_t(T x);
  float cos_t(float phi);
  float sin_t(float x);
  float tan_t(float x);
  float acos_t(float x);
  float asin_t(float x);
  float floor_t(float x);
  float fmod_t(float num, float denom);
#else
  #include <math.h>
  #define sin_t sin
  #define cos_t cos
  #define tan_t tan
  #define asin_t asin
  #define acos_t acos
  #define atan_t atan
  #define fmod_t fmod
  #define floor_t floor
#endif

//wled_serial.cpp
void handleSerial();
void updateBaudRate(uint32_t rate);

//wled_server.cpp
bool isIp(String str);
void createEditHandler(bool enable);
bool captivePortal(AsyncWebServerRequest *request);
void initServer();
void serveIndexOrWelcome(AsyncWebServerRequest *request);
void serveIndex(AsyncWebServerRequest* request);
String msgProcessor(const String& var);
void serveMessage(AsyncWebServerRequest* request, uint16_t code, const String& headl, const String& subl="", byte optionT=255);
String dmxProcessor(const String& var);
void serveSettings(AsyncWebServerRequest* request, bool post = false);
void serveSettingsJS(AsyncWebServerRequest* request);

//ws.cpp
void handleWs();
void wsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
void sendDataWs(AsyncWebSocketClient * client = nullptr);

//xml.cpp
void XML_response(AsyncWebServerRequest *request, char* dest = nullptr);
void URL_response(AsyncWebServerRequest *request);
void getSettingsJS(byte subPage, char* dest);

//kiot_web.cpp
void webSetup();
bool configStoreWifi(JsonArray wifiArray);
bool configStore(String key, JsonArray value);
bool configStore(String key, String value);
void getApiKeyChecksum(char *inpBuf);
void respondUnauthorized(AsyncWebServerRequest *request);
void responseBadRequest(AsyncWebServerRequest * request, const char * message);
void _onWifiScanServer(AsyncWebServerRequest* request);
void _wifiScan(DynamicJsonDocument& root);
void _wifiScanFix();
void processConfig(JsonObject config, const char *from, char *buffer, size_t bufferSize);
void _processAction(String action, JsonObject meta);
void _processAction(String action);

//kiot_wled.cpp
void kiotWledInitCommands();
void kiotHandleSettingsSet(JsonObject root, byte subPage);
void kiotWledPing(JsonObject &root);
void kiotWledSetup();
void kiotWledLoop();
void kiotWledConfigure();
void kiotWledMQTTCallback(unsigned int type, const char *topic, const char *payload, bool retained, bool isEncrypted); 

//httpclient.cpp
void httpClientLoop();
String _getCNonce(const int len);
void setFetchCustomConfig(bool val);
void setFetchMqttConf(bool val);
void setFetchRPCAction(bool val);
void httpClientSetup();

//base64.cpp
int base64_encode(char *output, char *input, int inputLen);
int base64_decode(char *output, char *input, int inputLen);
int base64_enc_len(int inputLen);
int base64_dec_len(char *input, int inputLen);

//encrypt.cpp
void encryptSetup();
void setupEncryption();
void char_array_to_byte_array(const char *str, byte byte_arr[], uint8_t string_length);
void temp_char_string_to_byte(const char *str, uint8_t byte_arr[], uint8_t string_length);
void encryptMsg(const char *msg, char *data_encoded, char *iv_encoded, byte *aesKey);
void encryptMsg(const char *msg, char *data_encoded, char *iv_encoded);
void decryptMsg(const char *payload, char *message, int payload_len, const char *iv_encoded, byte *aesKey);
void decryptMsg(const char *payload, char *message, int payload_len, const char *iv_encoded);
void decodeWifiPass(const char *RevB64Pass, char *buffer, int payloadLen);
uint8_t getrnd();
void gen_iv(byte *iv);
void hmacHash(const char *message, char *output, const char *key, int key_length);
bool compareHash(const char *message, const char *hash);

//debiug.cpp
void _debugSend(char* message);
void debugSetup();
void debugSend(const char* format, ...);
void debugSend_P(PGM_P format_P, ...);
void serialSend_P(PGM_P format, ...);
void serialSend(const char* format, ...); 
void debugConfigure();

// kiot_system.cpp
void systemSetup();
void serialDisabled(bool value);
bool serialDisabled();

//kiot_settings.cpp
#include <Embedis.h>
#include "libs/EmbedisWrap.h"
#include <EEPROM_Rotate.h>
#include <Ticker.h>

void getSupportedConfigWeb(DynamicJsonDocument& root);
void settingsLoop();
void settingsSetup();
void _settingsInitCommands();
void settingsRegisterCommand(const String& name, void (*call)(Embedis*));
void KiotCustomSet(Embedis* e);
void KiotCustomGet(Embedis* e);
void KiotCustomDel(Embedis* e);
void moveSetting(const String from, const String to);
template <typename T> bool setSetting(const String& key, T value) {
    return Embedis::set(key, String(value));
}
template <typename T> bool setSetting(const String& key, unsigned int index, T value) {
    return setSetting(key + String(index), value);
}
template <typename T> String getSetting(const String& key, T defaultValue) {
    String value;
    if (!Embedis::get(key, value)) value = String(defaultValue);
    return value;
}
template <typename T> String getSetting(const String& key, unsigned int index, T defaultValue) {
    return getSetting(key + String(index), defaultValue);
}
// template <typename T> String getSetting(const String& key, unsigned int index, T defaultValue);
// template <typename T> String getSetting(const String& key, T defaultValue);
String getSetting(const String& key);
// template <typename T> bool setSetting(const String& key, unsigned int index, T value);
// template <typename T> bool setSetting(const String& key, T value);
bool delSetting(const String& key);
bool delSetting(const String& key, unsigned int index);
bool hasSetting(const String& key);
bool hasSetting(const String& key, unsigned int index);
void saveSettings();
void settingsFactoryReset();
void migrateSettings();
bool setDefaultSettings();
void initSettigs();
String generateApiKey();
void _deleteDevice();
void settingsSetupMQTT();
bool settingsMQTT();
unsigned long settingsSize();

//prototypes.h
// #include <Arduino.h>
// #include <ArduinoJson.h>
// #include <pgmspace.h>
// #include <functional>
// #include <IRsend.h>

// extern "C" {
// #include "user_interface.h"
// }

// #if WEB_SUPPORT

// // -----------------------------------------------------------------------------
// // WebServer
// // -----------------------------------------------------------------------------
// #include <ESPAsyncWebServer.h>
// AsyncWebServer* webServer();

// // -----------------------------------------------------------------------------
// // API
// // -----------------------------------------------------------------------------
typedef std::function<void(char*, size_t)> apiGetCallbackFunction;
typedef std::function<void(const char*, const char*, bool)> apiPostCallbackFunction;
void apiRegister(const char* url, const char* key, apiGetCallbackFunction getFn, apiPostCallbackFunction postFn = NULL);

typedef std::function<void(void)> after_config_parse_callback_f;
void afterConfigParseRegister(after_config_parse_callback_f callback);

typedef std::function<void(DynamicJsonDocument&, char* topic, bool nested_under_topic)> set_config_keys_callback_f;
void setConfigKeys(set_config_keys_callback_f callback);

// // Core version 2.4.2 and higher changed the cont_t structure to a pointer:
// // https://github.com/esp8266/Arduino/commit/5d5ea92a4d004ab009d5f642629946a0cb8893dd#diff-3fa12668b289ccb95b7ab334833a4ba8L35
// // Core version 2.5.0 introduced EspClass helper method:
// // https://github.com/esp8266/Arduino/commit/0e0e34c614fe8a47544c9998201b1d9b3c24eb18
// extern "C" {

//     #include <cont.h>
// /* #if defined(ARDUINO_ESP8266_RELEASE_2_3_0) \
//     || defined(ARDUINO_ESP8266_RELEASE_2_4_0) \
//     || defined(ARDUINO_ESP8266_RELEASE_2_4_1)
//     extern cont_t g_cont;
//     #define getFreeStack() cont_get_free_stack(&g_cont)
// #elif defined(ARDUINO_ESP8266_RELEASE_2_4_2)
//     extern cont_t* g_pcont;
//     #define getFreeStack() cont_get_free_stack(g_pcont)
// #else
//     #define getFreeStack() ESP.getFreeContStack()
// #endif */
// extern cont_t* g_pcont;
// #define getFreeStack() cont_get_free_stack(g_pcont)
// }

// // -----------------------------------------------------------------------------
// // EEPROM_ROTATE
// // -----------------------------------------------------------------------------
// #include <EEPROM_Rotate.h>
// EEPROM_Rotate EEPROMr;

// /*
// // Commented by KIOT

// // -----------------------------------------------------------------------------
// // WebSockets
// // -----------------------------------------------------------------------------
// typedef std::function<void(JsonObject&)> ws_on_send_callback_f;
// void wsOnSendRegister(ws_on_send_callback_f callback);
// void wsSend(ws_on_send_callback_f sender);

// typedef std::function<void(const char *, JsonObject&)> ws_on_action_callback_f;
// void wsOnActionRegister(ws_on_action_callback_f callback);

// typedef std::function<void(void)> ws_on_after_parse_callback_f;
// void wsOnAfterParseRegister(ws_on_after_parse_callback_f callback);

// */

// #endif  // WEB_SUPPORT

// // void mqttRegister(void (*callback)(unsigned int, const char *, const char *));
// // String mqttSubtopic(char * topic);
// /*
// // template<typename T> bool setSetting(const String& key, T value);
// // template<typename T> bool setSetting(const String& key, unsigned int index, T value);
// // template<typename T> String getSetting(const String& key, T defaultValue);
// // template<typename T> String getSetting(const String& key, unsigned int index, T defaultValue);
// */

// // -----------------------------------------------------------------------------
// // Settings
// // -----------------------------------------------------------------------------
// #include <Embedis.h>
// template <typename T> bool setSetting(const String& key, T value);
// template <typename T> bool setSetting(const String& key, unsigned int index, T value);
// template <typename T> String getSetting(const String& key, T defaultValue);
// template <typename T> String getSetting(const String& key, unsigned int index, T defaultValue);
void settingsGetJson(JsonObject& data);
// bool settingsRestoreJson(JsonObject& data);
// void settingsRegisterCommand(const String& name, void (*call)(Embedis*));

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------
typedef std::function<void(unsigned int, const char*, const char*, bool, bool)> mqtt_callback_f;
void mqttRegister(mqtt_callback_f callback);
String mqttSubtopic(char* topic);
bool mqttSend(const char* topic, const char* message, bool stack_msg, bool dont_enctrypt, bool useGetter, bool retain);
bool mqttSendRaw(const char* topic, const char* message, bool dont_encrypt, bool retain);

// // -----------------------------------------------------------------------------
// // WiFi
// // -----------------------------------------------------------------------------

// #include "JustWifi.h"
// typedef std::function<void(justwifi_messages_t code, char* parameter)> wifi_callback_f;
// void wifiRegister(wifi_callback_f callback);
// bool createLongAP(bool withDelay = true);

// // -----------------------------------------------------------------------------
// // Utils
// // -----------------------------------------------------------------------------
// char* ltrim(char* s);

// template <class MapType> class MapKeyIterator {
//    public:
//     class iterator {
//        public:
//         iterator(typename MapType::iterator it) : it(it) {}
//         iterator operator++() { return ++it; }
//         bool operator!=(const iterator& other) { return it != other.it; }
//         typename MapType::key_type operator*() const { return it->first; }  // Return key part of map
//        private:
//         typename MapType::iterator it;
//     };

//    private:
//     MapType& map;

//    public:
//     MapKeyIterator(MapType& m) : map(m) {}
//     iterator begin() { return iterator(map.begin()); }
//     iterator end() { return iterator(map.end()); }
// };
// template <class MapType> MapKeyIterator<MapType> MapKeys(MapType& m) {
//     return MapKeyIterator<MapType>(m);
// }

// // HTTPSERVER
// void _httpGetRaw(const char* path, const char* query);

// // -----------------------------------------------------------------------------
// // GPIO
// // -----------------------------------------------------------------------------
// bool gpioValid(unsigned char gpio);
// bool gpioGetLock(unsigned char gpio);
// bool gpioReleaseLock(unsigned char gpio);



// // -----------------------------------------------------------------------------
// // HM11
// // -----------------------------------------------------------------------------

// typedef enum  {UART_ERR_OK, UART_ERR_TIMEOUT} Uart_Errors;
// Uart_Errors GetData(char *receivedBuff, int expectedLen, unsigned int timeOut); //expect some data in specified time

// // ---------------------------------------------------------
// // IR 
// // ---------------------------------------------------------
// void SendAcGeneric(stdAc::state_t acState);

// // TUYA
// #if TUYA_SUPPORT
// namespace Tuya {
//     void tuyaSetup();
//     void tuyaSetupSwitch();
//     void tuyaSetupDimmer();
//     void tuyaSendSwitch(unsigned char, bool);  
//     void tuyaSendDimmer(unsigned char, unsigned int);
//     void sendWiFiStatus();
// #if THERMOSTAT_PROVIDER == THERMOSTAT_PROVIDER_TUYA
//     void tuyaSetupThermostat();
//     void tuyaSendThermostatPower(bool);
//     void tuyaSendThermostatTemp(float);
//     void tuyaSendThermostatFanMode(uint8_t);
//     void tuyaSendThermostatOpMode(uint8_t);
//     void tuyaSendDPReportSucess(bool);
//     // void sendThermoStateRoomTemp();
// #endif
// #if CURTAIN_PROVIDER == CURTAIN_PROVIDER_TUYA
//     void tuyaSetupCurtain();
//     void tuyaSendCurtainPostion(uint8_t);
//     void tuyaSendCurtainSwitch(uint8_t);
//     void tuyaSendCurtainReverseState(bool);
// #endif
// #if TUYA_SENSOR_TYPE == TY_SENSOR_GAS
//     void tuyaSetupGasSensor();
//     void tuyasendAlarmTimetoMCU(int);
//     void tuyasendMufflingtoMCU(bool);
//     void tuyasendCurrentTimetoMCU(uint8_t *currTime);
//     void tuyasendMaxTempAlarmValuetoMCU(int);
//     void tuyasendOutdoorTemptoMCU(int);
//     void tuyasendSelfCheckingtoMCU(bool);   
// #endif
// }
// template <typename T> void updateSensorState(uint8_t type, uint8_t dp_id, T value);
// #endif

// void fireIR(unsigned long long actCode, int length, int proto, int repeat, int freq, stdAc::state_t acState);

#endif
