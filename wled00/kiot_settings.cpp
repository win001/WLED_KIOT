/*

SETTINGS MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/
#include "wled.h"
// #include <EEPROM_Rotate.h> // moved to fcn_declare.h
#include <Stream.h>
// #include "libs/EmbedisWrap.h" // moved to fcn_declare.h
#include "spi_flash.h"
#include <libs/StreamInjector.h>

// StreamInjector _serial = StreamInjector(TERMINAL_BUFFER_SIZE);
EmbedisWrap embedis(Serial);
EEPROM_Rotate EEPROMr;

// START move it to system.ino TODO_S1
bool _serialDisabled = false;

bool serialDisabled(){
    return _serialDisabled;
}

void serialDisabled(bool value){
    _serialDisabled = value;
}
// END

unsigned long last_settings_sent = 0;
bool _settings_save = false;
bool _sendSettings = false;

std::vector<set_config_keys_callback_f> set_config_keys_callbacks;

// Config Functions
void copyKiotSettings();

void setConfigKeys(set_config_keys_callback_f callback) {
    set_config_keys_callbacks.push_back(callback);
}

void getSupportedConfigWeb(DynamicJsonDocument& root) {

    for (int i = 0; i < set_config_keys_callbacks.size(); i++) {
        char topic[MAX_CONFIG_TOPIC_SIZE] = "";
        (set_config_keys_callbacks[i])(root, topic, true);

    }
}

void setReportSettings() {
    // This is to protect the device from sending config messages two times
    if (mqtt_firstbeat_reported) {
        _sendSettings = true;
    }
}
bool getReportSetting() {
    return _sendSettings;
}

String generateApiKey() {
    char apiKey[21] = "";
    for (int i = 0; i < 10; i++) {
        sprintf(apiKey, "%s%02x", apiKey, getrnd());
    }
    apiKey[20] = '\0';
    return String(apiKey);
}
// -----------------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------------
// Stream & settingsSerial() {
//     return (Stream &) _serial;
// }

size_t settingsMaxSize() {
    size_t size = EEPROM_SIZE;
    if (size > SPI_FLASH_SEC_SIZE) size = SPI_FLASH_SEC_SIZE;
    size = (size + 3) & (~3);
    return size;
}

String getIdentifier() {
    if (getSetting("identifier").length() == 0) {
        char identifier[13] = "";
        uint8_t MAC_array[6];
        WiFi.macAddress(MAC_array);
        for (int i = 0; i < sizeof(MAC_array); ++i) {
            sprintf(identifier, "%s%02x", identifier, MAC_array[i]);
        }

        DEBUG_MSG_P(PSTR("[DEBUG]  MacString: %s\n"), identifier);
        setSetting("identifier", identifier);
        return String(identifier);
    } else {
        return getSetting("identifier", "");
    }
}

unsigned long settingsSize() {
    unsigned pos = SPI_FLASH_SEC_SIZE - 1;
    while (size_t len = EEPROMr.read(pos)) {
        if (0xFF == len) break;
        pos = pos - len - 2;
    }
    return SPI_FLASH_SEC_SIZE - pos + EEPROM_DATA_END;
}

unsigned int settingsKeyCount() {
    unsigned count = 0;
    unsigned pos = SPI_FLASH_SEC_SIZE - 1;
    while (size_t len = EEPROMr.read(pos)) {
        if (0xFF == len) break;
        pos = pos - len - 2;
        len = EEPROMr.read(pos);
        pos = pos - len - 2;
        count++;
    }
    return count;
}

unsigned int _settingsKeyCount() {
    unsigned count = 0;
    unsigned pos = SPI_FLASH_SEC_SIZE - 1;
    while (size_t len = EEPROMr.read(pos)) {
        if (0xFF == len) break;
        pos = pos - len - 2;
        len = EEPROMr.read(pos);
        pos = pos - len - 2;
        count++;
    }
    return count;
}

String settingsKeyName(unsigned int index) {
    String s;

    unsigned count = 0;
    unsigned pos = SPI_FLASH_SEC_SIZE - 1;
    while (size_t len = EEPROMr.read(pos)) {
        if (0xFF == len) break;
        pos = pos - len - 2;
        if (count == index) {
            s.reserve(len);
            for (unsigned char i = 0; i < len; i++) {
                s += (char)EEPROMr.read(pos + i + 1);
            }
            break;
        }
        count++;
        len = EEPROMr.read(pos);
        pos = pos - len - 2;
    }

    return s;
}

std::vector<String> _settingsKeys() {
    // Get sorted list of keys
    std::vector<String> keys;

    //unsigned int size = settingsKeyCount();
    unsigned int size = settingsKeyCount();
    for (unsigned int i = 0; i < size; i++) {
        //String key = settingsKeyName(i);
        String key = settingsKeyName(i);
        bool inserted = false;
        for (unsigned char j = 0; j < keys.size(); j++) {
            // Check if we have to insert it before the current element
            if (keys[j].compareTo(key) > 0) {
                keys.insert(keys.begin() + j, key);
                inserted = true;
                break;
            }
        }

        // If we could not insert it, just push it at the end
        if (!inserted) keys.push_back(key);
    }

    return keys;
}

void settingsGetJson(JsonObject& root) {
    unsigned int size = _settingsKeyCount();
    for (unsigned int i = 0; i < size; i++) {
        String key = settingsKeyName(i);
        String value = getSetting(key);
        // Filter Secret Keys here
        if (key.equals("aesKey") || key.equals("apiSecret") || key.equals("apiKey")) {
            continue;
        }

        root[key] = value;
    }
}

// -----------------------------------------------------------------------------
// Commands
// -----------------------------------------------------------------------------

void _settingsHelpCommand() {
    // Get sorted list of commands
    std::vector<String> commands;
    unsigned char size = embedis.getCommandCount();
    for (unsigned int i = 0; i < size; i++) {
        String command = embedis.getCommandName(i);
        bool inserted = false;
        for (unsigned char j = 0; j < commands.size(); j++) {
            // Check if we have to insert it before the current element
            if (commands[j].compareTo(command) > 0) {
                commands.insert(commands.begin() + j, command);
                inserted = true;
                break;
            }
        }

        // If we could not insert it, just push it at the end
        if (!inserted) commands.push_back(command);
    }

    // Output the list
    DEBUG_MSG_P(PSTR("Available commands:\n"));
    for (unsigned char i = 0; i < commands.size(); i++) {
        DEBUG_MSG_P(PSTR("> %s\n"), (commands[i]).c_str());
    }
}

void _settingsKeysCommand() {
    // Get sorted list of keys
    std::vector<String> keys = _settingsKeys();

    // Write key-values
    DEBUG_MSG_P(PSTR("Current settings:\n"));
    for (unsigned int i = 0; i < keys.size(); i++) {
        String value = getSetting(keys[i]);
        DEBUG_MSG_P(PSTR("> %s => \"%s\"\n"), (keys[i]).c_str(), value.c_str());
    }

    unsigned long freeEEPROM = SPI_FLASH_SEC_SIZE - settingsSize();
    DEBUG_MSG_P(PSTR("Number of keys: %d\n"), keys.size());
    DEBUG_MSG_P(PSTR("Current EEPROM sector: %u\n"), EEPROMr.current());
    DEBUG_MSG_P(PSTR("Free EEPROM: %d bytes (%d%%)\n"), freeEEPROM, 100 * freeEEPROM / SPI_FLASH_SEC_SIZE);
}

void _deleteDevice() {
    // Delete Home id
    // Delete Wifi Details
    /* delSetting("homeId");
    // Delete all other settings

    // Deleting wifi settings
    int i = 0;
    while (i < WIFI_MAX_NETWORKS) {
        delSetting("ssid", i);
        delSetting("pass", i);
        delSetting("ip", i);
        delSetting("gw", i);
        delSetting("mask", i);
        delSetting("dns", i);
        ++i;
    }
    // Deleting MQTT Settings
    delSetting("mqttServer");
    // Delete api Key
    delSetting("apiKey");    

    saveSettings(); */

    // perform a factory reset instead.
    settingsFactoryReset();
}

void settingsFactoryReset() {
    // Protect These keys from deleting even in factory reset.
    double pwrRatioP = getSetting("pwRP").toFloat();
    double pwrRatioC = getSetting("pwRC").toFloat();
    double pwrRatioV = getSetting("pwRV").toFloat();
    //for RGB Only do we need ypo use any macro here?
    bool LEDSHOW = getSetting("LSW","0").toInt();
    // Preserving Secrets as well
    String aesKey = getSetting("aesKey");
    String apiSecret = getSetting("apiSecret");
    String httpPass = getSetting("httpPass");
    // String apiKey = getSetting("apiKey");

    for (unsigned int i = 0; i < SPI_FLASH_SEC_SIZE; i++) {
        EEPROMr.write(i, 0xFF);
    }

    setSetting("pwRP", pwrRatioP);
    setSetting("pwRC", pwrRatioC);
    setSetting("pwRV", pwrRatioV);

    setSetting("aesKey", aesKey);
    setSetting("apiSecret", apiSecret);
    setSetting("httpPass", httpPass);

    // DEBUG_MSG_P(PSTR("[FACTORYSET] EEPROM RELAY STATUS - %d \n"), EEPROMr.read(EEPROM_RELAY_STATUS));

    saveSettings();

    // deferredReset(1000, CUSTOM_RESET_FACTORY); TODO_S1
}

void _eepromRraseAll() {
    for (uint32_t j = 0; j < EEPROMr.size(); j++) {
        for (unsigned int i = 0; i < SPI_FLASH_SEC_SIZE; i++) {
            EEPROMr.write(i, 0xFF);
        }
        EEPROMr.commit();
    }
}

void settingsSetup() {
    EEPROMr.begin(SPI_FLASH_SEC_SIZE);

    Embedis::dictionary(F("EEPROM"),
                        SPI_FLASH_SEC_SIZE,
                        [](size_t pos) -> char { return EEPROMr.read(pos); },
                        [](size_t pos, char value) { EEPROMr.write(pos, value); },
#if AUTO_SAVE
                        // []() { EEPROM.commit(); }
                        []() { _settings_save = true; }
#else
                        []() {}
#endif
    );

    _settingsInitCommands();

    // Embedis::hardware( F("WIFI"), [](Embedis* e) {
    //     StreamString s;
    //     WiFi.printDiag(s);
    //     DEBUG_MSG(s.c_str());
    // }, 0);

    // -------------------------------------------------------------------------

    /* --- Device Specific Settings */

    initSettigs();

#ifdef MQTT_SETTINGS_ENABLED
    // settingsSetupMQTT(); TODO_S1
#endif

    DEBUG_MSG_P(PSTR("\n"));
    DEBUG_MSG_P(PSTR("[SETTINGS] EEPROM size: %d bytes\n"), SPI_FLASH_SEC_SIZE);
    DEBUG_MSG_P(PSTR("[SETTINGS] Settings size: %d bytes\n"), settingsSize());
    // Callbacks
    afterConfigParseRegister(setReportSettings);
    // loopRegister(settingsLoop); TODO_S1
    migrateSettings();
    copyKiotSettings();
}
void initSettigs() {
    // Settingup Hostname
    bool changed = false;
    // if (getSetting("hostname").length() == 0) {

    String identifier = getIdentifier();
    char buffer[strlen(DEVICE) + identifier.length() + 3];
    DEBUG_MSG_P(PSTR("[DEBUG] Hostname: %s_%s\n"), DEVICE, identifier.c_str());
    sprintf(buffer, "%s_%s", DEVICE, identifier.c_str());
    String hostname = String(buffer);
    hostname.toLowerCase();
    if (!getSetting("hostname").equals(hostname)) {
        setSetting("hostname", hostname.c_str());
        changed = true;
    }

    // }

    if (!hasSetting("apiKey") || !hasSetting("apiSecret") || !hasSetting("aesKey")) {
// setSecuritySettings();
// changed = true;
#if HTTP_CONF_SUPPORT
        // Not always required
        // fetchSecurityConf = true;
#endif
    }
    if (setDefaultSettings()) changed = true;

    if (changed) {
        saveSettings();
    }
}

bool setDefaultSettings() {
    bool changed = false;
    // Empty function for now, later it can be updated with stuff.
    return changed;
}
void settingsLoop()
{
    if (_settings_save)
    {
        DEBUG_MSG_P(PSTR("[SETTINGS] Saving\n"));

        bool commited = EEPROMr.commit();
        // Serial.println(commited ? "Commited": "Error");
        if (commited)
        {
            _settings_save = false;
        }
    }
    bool _skip_embedis = false;
    if (!serialDisabled())
    {
        if (!_skip_embedis)
        {
            embedis.process();
        }
    }

    if (_sendSettings)
    {
        // DEBUG_MSG_P(PSTR("I am being called \n"));
        if (0)//(settingsMQTT()) TODO_S1
        {
            _sendSettings = false;
        }
    }
}

void _settingsInitCommands() {
    // TODO_S1 : Add all the commands here
//     settingsRegisterCommand(F("WIFIRESET"), [](Embedis* e) {
//         _wifiConfigure();
//         wifiDisconnect();
//         DEBUG_MSG_P(PSTR("Done\n"));
//     });

// #if SENSOR_SUPPORT
//     settingsRegisterCommand(F("TESTSensorReport"), [](Embedis* e) {
//         // wifiReconnectCheck(200);
//         // _httpGetRaw(String("Something"));
//         // setLedPattern(6);
//         // DEBUG_MSG_P(PSTR("Done\n"));
//         DynamicJsonBuffer jsonBuffer(250);
//         JsonObject& root = jsonBuffer.createObject();
//         DEBUG_MSG_P(PSTR("Came Here \n"));
//         sensorString(jsonBuffer, root);
//         DEBUG_MSG_P(PSTR("Came Here 2 \n"));
//         String output;
//         root.printTo(output);
//         DEBUG_MSG_P(PSTR("Came Here 3 \n"));
//         Serial.println(output);
//     });
// #endif

// #if DEVELOPEMEMT_MODE
//     /* settingsRegisterCommand(F(" "), [](Embedis* e) {
//         String wifiBuffer;

//         DynamicJsonBuffer jsonBuffer;
//         JsonObject& root = jsonBuffer.createObject();
//         _wifiScan(jsonBuffer, root);
//         root.printTo(wifiBuffer);
//         Serial.println(wifiBuffer);
//         DEBUG_MSG_P(PSTR("Done\n"));
//     }); */

//     settingsRegisterCommand(F("TESTDIMMER"), [](Embedis* e) {
//         dimmerProviderStatus(String(e->argv[1]).toInt(), String(e->argv[2]).toInt()); 
//         DEBUG_MSG_P(PSTR("Done\n"));
//     });
//     settingsRegisterCommand(F("MQTTRESET"), [](Embedis* e) {
//         mqttConfigure();
//         mqttDisconnect();
//         DEBUG_MSG_P(PSTR("Done\n"));
//     });
//     settingsRegisterCommand(F("GETHEAP"), [](Embedis* e) {
//         DEBUG_MSG_P(PSTR("Free HEAP: %d bytes\n"), ESP.getFreeHeap());
//         DEBUG_MSG_P(PSTR("Done\n"));
//     });

//     settingsRegisterCommand(F("HEAP"), [](Embedis* e) {
//         infoMemory("Heap", getInitialFreeHeap(), getFreeHeap());
//         DEBUG_MSG_P(PSTR("+OK\n"));
//     });

//     settingsRegisterCommand(F("STACK"), [](Embedis* e) {
//         DEBUG_MSG_P(PSTR("Initial Stack : %d \n"), getInitialFreeStack());
//         infoMemory("Stack", 4096, getFreeStack());
//         DEBUG_MSG_P(PSTR("+OK\n"));
//     });

//     settingsRegisterCommand(F("TEST_AFTER_CONFIG"), [](Embedis* e) {
//         test_after_config_parse_callbacks();
//         DEBUG_MSG_P(PSTR("+OK\n"));
//     });

//     settingsRegisterCommand(F("HELP"), [](Embedis* e) {
//         _settingsHelpCommand();
//         DEBUG_MSG_P(PSTR("Done\n"));
//     });

//     settingsRegisterCommand(F("GETKEYS"), [](Embedis* e) {
//         _settingsKeysCommand();
//         DEBUG_MSG_P(PSTR("Done\n"));
//     });
//     settingsRegisterCommand(F("EEPROMINF"), [](Embedis* e) {
//         unsigned long freeEEPROM = SPI_FLASH_SEC_SIZE - settingsSize();
//         DEBUG_MSG_P(PSTR("Number of keys: %d\n"), _settingsKeyCount());
//         DEBUG_MSG_P(PSTR("Free EEPROM: %d bytes (%d%%)\n"), freeEEPROM, 100 * freeEEPROM / SPI_FLASH_SEC_SIZE);
//         eepromSectorsDebug();
//         DEBUG_MSG_P(PSTR("Done\n"));
//     });

//     settingsRegisterCommand(F("DUMP"), [](Embedis* e) {
//         unsigned int size = _settingsKeyCount();
//         for (unsigned int i = 0; i < size; i++) {
//             String key = settingsKeyName(i);
//             String value = getSetting(key);
//             DEBUG_MSG_P(PSTR("+%s => %s\n"), key.c_str(), value.c_str());
//         }
//         DEBUG_MSG_P(PSTR("Done\n"));
//     });

//     settingsRegisterCommand(F("TEST.EEPROM"), [](Embedis* e) {
//         DEBUG_MSG_P(PSTR("[DEBUG] Counts : %d \n"), String(e->argv[1]).toInt());
//         // testEEPROM(String(e->argv[1]).toInt());
//         // testEEPROM_TWO(String(e->argv[1]).toFloat());
//         DEBUG_MSG_P(PSTR("\n"));
//         DEBUG_MSG_P(PSTR("Done\n"));
//     });

//     settingsRegisterCommand(F("EEPROM.COMMIT"), [](Embedis* e) {
//         const bool res = EEPROMr.commit();
//         if (res) {
//             DEBUG_MSG_P(PSTR("+OK\n"));
//         } else {
//             DEBUG_MSG_P(PSTR("-ERROR\n"));
//         }
//     });
//     settingsRegisterCommand(F("SETLED"), [](Embedis* e) {
//         uint8_t ledPattern = String(e->argv[1]).toInt();
//         setLedPattern(ledPattern);
//     });
//     settingsRegisterCommand(F("TEST"), [](Embedis* e) {
//         // wifiReconnectCheck(200);
//         // _httpGetRaw(String("Something"));
//         // setLedPattern(6);
//         // DEBUG_MSG_P(PSTR("Done\n"));
//         String someData = "a";
//         for (int i = 0; i < 19; i++) {
//             someData = someData + "a";
//             char message[70] = "";
//             sprintf(message, "{\"uptime\":\"1958\",\"freeheap\":\"28032\",\"load\":\"%s\"}", someData.c_str());
//             // const char * message = String(e->argv[1]).c_str();

//             Serial.print(message);
//             Serial.print(",");
//             Serial.print(strlen(message));
//             Serial.print(",");
//             Serial.print(getCipherlength(strlen(message)));
//             Serial.println("");

//             char data_encoded[getBase64Len(getCipherlength(strlen(message)) + 1)];
//             memset(data_encoded, '\0', sizeof(data_encoded));

//             // DEBUG_MSG_P(PSTR("[DEBUG] Encoded Message Size : %d \n"),sizeof(data_encoded));

//             char iv_encoded[30] = "";
//             // char device_id[14] = "";
//             // char device_id_b64[21] = "";
//             // DEBUG_MSG_P(PSTR("Sending Message \n"));
//             // Serial.print("Message to Encrypt: ");
//             // Serial.println(message);

//             // strcpy(device_id, getSetting("identifier").c_str());
//             // base64_encode(device_id_b64, device_id, strlen(device_id));

//             // Serial.println(sizeof(data_encoded)+sizeof(iv_encoded)+sizeof(device_id_b64)+35);
//             encryptMsg(message, data_encoded, iv_encoded);
//             Serial.print("IV b64: ");
//             Serial.println(iv_encoded);
//             Serial.print(strlen(data_encoded));
//             Serial.print(",");
//             Serial.println(data_encoded);

//             Serial.println("Now sending Message");
//             mqttSend(MQTT_TOPIC_BEAT, message);
//             Serial.println("==========================================");
//             Serial.println("==========================================");
//         }
//     });

// #endif

// #if DEBUG_SUPPORT
// #if SAVE_CRASH_INFO
//     settingsRegisterCommand(F("CRASHDUMP"), [](Embedis* e) {
//         crashDump();
//         DEBUG_MSG_P(PSTR("Done\n"));
//     });
// #endif  // SAVE_CRASH_INFO
// #endif  // DEBUG_SUPPORT

    settingsRegisterCommand(F("KIOTSTDUMP"), [](Embedis* e) {
        unsigned int size = _settingsKeyCount();
        for (unsigned int i = 0; i < size; i++) {
            String key = settingsKeyName(i);
            String value = getSetting(key);
            SERIAL_SEND_P(PSTR("+%s => %s\n"), key.c_str(), value.c_str());
        }
        SERIAL_SEND_P(PSTR("Done\n"));
    });

    settingsRegisterCommand(F("KIOTMQTTFETCH"), [](Embedis* e) {
        setFetchMqttConf(true);
        SERIAL_SEND_P(PSTR("Done\n"));
    });    

//     settingsRegisterCommand(F("EEPROMDUMPRAW"), [](Embedis* e) {
//         bool ascii = false;
//         if (e->argc == 2) ascii = String(e->argv[1]).toInt() == 1;
//         for (unsigned int i = 0; i < SPI_FLASH_SEC_SIZE; i++) {
//             if (i % 16 == 0) SERIAL_SEND("\n[%04X] ", i);
//             byte c = EEPROMr.read(i);
//             if (ascii && 32 <= c && c <= 126) {
//                 SERIAL_SEND(" %c ", c);
//             } else {
//                 SERIAL_SEND("%02X ", c);
//             }
//         }
//         SERIAL_SEND("Done\n");
//     });

//     settingsRegisterCommand(F("EEPROMDUMPRAWALL"), [](Embedis* e) {
//         bool ascii = false;
//         if (e->argc == 2) ascii = String(e->argv[1]).toInt() == 1;
//         ESP.wdtDisable();
//         for (int k = 0; k < EEPROMr.size(); k++) {
//             uint32_t currentSector = EEPROMr.current();
//             DEBUG_MSG_P(PSTR("[EEPROM] Current Sector : %d \n"), currentSector);
//             for (unsigned int i = 0; i < SPI_FLASH_SEC_SIZE; i++) {
//                 if (i % 16 == 0) SERIAL_SEND("\n[%04X] ", i);
//                 byte c = EEPROMr.read(i);
//                 if (ascii && 32 <= c && c <= 126) {
//                     SERIAL_SEND(" %c ", c);
//                 } else {
//                     SERIAL_SEND("%02X ", c);
//                 }
//             }
//             // EEPROMr.write(EEPROM_TEMP_ADDRESS, k);
//             EEPROMr.commit();
//             DEBUG_MSG_P(PSTR("\n\n"));
//         }
//         ESP.wdtFeed();
//         ESP.wdtEnable(6000);

//         SERIAL_SEND("Done\n");
//     });

//     settingsRegisterCommand(F("DEVICEINFO"), [](Embedis* e) {
//         info();
//         wifiStatus();
//         DEBUG_MSG_P(PSTR("Done\n"));

// #if DEVELOPEMEMT_MODE == 0 || DEVELOPEMEMT_MODE == 2
//         infoProduction();
//         Serial.println("Done");
// #endif
//     });

//     /* settingsRegisterCommand(F("DEVUPTIME"), [](Embedis* e) {
//         DEBUG_MSG_P(PSTR("Uptime: %d seconds\n"), getUptime());
//         DEBUG_MSG_P(PSTR("Done\n"));
//     }); */

//     settingsRegisterCommand(F("RESTART"), [](Embedis* e) {
//         DEBUG_MSG_P(PSTR("Done\n"));
//         deferredReset(100, CUSTOM_RESET_TERMINAL);
//     });

//     settingsRegisterCommand(F("DELCONFIG"), [](Embedis* e) {
//         DEBUG_MSG_P(PSTR("Done\n"));
//         resetReason(CUSTOM_RESET_TERMINAL);
//         ESP.eraseConfig();
//         *((int*)0) = 0;  // see https://github.com/esp8266/Arduino/issues/1494
//     });

    settingsRegisterCommand(F("FACTORYSET"), [](Embedis* e) {
        settingsFactoryReset();
        DEBUG_MSG_P(PSTR("Done\n"));
    });

//     settingsRegisterCommand(F("EEPROMERASEALL"), [](Embedis* e) {
//         // settingsFactoryReset();
//         // DEBUG_MSG_P(PSTR("Done\n"));
//         _eepromRraseAll();
//         // DEBUG_MSG_P(PSTR("Done\n"));
//         Serial.println("Done");
//     });

//     settingsRegisterCommand(F("HOTSPOT"), [](Embedis* e) {
//         // Turn the device to hotspot mode
//         DEBUG_MSG_P(PSTR("Done\n"));
//         // forgetNetwork();
//         // createAP();
//         createLongAP();
//     });

//     settingsRegisterCommand(F("UPDATE_COMMAND"), [](Embedis* e) {
//         // Turn the device to hotspot mode
//         DEBUG_MSG_P(PSTR("UPDATE COMMAND Start\n"));
//         unsigned long i = 0;
//         while(1){
//             // Do nothing
//             i++;
//             if(i > 2400){
//                 i = 0;
//             }
//         }
//         DEBUG_MSG_P(PSTR("UPDATE COMMAND Done\n"));
//         // forgetNetwork();
//         // createAP();
//         // createLongAP();
//     });
    

// #ifndef NO_RELAYS
//     settingsRegisterCommand(F("RELAY"), [](Embedis* e) {
//         if (e->argc < 2) {
//             return e->response(Embedis::ARGS_ERROR);
//         }
//         int id = String(e->argv[1]).toInt();
//         if (e->argc > 2) {
//             int value = String(e->argv[2]).toInt();
//             if (value == 2) {
//                 relayToggle(id);
//             } else {
//                 relayStatus(id, value == 1);
//             }
//         }
//         DEBUG_MSG_P(PSTR("Status: %s\n"), relayStatus(id) ? "true" : "false");
//         DEBUG_MSG_P(PSTR("Done\n"));
//     });
//     settingsRegisterCommand(F("DIMMER"), [](Embedis* e) {
//         if (e->argc < 2) {
//             return e->response(Embedis::ARGS_ERROR);
//         }
//         int id = String(e->argv[1]).toInt();
//         if (e->argc > 2) {
//             int value = String(e->argv[2]).toInt();
//             relayStatus(id, value, false, true, false);
//         }
//         DEBUG_MSG_P(PSTR("Done\n"));
//     });

//     settingsRegisterCommand(F("RELAY_UPDATE"), [](Embedis* e) {
//         if (e->argc < 3) {
//             return e->response(Embedis::ARGS_ERROR);
//         }
//         int id = String(e->argv[1]).toInt();
//         if (e->argc > 2) {
//             int value = String(e->argv[2]).toInt();
//             relayUpdate((unsigned char)id, (unsigned char)value);
//             relayMQTT((unsigned char)id);
// #if MQTT_REPORT_EVENT_TO_HOME
//             activeHomePong(true);
// #endif
//         }
//         DEBUG_MSG_P(PSTR("Status: %s\n"), relayStatus(id) ? "true" : "false");
//         DEBUG_MSG_P(PSTR("Done\n"));
//     });

//     settingsRegisterCommand(F("DIM_UPDATE"), [](Embedis* e) {
//         if (e->argc < 2) {
//             return e->response(Embedis::ARGS_ERROR);
//         }
//         int dim_val = String(e->argv[1]).toInt();
//         dimmerUpdate(1,dim_val);
//         // dimmerMQTT accpets id right now but its only dummy as of now.
//         dimmerMQTT(1);

// #if MQTT_REPORT_EVENT_TO_HOME
//         activeHomePong(true);
// #endif
//     });

//     settingsRegisterCommand(F("RELAY_STATUS"), [](Embedis* e) {
//         if (e->argc < 2) {
//             // DO nothing.
//             return;
//         }
//         int status = String(e->argv[1]).toInt();
//         relayAllUpdate((uint8_t)status);
//         if (e->argc > 2) {
//             dimmerUpdate(1,String(e->argv[2]).toInt());
//         }
//     });
// #endif

//     /*
//     settingsRegisterCommand( F("MOTION_UPDATE"), [](Embedis* e) {
//         if (e->argc < 2) {
//             return e->response(Embedis::ARGS_ERROR);
//         }
//         // Serial.println(String(e->argv[1]));
//         // sensorsMQTT(MQTT_TOPIC_TEMP,e->argv[1],e->argv[2]);
//         // if(e->argv[1]) {}
//         int value = String(e->argv[1]).toInt();
//         if(value == 1 ){
//             sensorsAction(SENS_NAME_PIR,"1", false);
//         }else{
//             sensorsAction(SENS_NAME_PIR,"0", false);
//         }
//         DEBUG_MSG_P(PSTR("Motion Update :%s \n"), e->argv[1]);
//         DEBUG_MSG_P(PSTR("Done\n"));
//     });
//     */

//     /* 
//     settingsRegisterCommand( F("DIGI_UPDATE"), [](Embedis* e) {
//         if (e->argc < 2) {
//             return e->response(Embedis::ARGS_ERROR);
//         }
//         // Serial.println(String(e->argv[1]));
//         // sensorsMQTT(MQTT_TOPIC_TEMP,e->argv[1],e->argv[2]);
//         bool reportNow = false;
//         if(e->argc==4){
//             reportNow = true;
//         }
//         sensorsAction(SENS_NAME_DIGI,e->argv[1], reportNow);
//         DEBUG_MSG_P(PSTR("Digital Update :%s \n"), e->argv[1]);
//         DEBUG_MSG_P(PSTR("Done\n"));
//     });
//      */
//     settingsRegisterCommand(F("SEND_CONTROLLER_SETTINGS"), [](Embedis* e) {
// #if ENABLE_CONTROLLER_SETTINGS
//         sendControllerSettings();
// #endif

//         very_nice_delay(50000);
// #if LED_PROVIDER == ANOTHER_BOARD
//         sendLEDPattern();
// #endif

//         DEBUG_MSG_P(PSTR("Done\n"));
//     });
// #if HAVE_SIBLING_CONTROLLER
//     settingsRegisterCommand(F("ATMV"), [](Embedis* e) {
//         if (e->argc < 3) {
//             return;
//         }
//         // Introducing the end characters now to ensure proper data for important params.
//         if (String(e->argv[2]).equals("E")) {
//             String version = String(e->argv[1]);
//             setAtmVersion(version);
//         } else {
//             // Data is not proper. Characters missing.
//             return;
//         }
//     });
// #endif
// #if POWER_PROVIDER == POWER_PROVIDER_HLW8012
//     settingsRegisterCommand(F("HLW_CALIB"), [](Embedis* e) {
//         if (e->argc < 4) {
//             return e->response(Embedis::ARGS_ERROR);
//         }
//         double value;
//         value = String(e->argv[1]).toFloat();  // Current Expected
//         setSetting("pwrEC", value);

//         value = String(e->argv[2]).toInt();  // Voltage Expected
//         setSetting("pwrExpectedV", value);

//         value = String(e->argv[3]).toInt();  // Power Expected
//         setSetting("pwrExpectedP", value);
//         _powerConfigureProvider();
//         // saveSettings();
//         Serial.println("Done ! Calibaration Success");

//         double power = _powerActivePower();
//         double voltage = _powerVoltage();
//         double current = _powerCurrent();

//         Serial.print("Current Power - ");
//         Serial.print(power);
//         Serial.print(" , Voltage - ");
//         Serial.print(voltage);
//         Serial.print(" , Current - ");
//         Serial.print(current);
//     });

//     settingsRegisterCommand("LIVEPOWER", [](Embedis* e) {
//         double power = _powerActivePower();
//         double voltage = _powerVoltage();
//         double current = _powerCurrent();

//         Serial.print("Current Power - ");
//         Serial.print(power);
//         Serial.print(" , Voltage - ");
//         Serial.print(voltage);
//         Serial.print(" , Current - ");
//         Serial.print(current);
//     });
// #endif

// // -----------------------------------------------------------------------------
// // Overriding Default Embedis Commands
// // -----------------------------------------------------------------------------
// #if DEVELOPEMEMT_MODE == 0

//     settingsRegisterCommand(F("COMMANDS"), DEFUALT_OVERIDE_EMBEDIS);
//     settingsRegisterCommand(F("PUBLISH"), DEFUALT_OVERIDE_EMBEDIS);
//     settingsRegisterCommand(F("SUBSCRIBE"), DEFUALT_OVERIDE_EMBEDIS);
//     settingsRegisterCommand(F("UNSUBSCRIBE"), DEFUALT_OVERIDE_EMBEDIS);
//     settingsRegisterCommand(F("DICTIONARIES"), DEFUALT_OVERIDE_EMBEDIS);
//     settingsRegisterCommand(F("GET"), DEFUALT_OVERIDE_EMBEDIS);
//     settingsRegisterCommand(F("SET"), DEFUALT_OVERIDE_EMBEDIS);
//     settingsRegisterCommand(F("DEL"), DEFUALT_OVERIDE_EMBEDIS);
//     settingsRegisterCommand(F("KEYS"), DEFUALT_OVERIDE_EMBEDIS);
//     settingsRegisterCommand(F("SELECT"), DEFUALT_OVERIDE_EMBEDIS);

//     // Adding Custom Command Names for same things.

// #endif

    settingsRegisterCommand(F("KIOTST"), KiotCustomSet);
    settingsRegisterCommand(F("KIOTDT"), KiotCustomDel);
    settingsRegisterCommand(F("KIOTGT"), KiotCustomGet);
}

void DEFUALT_OVERIDE_EMBEDIS(Embedis* e) {
    DEBUG_MSG_P(PSTR("Unknown Command \n"));
}

void KiotCustomSet(Embedis* e) {
    if (e->argc != 3) {
        Serial.println("Invalid Argumants");
        return;
    }

    int key_len = e->argv[2] - e->argv[1] - 1;
    int value_len = e->argv[3] - e->argv[2] - 1;
    setSetting(rightString(e->argv[1], key_len), rightString(e->argv[2], value_len));
    saveSettings();
    return;
}
void KiotCustomGet(Embedis* e) {
    if (e->argc != 2) {
        Serial.println("Invalid Argumants");
        return;
    }
    Serial.println(getSetting(String(e->argv[1])));
    return;
}
void KiotCustomDel(Embedis* e) {
    if (e->argc != 2) {
        Serial.println("Invalid Argumants");
        return;
    }
    delSetting(String(e->argv[1]));
    saveSettings();
    Serial.println("Done");
    return;
}
void moveSetting(const String from, const String to) {
    String value = getSetting(from);
    if (value.length() > 0) setSetting(to, value);
    delSetting(from);
}

String getSetting(const String& key) {
    return getSetting(key, "");
}

bool delSetting(const String& key) {
    return Embedis::del(key);
}

bool delSetting(const String& key, unsigned int index) {
    return delSetting(key + String(index));
}

bool hasSetting(const String& key) {
    return getSetting(key).length() != 0;
}

bool hasSetting(const String& key, unsigned int index) {
    return getSetting(key, index, "").length() != 0;
}

void saveSettings() {
    DEBUG_MSG_P(PSTR("[SETTINGS] Setting setting_save_flag\n"));
#if not AUTO_SAVE
    // EEPROM.commit();
    _settings_save = true;
#endif
}

void settingsRegisterCommand(const String& name, void (*call)(Embedis*)) {
    Embedis::command(name, call);
};

// void settingsSetupMQTT() { // TODO_S1
//     mqttRegister(settingsMQTTCallback);
// }

// void settingsMQTTCallback(unsigned int type, const char* topic, const char* payload, bool retained, bool isEncrypted) { // TODO_S1
//     if (type == MQTT_CONNECT_EVENT) {
//         char buffer[strlen(MQTT_TOPIC_CONFIG) + 3];
//         sprintf(buffer, "%s", MQTT_TOPIC_CONFIG);
//         mqttSubscribe(buffer, MQTT_QOS_SETTINGS);
//     }
//     if (type == MQTT_MESSAGE_EVENT) {
//         if (retained && !MQTT_HANDLE_RET_SETTINGS) {
//             return;
//         }
//         // Match topic
//         String t = mqttSubtopic((char*)topic);
//         if (!t.startsWith(MQTT_TOPIC_CONFIG)) return;

//         if (MQTT_MANDATE_ENC_SETTINGS && !isEncrypted) {
//             DEBUG_MSG_P(PSTR("[CONFIG] Required Encrypted payload. Got unencrypted."));
//             return;
//         }

//         DynamicJsonBuffer jsonBuffer(256);
//         JsonObject& root = jsonBuffer.parseObject(reinterpret_cast<const char*>(payload));
//         if (!root.success()) {
//             DEBUG_MSG_P(PSTR("[ERROR] parseObject() failed for MQTT"));
//             Serial.println(reinterpret_cast<const char*>(payload));
//             return;
//         }
//         if (root.containsKey("config") || root.containsKey("action")) {
//             DEBUG_MSG_P(PSTR("[DEBUG] CONFIG message recieved \n"));
//             #if DEBUG_KU_CONTROLLER
//                 char messageBuffer[250] = "";
//                 root.printTo(messageBuffer, sizeof(messageBuffer));
//                 DEBUG_MSG_P(PSTR("[DEBUG] : %s \n"), String(messageBuffer).c_str());
//             #endif
//             if (root.containsKey("config") && root["config"].is<JsonObject&>()) {
//                 // JsonArray& config = root["config"];
//                 JsonObject& config = root["config"];
//                 char buffer[50] = "";
//                 processConfig(config, "mqtt", buffer, 50);
//                 _sendSettings = true;
//                 reportEventToHome(String(EVENT_CONFIG_RCVD), "1");
//             } else if (root.containsKey("action")) {
//                 String action = root["action"];
//                 if (root.containsKey("meta")) {
//                     JsonObject& meta = root["meta"];
//                     _processAction(action, meta);
//                 } else {
//                     _processAction(action);
//                 }
//             }
//         }

//         // if(q==NULL){
//         //     return ;
//         // }
//         // DEBUG_MSG_P(PSTR("[DEBUG] MQTT Question Type: %s\n"),q);
//         // // Send a message on serial, it'll report to mqtt when recieving update from serial
//         // if(strcmp(q,MQTT_TOPIC_TEMP) == 0){
//         //     report_sensor(MQTT_TOPIC_TEMP);

//         // }
//     }
// }

// bool settingsMQTT() {
//     if (!canSendMqttMessage()) {
//         return false;
//     }
//     DynamicJsonBuffer jsonBuffer(1024);
//     bool sent = true;
    
//     #if LOW_POWER_OPTIMIZATION != 1
//         JsonObject& root = jsonBuffer.createObject();
//         root["sv"] = SETTINGS_VERSION;
//         root["ssv"] = SENSORS_VERSION;
//         String output;
//         root.printTo(output);
//         String mqttTopic = String(MQTT_TOPIC_CONFIG) + "/" + String(MQTT_TOPIC_INFO);
//         sent = sent && mqttSend(mqttTopic.c_str(), output.c_str());
//         if (!sent) {
//             return false;
//         }
//         delay(100);
//     #endif

//     for (int i = 0; i < set_config_keys_callbacks.size(); i++) {
//         JsonObject& root = jsonBuffer.createObject();

//         char topic[MAX_CONFIG_TOPIC_SIZE] = "";
//         (set_config_keys_callbacks[i])(jsonBuffer, root, topic, false);
//         String output;
//         root.printTo(output);
//         String mqttTopic = String(MQTT_TOPIC_CONFIG) + "/" + String(topic);
//         sent = sent && mqttSend(mqttTopic.c_str(), output.c_str());
//         // To stop this loop here Only.
//         // Untill We find a solution to send only specific config to server.
//         if (!sent) {
//             return false;
//         }
//         // I do not know why - but just intuition wise keeping 100ms delay.
//         delay(100);
//     }
//     return sent;
// }

// bool settingsHTTP() {
//     if (!wifiConnected() || (WiFi.getMode() != WIFI_STA)) return false;

//     DynamicJsonBuffer jsonBuffer(1000);
//     JsonObject& root = jsonBuffer.createObject();
//     root["app"] = APP_NAME;
//     root["version"] = APP_VERSION;
//     root["rev"] = APP_REVISION;
//     root["network"] = getNetwork();
//     root["buildDate"] = __DATE__;
//     bool sent = true;

//     // String output;
//     // root.printTo(output);
//     // String mqttTopic = String(MQTT_TOPIC_CONFIG)+ "/"+String(MQTT_TOPIC_INFO);

//     // delay(100);

//     for (int i = 0; i < set_config_keys_callbacks.size(); i++) {
//         JsonObject& container = jsonBuffer.createObject();

//         char topic[MAX_CONFIG_TOPIC_SIZE] = "";
//         (set_config_keys_callbacks[i])(jsonBuffer, container, topic, false);
//         // String output;
//         // root.printTo(output);
//         // String mqttTopic = String(MQTT_TOPIC_CONFIG)+ "/"+String(topic);
//         // sent = sent && mqttSend(mqttTopic.c_str(), output.c_str());
//         // I do not know why - but just intuition wise keeping 100ms delay.
//         // delay(100);
//         root[String(topic)] = container;
//     }

//     String output;
//     root.printTo(output);
//     // Serial.println(output);

//     //  httpEnqueue("config", output.c_str());
//     for (int i = 0; i < 3; i++) {
//         mqttSend("config", output.c_str(), false, true);
//     }
//     return sent;
// }

void migrateSettings() {
    uint8_t sv = getSetting("sv", 1).toInt();
    uint8_t ssv = getSetting("ssv", 1).toInt();
    bool changed = false;

    if (sv == (uint8_t)SETTINGS_VERSION && ssv == (uint8_t)SENSORS_VERSION) return;
    DEBUG_MSG_P(PSTR("[DEBUG] Version Changed \n"));

    if (SETTINGS_VERSION > sv) {
        if (SETTINGS_VERSION == 2) {
            moveSetting("nofussInterval", "otaI");
            moveSetting("nofussServer", "otaS");
            moveSetting("nofussEnabled", "otaE");

            moveSetting("pwrReportEvery", "pwReE");
            moveSetting("apiRealTime", "RTV");
            moveSetting("pwrReadEvery", "pwREv");
            moveSetting("pwrRatioV", "pwRV");
            moveSetting("pwrRatioC", "pwRC");
            moveSetting("pwrRatioP", "pwRP");
            moveSetting("pwrExpectedC", "pwrEC");

            moveSetting("ntpOffset", "ntpOS");
            moveSetting("ntpVisible", "ntpV");
            moveSetting("ntpServer", "ntpS");

            moveSetting("sensor_reporting_time", "srt");
            moveSetting("sens_topic_temp", "SnTT");
            moveSetting("sens_slot1_pre", "SnS1P");
            moveSetting("temp_min_change", "SnTMC");
            moveSetting("temp_sens_type", "SnTST");
            moveSetting("mc_temp_upd_int", "SnMTUI");
            moveSetting("temp_read_interval", "SnTRI");
        }

        setSetting("sv", SETTINGS_VERSION);
        changed = true;
    }
    if ((uint8_t)SENSORS_VERSION > ssv) {
        setSetting("ssv", (uint8_t)SENSORS_VERSION);
        changed = true;
    }

    if (changed) {
        saveSettings();
    }
}

void copyKiotSettings() {
    String copySetting = "";
    copySetting = getSetting("ssid0", "");
    getStringFromJson(clientSSID, copySetting.c_str(), 33);
    copySetting = getSetting("pass0", "");
    getStringFromJson(clientPass, copySetting.c_str(), 65);
    encryptSetup();
    copySetting = getSetting("mqttUser", "");
    getStringFromJson(mqttUser, copySetting.c_str(), 33);
    getStringFromJson(mqttClientID, copySetting.c_str(), 33);
    copySetting = getSetting("mqttPassword", "");
    getStringFromJson(mqttPass, copySetting.c_str(), 33);
    copySetting = getSetting("mqttPort", "");
    mqttPort = copySetting.toInt();
    copySetting = getSetting("mqttServer", "");
    getStringFromJson(mqttServer, copySetting.c_str(), 33);
}

// void sendSwitchMode(){
//     #if KU_SERIAL_SUPPORT

//     #else
//     String mode = getSetting(SWITCH_MODE, SWITCH_MANUAL_OVERRIDE_DISABLED);
//     SERIAL_SEND_P(PSTR("%s %s %s\n"),COMMAND_CONFIG, SWITCH_MODE, mode.c_str());
//     #endif
// }

// Disabling genaration of key in device.
/*

void setSecuritySettings(){
    String identifier = getIdentifier();
    // DEBUG_MSG_P(PSTR("[DEBUG] IDENTIFIER : %s"), identifier.c_str());
    // DEBUG_MSG_P(PSTR("[DEBUG] Key : %s"),HMAC_SECRET_DEVICE_KEYS);
    char security_hash[257]="";
    hmacHash(identifier.c_str(), security_hash,String(HMAC_SECRET_DEVICE_KEYS).c_str(),HMAC_KEY_LENGTH);
    // DEBUG_MSG_P(PSTR("[DEBUG] Hash : %s"), &security_hash[0]);
    char aes_key[33] = "";
    char api_key[21] = "";
    char api_secret[21] = "";
    subString(security_hash,aes_key,0,32);
    subString(security_hash,api_key,20,40);
    subString(security_hash,api_secret,40,60);
    setSetting("aesKey",aes_key);
    setSetting("apiKey",api_key);
    setSetting("apiSecret",api_secret);
    
}
*/