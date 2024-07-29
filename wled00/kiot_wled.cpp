#include "wled.h"

void kiotWledSetup() {
    loopRegister(kiotWledLoop);
    kiotWledInitCommands();
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
}

void kiotWledInitCommands() {
    // setConfigKeys(_kiot_config_keys);
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

    settingsRegisterCommand(F("HEARTBEAT"), [](Embedis* e) {
        schedule_heartbeat = millis();
        DEBUG_MSG_P(PSTR("Done\n"));
    });      

    settingsRegisterCommand(F("MODENAMES"), [](Embedis* e) {
        StaticJsonDocument<512> doc;
        JsonArray arr = doc.to<JsonArray>();
        serializeModeNames(arr);
        serializeJson(doc, Serial);
        DEBUG_MSG_P(PSTR("Done\n"));
    });

    settingsRegisterCommand(F("MODEDATA"), [](Embedis* e) {
        StaticJsonDocument<512> doc;
        JsonArray arr = doc.to<JsonArray>();
        serializeModeData(arr);
        serializeJson(doc, Serial);
        DEBUG_MSG_P(PSTR("Done\n"));
    });

    settingsRegisterCommand(F("NODES"), [](Embedis* e) {
        StaticJsonDocument<512> doc;
        JsonObject root = doc.to<JsonObject>();
        serializeNodes(root);
        serializeJson(doc, Serial);
        DEBUG_MSG_P(PSTR("Done\n"));
    });

    settingsRegisterCommand(F("NETWORKS"), [](Embedis* e) {
        StaticJsonDocument<1024> doc;
        JsonObject root = doc.to<JsonObject>();
        serializeNetworks(root);
        serializeJson(doc, Serial);
        DEBUG_MSG_P(PSTR("Done\n"));
    });


    settingsRegisterCommand(F("INFO"), [](Embedis* e) {
        StaticJsonDocument<1024> doc;
        JsonObject root = doc.to<JsonObject>();
        serializeInfo(root);
        serializeJson(doc, Serial);
        DEBUG_MSG_P(PSTR("Done\n"));
    });

    settingsRegisterCommand(F("STATE"), [](Embedis* e) {
        StaticJsonDocument<1024> doc;
        JsonObject root = doc.to<JsonObject>();
        serializeState(root);
        serializeJson(doc, Serial);
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
        StaticJsonDocument<1024> _doc_;
        DeserializationError error = deserializeJson(_doc_, reinterpret_cast<const char*>(payload));
        JsonObject root = _doc_.as<JsonObject>();
        if (error || root.isNull()) {
            DEBUG_MSG_P(PSTR("[ERROR] parseObject() failed for MQTT OR NULL \n"));
            DEBUG_MSG_P(PSTR("[ERROR] DeserializationError %d \n"), error);
            Serial.println(reinterpret_cast<const char*>(payload));
            return;
        }
        DEBUG_MSG_P(PSTR("[SIZE] DeserializationSize %d \n"), root.size());
        serializeJson(root, Serial); //debug TODO_S1 remove it after testing
        DEBUG_MSG_P(PSTR(" \n"));

      if(root.containsKey("state")) {
        JsonObject state = root["state"];
        
        //  JsonObject root = doc.to<JsonObject>();

        bool verboseResponse = false;
        // TODO_S2: refactor and check if the below conditions is needed
        if (state["v"] && state.size() == 1) {
          //if the received value is just "{"v":true}", send only to this client
          verboseResponse = true;
        } else if (state.containsKey("lv")) {
        //   wsLiveClientId = root["lv"] ? client->id() : 0;
        } else {
          DEBUG_MSG_P(PSTR("Message received successfully and taking action \n"));  
          verboseResponse = deserializeState(state);
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

      } else if(root.containsKey("set")){
        DEBUG_MSG_P(PSTR("root[set]   Message received successfully and taking action \n"));
        JsonObject set = root["set"];
        DEBUG_MSG_P(PSTR("[SIZE] DeserializationSize %d \n"), set.size());
        kiotHandleSettingsSet(set, SUBPAGE_LEDS);
      }

    }
}

void kiotWledPing(JsonObject &root) {
  JsonObject state = root.createNestedObject("state");
  serializeState(state);
  JsonObject info  = root.createNestedObject("info");
  // enabling this causes a crash due to stack overflow TODO_S1 even with 2048
  // serializeInfo(info);
}

void kiotHandleSettingsSet(JsonObject root, byte subPage)
{
  if (subPage == SUBPAGE_PINREQ) // TODO_S1: check if this is needed
  {
    const char* pin = root[F("PIN")];
    checkSettingsPIN(pin);
    return;
  }

  //0: menu 1: wifi 2: leds 3: ui 4: sync 5: time 6: sec 7: DMX 8: usermods 9: N/A 10: 2D
  if (subPage < 1 || subPage > 10 || !correctPIN) return;

  //WIFI SETTINGS TODO_S1: check if this is needed
  // if (subPage == SUBPAGE_WIFI)
  // {
  //   strlcpy(clientSSID, root[F("CS")], 33);

  //   if (!isAsterisksOnly(root[F("CP")], 65)) strlcpy(clientPass, root[F("CP")], 65);

  //   strlcpy(cmDNS, root[F("CM")], 33);

  //   apBehavior = root[F("AB")].as<int>();
  //   strlcpy(apSSID, root[F("AS")], 33);
  //   apHide = root.containsKey("set");
  //   int passlen = strlen(root[F("AP")]);
  //   if (passlen == 0 || (passlen > 7 && !isAsterisksOnly(root[F("AP")], 65))) strlcpy(apPass, root[F("AP")], 65);
  //   int t = root[F("AC")]; if (t > 0 && t < 14) apChannel = t;

  //   force802_3g =  root.containsKey("FG");
  //   noWifiSleep =  root.containsKey("WS");

  //   #ifndef WLED_DISABLE_ESPNOW
  //   enable_espnow_remote = root.containsKey("RE");
  //   strlcpy(linked_remote,root[F("RMAC")], 13);

  //   //Normalize MAC format to lowercase
  //   strlcpy(linked_remote,strlwr(linked_remote), 13);
  //   #endif

  //   #ifdef WLED_USE_ETHERNET
  //   ethernetType = root[F("ETH")];
  //   WLED::instance().initEthernet();
  //   #endif

  //   char k[3]; k[2] = 0;
  //   for (int i = 0; i<4; i++)
  //   {
  //     k[1] = i+48;//ascii 0,1,2,3

  //     k[0] = 'I'; //static IP
  //     staticIP[i] = root[k];

  //     k[0] = 'G'; //gateway
  //     staticGateway[i] = root[k];

  //     k[0] = 'S'; //subnet
  //     staticSubnet[i] = root[k];
  //   }
  // }

  //LED SETTINGS
  if (subPage == SUBPAGE_LEDS)
  {
    int t = 0;

    if (rlyPin>=0 && pinManager.isPinAllocated(rlyPin, PinOwner::Relay)) {
       pinManager.deallocatePin(rlyPin, PinOwner::Relay);
    }
    if (irPin>=0 && pinManager.isPinAllocated(irPin, PinOwner::IR)) {
       pinManager.deallocatePin(irPin, PinOwner::IR);
    }
    for (uint8_t s=0; s<WLED_MAX_BUTTONS; s++) {
      if (btnPin[s]>=0 && pinManager.isPinAllocated(btnPin[s], PinOwner::Button)) {
        pinManager.deallocatePin(btnPin[s], PinOwner::Button);
      }
    }

    uint8_t colorOrder, type, skip, awmode, channelSwap;
    uint16_t length, start;
    uint8_t pins[5] = {255, 255, 255, 255, 255};
    DEBUG_MSG_P(PSTR("[contain] ...................... %d \n"), root.containsKey("MS"));
    autoSegments =  root.containsKey("MS");
    correctWB = root.containsKey("CCT");
    cctFromRgb = root.containsKey("CR");
    DEBUG_MSG_P(PSTR("[CB] ...................... %d \n"), root[F("CB")].as<int>());
    strip.cctBlending = root[F("CB")].as<int>();
    Bus::setCCTBlend(strip.cctBlending);
    DEBUG_MSG_P(PSTR("[AW] ...................... %d \n"), root[F("AW")].as<int>());
    Bus::setGlobalAWMode(root[F("AW")].as<int>());
    DEBUG_MSG_P(PSTR("[FR] ...................... %d \n"), root[F("FR")].as<int>());
    strip.setTargetFps(root[F("FR")].as<int>());
    DEBUG_MSG_P(PSTR("[contain] ...................... %d \n"), root.containsKey("LD"));
    useGlobalLedBuffer = root.containsKey("LD");
    
    JsonObject  ledStipObject = root["stripP"].as<JsonObject>();
    DEBUG_MSG_P(PSTR("[size] ...................... %d \n"), ledStipObject.size());
    bool busesChanged = false;
    for (uint8_t s = 0; s < WLED_MAX_BUSSES+WLED_MIN_VIRTUAL_BUSSES; s++) {
      DEBUG_MSG_P(PSTR("[L00] ...................... %d \n"), ledStipObject[F("L00")].as<int>());
      char lp[4] = "L0"; lp[2] = 48+s; lp[3] = 0; //ascii 0-9 //strip data pin
      char lc[4] = "LC"; lc[2] = 48+s; lc[3] = 0; //strip length
      char co[4] = "CO"; co[2] = 48+s; co[3] = 0; //strip color order
      char lt[4] = "LT"; lt[2] = 48+s; lt[3] = 0; //strip type
      char ls[4] = "LS"; ls[2] = 48+s; ls[3] = 0; //strip start LED
      char cv[4] = "CV"; cv[2] = 48+s; cv[3] = 0; //strip reverse
      char sl[4] = "SL"; sl[2] = 48+s; sl[3] = 0; //skip first N LEDs
      char rf[4] = "RF"; rf[2] = 48+s; rf[3] = 0; //refresh required
      char aw[4] = "AW"; aw[2] = 48+s; aw[3] = 0; //auto white mode
      char wo[4] = "WO"; wo[2] = 48+s; wo[3] = 0; //channel swap
      char sp[4] = "SP"; sp[2] = 48+s; sp[3] = 0; //bus clock speed (DotStar & PWM)
      if (!ledStipObject.containsKey(lp)) {
        DEBUG_PRINT(F("No data for "));
        DEBUG_PRINTLN(s);
        break;
      }
      for (uint8_t i = 0; i < 5; i++) {
        lp[1] = 48+i;
        if (!ledStipObject.containsKey(lp)) break;
        DEBUG_MSG_P(PSTR("[lp] ...................... %d \n"), ledStipObject[lp].as<int>());
        pins[i] = (ledStipObject[lp].as<int>() >= 0) ? ledStipObject[lp].as<int>() : 255;
      }
      DEBUG_MSG_P(PSTR("[type] ...................... %d \n"), ledStipObject[lt].as<int>());
      DEBUG_MSG_P(PSTR("[skip] ...................... %d \n"), ledStipObject[sl].as<int>());
      type = ledStipObject[lt].as<int>();
      skip = ledStipObject[sl].as<int>();
      colorOrder = ledStipObject[co].as<int>();

      start = (ledStipObject.containsKey(ls)) ? ledStipObject[ls].as<int>() : t;
      if (ledStipObject.containsKey(lc) && ledStipObject[lc].as<int>() > 0) {
        t += length = ledStipObject[lc];
      } else {
        break;  // no parameter
      }
      DEBUG_MSG_P(PSTR("[awmode] ...................... %d \n"), ledStipObject[aw].as<int>());
      awmode = ledStipObject[aw].as<int>();
      DEBUG_MSG_P(PSTR("[freqHz] ...................... %d \n"), ledStipObject[sp].as<int>());
      uint16_t freqHz = ledStipObject[sp].as<int>();
      if (type > TYPE_ONOFF && type < 49) {
        switch (freqHz) {
          case 0 : freqHz = WLED_PWM_FREQ/3; break;
          case 1 : freqHz = WLED_PWM_FREQ/2; break;
          default:
          case 2 : freqHz = WLED_PWM_FREQ;   break;
          case 3 : freqHz = WLED_PWM_FREQ*2; break;
          case 4 : freqHz = WLED_PWM_FREQ*3; break;
        }
      } else if (type > 48 && type < 64) {
        switch (freqHz) {
          default:
          case 0 : freqHz =  1000; break;
          case 1 : freqHz =  2000; break;
          case 2 : freqHz =  5000; break;
          case 3 : freqHz = 10000; break;
          case 4 : freqHz = 20000; break;
        }
      } else {
        freqHz = 0;
      }
      DEBUG_MSG_P(PSTR("[channelSwap] ...................... %d \n"), ledStipObject[wo].as<int>());
      channelSwap = Bus::hasWhite(type) ? ledStipObject[wo] : 0;
      type |= ledStipObject.containsKey(rf) << 7; // off refresh override
      // actual finalization is done in WLED::loop() (removing old busses and adding new)
      // this may happen even before this loop is finished so we do "doInitBusses" after the loop
      if (busConfigs[s] != nullptr) delete busConfigs[s];
      busConfigs[s] = new BusConfig(type, pins, start, length, colorOrder | (channelSwap<<4), ledStipObject.containsKey(cv), skip, awmode, freqHz, useGlobalLedBuffer);
      busesChanged = true;
      DEBUG_MSG_P(PSTR("Looping done successfulyy \n"));
    }
    //doInitBusses = busesChanged; // we will do that below to ensure all input data is processed

    ColorOrderMap com = {};
    for (uint8_t s = 0; s < WLED_MAX_COLOR_ORDER_MAPPINGS; s++) {
      char xs[4] = "XS"; xs[2] = 48+s; xs[3] = 0; //start LED
      char xc[4] = "XC"; xc[2] = 48+s; xc[3] = 0; //strip length
      char xo[4] = "XO"; xo[2] = 48+s; xo[3] = 0; //color order
      if (root.containsKey(xs)) {
        start = root[xs].as<int>();
        length = root[xc].as<int>();
        colorOrder = root[xo].as<int>();
        com.add(start, length, colorOrder);
      }
    }
    busses.updateColorOrderMap(com);

    // update other pins
    int hw_ir_pin = root[F("IR")].as<int>();
    if (pinManager.allocatePin(hw_ir_pin,false, PinOwner::IR)) {
      irPin = hw_ir_pin;
    } else {
      irPin = -1;
    }
    irEnabled = root[F("IT")].as<int>();
    irApplyToAllSelected = !ledStipObject.containsKey("MSO");

    int hw_rly_pin = root[F("RL")].as<int>();
    if (pinManager.allocatePin(hw_rly_pin,true, PinOwner::Relay)) {
      rlyPin = hw_rly_pin;
    } else {
      rlyPin = -1;
    }
    rlyMde = (bool)ledStipObject.containsKey("RM");

    disablePullUp = (bool)ledStipObject.containsKey("IP");
    for (uint8_t i=0; i<WLED_MAX_BUTTONS; i++) {
      char bt[4] = "BT"; bt[2] = (i<10?48:55)+i; bt[3] = 0; // button pin (use A,B,C,... if WLED_MAX_BUTTONS>10)
      char be[4] = "BE"; be[2] = (i<10?48:55)+i; be[3] = 0; // button type (use A,B,C,... if WLED_MAX_BUTTONS>10)
      int hw_btn_pin = root[bt].as<int>();
      if (pinManager.allocatePin(hw_btn_pin,false,PinOwner::Button)) {
        btnPin[i] = hw_btn_pin;
        buttonType[i] = root[be].as<int>();
      #ifdef ARDUINO_ARCH_ESP32
        // ESP32 only: check that analog button pin is a valid ADC gpio
        if (((buttonType[i] == BTN_TYPE_ANALOG) || (buttonType[i] == BTN_TYPE_ANALOG_INVERTED)) && (digitalPinToAnalogChannel(btnPin[i]) < 0))
        {
          // not an ADC analog pin
          if (btnPin[i] >= 0) DEBUG_PRINTF("PIN ALLOC error: GPIO%d for analog button #%d is not an analog pin!\n", btnPin[i], i);
          btnPin[i] = -1;
          pinManager.deallocatePin(hw_btn_pin,PinOwner::Button);
        }
        else
      #endif
        {
          if (disablePullUp) {
            pinMode(btnPin[i], INPUT);
          } else {
            #ifdef ESP32
            pinMode(btnPin[i], buttonType[i]==BTN_TYPE_PUSH_ACT_HIGH ? INPUT_PULLDOWN : INPUT_PULLUP);
            #else
            pinMode(btnPin[i], INPUT_PULLUP);
            #endif
          }
        }
      } else {
        btnPin[i] = -1;
        buttonType[i] = BTN_TYPE_NONE;
      }
    }
    touchThreshold = root[F("TT")].as<int>();

    strip.ablMilliampsMax = root[F("MA")].as<int>();
    strip.milliampsPerLed = root[F("LA")].as<int>();

    briS = root[F("CA")].as<int>();

    turnOnAtBoot = root.containsKey("BO");
    t = root[F("BP")].as<int>();
    if (t <= 250) bootPreset = t;
    gammaCorrectBri = root.containsKey("GB");
    gammaCorrectCol = root.containsKey("GC");
    gammaCorrectVal = root[F("GV")].as<float>();
    if (gammaCorrectVal > 1.0f && gammaCorrectVal <= 3)
      NeoGammaWLEDMethod::calcGammaTable(gammaCorrectVal);
    else {
      gammaCorrectVal = 1.0f; // no gamma correction
      gammaCorrectBri = false;
      gammaCorrectCol = false;
    }

    fadeTransition = root.containsKey("TF");
    modeBlending = root.containsKey("EB");
    t = root[F("TD")].as<int>();
    if (t >= 0) transitionDelayDefault = t;
    strip.paletteFade = root.containsKey("PF");;
    t = root[F("TP")].as<int>();
    randomPaletteChangeTime = MIN(255,MAX(1,t));

    nightlightTargetBri = root[F("TB")];
    t = root[F("TL")];
    if (t > 0) nightlightDelayMinsDefault = t;
    nightlightDelayMins = nightlightDelayMinsDefault;
    nightlightMode = root[F("TW")];

    t = root[F("PB")];
    if (t >= 0 && t < 4) strip.paletteBlend = t;
    t = root[F("BF")];
    if (t > 0) briMultiplier = t;

    doInitBusses = busesChanged;
  }

  // //UI TODO_S1: check if this is needed
  // if (subPage == SUBPAGE_UI)
  // {
  //   strlcpy(serverDescription, root[F("DS")).c_str(), 33);
  //   syncToggleReceive = request->hasArg(F("ST"));
  // #ifdef WLED_ENABLE_SIMPLE_UI
  //   if (simplifiedUI ^ request->hasArg(F("SU"))) {
  //     // UI selection changed, invalidate browser cache
  //     cacheInvalidate++;
  //   }
  //   simplifiedUI = request->hasArg(F("SU"));
  // #endif
  //   DEBUG_PRINTLN(F("Enumerating ledmaps"));
  //   enumerateLedmaps();
  //   DEBUG_PRINTLN(F("Loading custom palettes"));
  //   strip.loadCustomPalettes(); // (re)load all custom palettes
  // }

  // //SYNC TODO_S1: check if this is needed
  // if (subPage == SUBPAGE_SYNC)
  // {
  //   int t = root[F("UP")).toInt();
  //   if (t > 0) udpPort = t;
  //   t = root[F("U2")).toInt();
  //   if (t > 0) udpPort2 = t;

  //   syncGroups = root[F("GS")).toInt();
  //   receiveGroups = root[F("GR")).toInt();

  //   receiveNotificationBrightness = request->hasArg(F("RB"));
  //   receiveNotificationColor = request->hasArg(F("RC"));
  //   receiveNotificationEffects = request->hasArg(F("RX"));
  //   receiveSegmentOptions = request->hasArg(F("SO"));
  //   receiveSegmentBounds = request->hasArg(F("SG"));
  //   receiveNotifications = (receiveNotificationBrightness || receiveNotificationColor || receiveNotificationEffects || receiveSegmentOptions);
  //   notifyDirectDefault = request->hasArg(F("SD"));
  //   notifyDirect = notifyDirectDefault;
  //   notifyButton = request->hasArg(F("SB"));
  //   notifyAlexa = request->hasArg(F("SA"));
  //   notifyHue = request->hasArg(F("SH"));
  //   notifyMacro = request->hasArg(F("SM"));

  //   t = root[F("UR")).toInt();
  //   if ((t>=0) && (t<30)) udpNumRetries = t;


  //   nodeListEnabled = request->hasArg(F("NL"));
  //   if (!nodeListEnabled) Nodes.clear();
  //   nodeBroadcastEnabled = request->hasArg(F("NB"));

  //   receiveDirect = request->hasArg(F("RD"));
  //   useMainSegmentOnly = request->hasArg(F("MO"));
  //   e131SkipOutOfSequence = request->hasArg(F("ES"));
  //   e131Multicast = request->hasArg(F("EM"));
  //   t = root[F("EP")).toInt();
  //   if (t > 0) e131Port = t;
  //   t = root[F("EU")).toInt();
  //   if (t >= 0  && t <= 63999) e131Universe = t;
  //   t = root[F("DA")).toInt();
  //   if (t >= 0  && t <= 510) DMXAddress = t;
  //   t = root[F("XX")).toInt();
  //   if (t >= 0  && t <= 150) DMXSegmentSpacing = t;
  //   t = root[F("PY")).toInt();
  //   if (t >= 0  && t <= 200) e131Priority = t;
  //   t = root[F("DM")).toInt();
  //   if (t >= DMX_MODE_DISABLED && t <= DMX_MODE_PRESET) DMXMode = t;
  //   t = root[F("ET")).toInt();
  //   if (t > 99  && t <= 65000) realtimeTimeoutMs = t;
  //   arlsForceMaxBri = request->hasArg(F("FB"));
  //   arlsDisableGammaCorrection = request->hasArg(F("RG"));
  //   t = root[F("WO")).toInt();
  //   if (t >= -255  && t <= 255) arlsOffset = t;

  //   alexaEnabled = request->hasArg(F("AL"));
  //   strlcpy(alexaInvocationName, root[F("AI")).c_str(), 33);
  //   t = root[F("AP")).toInt();
  //   if (t >= 0 && t <= 9) alexaNumPresets = t;

  //   #ifdef WLED_ENABLE_MQTT
  //   mqttEnabled = request->hasArg(F("MQ"));
  //   strlcpy(mqttServer, root[F("MS")).c_str(), MQTT_MAX_SERVER_LEN+1);
  //   t = root[F("MQPORT")).toInt();
  //   if (t > 0) mqttPort = t;
  //   strlcpy(mqttUser, root[F("MQUSER")).c_str(), 41);
  //   if (!isAsterisksOnly(root[F("MQPASS")).c_str(), 41)) strlcpy(mqttPass, root[F("MQPASS")).c_str(), 65);
  //   strlcpy(mqttClientID, root[F("MQCID")).c_str(), 41);
  //   strlcpy(mqttDeviceTopic, root[F("MD")).c_str(), MQTT_MAX_TOPIC_LEN+1);
  //   strlcpy(mqttGroupTopic, root[F("MG")).c_str(), MQTT_MAX_TOPIC_LEN+1);
  //   buttonPublishMqtt = request->hasArg(F("BM"));
  //   retainMqttMsg = request->hasArg(F("RT"));
  //   #endif

  //   #ifndef WLED_DISABLE_HUESYNC
  //   for (int i=0;i<4;i++){
  //     String a = "H"+String(i);
  //     hueIP[i] = root[a).toInt();
  //   }

  //   t = root[F("HL")).toInt();
  //   if (t > 0) huePollLightId = t;

  //   t = root[F("HI")).toInt();
  //   if (t > 50) huePollIntervalMs = t;

  //   hueApplyOnOff = request->hasArg(F("HO"));
  //   hueApplyBri = request->hasArg(F("HB"));
  //   hueApplyColor = request->hasArg(F("HC"));
  //   huePollingEnabled = request->hasArg(F("HP"));
  //   hueStoreAllowed = true;
  //   reconnectHue();
  //   #endif

  //   t = root[F("BD")).toInt();
  //   if (t >= 96 && t <= 15000) serialBaud = t;
  //   updateBaudRate(serialBaud *100);
  // }

  // //TIME TODO_S1: check if this is needed
  // if (subPage == SUBPAGE_TIME)
  // {
  //   ntpEnabled = request->hasArg(F("NT"));
  //   strlcpy(ntpServerName, root[F("NS")).c_str(), 33);
  //   useAMPM = !request->hasArg(F("CF"));
  //   currentTimezone = root[F("TZ")).toInt();
  //   utcOffsetSecs = root[F("UO")).toInt();

  //   //start ntp if not already connected
  //   if (ntpEnabled && WLED_CONNECTED && !ntpConnected) ntpConnected = ntpUdp.begin(ntpLocalPort);
  //   ntpLastSyncTime = NTP_NEVER; // force new NTP query

  //   longitude = root[F("LN")).toFloat();
  //   latitude = root[F("LT")).toFloat();
  //   // force a sunrise/sunset re-calculation
  //   calculateSunriseAndSunset();

  //   overlayCurrent = request->hasArg(F("OL")) ? 1 : 0;

  //   overlayMin = root[F("O1")).toInt();
  //   overlayMax = root[F("O2")).toInt();
  //   analogClock12pixel = root[F("OM")).toInt();
  //   analogClock5MinuteMarks = request->hasArg(F("O5"));
  //   analogClockSecondsTrail = request->hasArg(F("OS"));

  //   countdownMode = request->hasArg(F("CE"));
  //   countdownYear = root[F("CY")).toInt();
  //   countdownMonth = root[F("CI")).toInt();
  //   countdownDay = root[F("CD")).toInt();
  //   countdownHour = root[F("CH")).toInt();
  //   countdownMin = root[F("CM")).toInt();
  //   countdownSec = root[F("CS")).toInt();
  //   setCountdown();

  //   macroAlexaOn = root[F("A0")).toInt();
  //   macroAlexaOff = root[F("A1")).toInt();
  //   macroCountdown = root[F("MC")).toInt();
  //   macroNl = root[F("MN")).toInt();
  //   for (uint8_t i=0; i<WLED_MAX_BUTTONS; i++) {
  //     char mp[4] = "MP"; mp[2] = (i<10?48:55)+i; mp[3] = 0; // short
  //     char ml[4] = "ML"; ml[2] = (i<10?48:55)+i; ml[3] = 0; // long
  //     char md[4] = "MD"; md[2] = (i<10?48:55)+i; md[3] = 0; // double
  //     //if (!request->hasArg(mp)) break;
  //     macroButton[i] = root[mp).toInt();      // these will default to 0 if not present
  //     macroLongPress[i] = root[ml).toInt();
  //     macroDoublePress[i] = root[md).toInt();
  //   }

  //   char k[3]; k[2] = 0;
  //   for (int i = 0; i<10; i++) {
  //     k[1] = i+48;//ascii 0,1,2,3,...
  //     k[0] = 'H'; //timer hours
  //     timerHours[i] = root[k).toInt();
  //     k[0] = 'N'; //minutes
  //     timerMinutes[i] = root[k).toInt();
  //     k[0] = 'T'; //macros
  //     timerMacro[i] = root[k).toInt();
  //     k[0] = 'W'; //weekdays
  //     timerWeekday[i] = root[k).toInt();
  //     if (i<8) {
  //       k[0] = 'M'; //start month
  //       timerMonth[i] = root[k).toInt() & 0x0F;
  //       timerMonth[i] <<= 4;
  //       k[0] = 'P'; //end month
  //       timerMonth[i] += (root[k).toInt() & 0x0F);
  //       k[0] = 'D'; //start day
  //       timerDay[i] = root[k).toInt();
  //       k[0] = 'E'; //end day
  //       timerDayEnd[i] = root[k).toInt();
  //     }
  //   }
  // }

  // //SECURITY TODO_S1: check if this is needed
  // if (subPage == SUBPAGE_SEC)
  // {
  //   if (request->hasArg(F("RS"))) //complete factory reset
  //   {
  //     WLED_FS.format();
  //     #ifdef WLED_ADD_EEPROM_SUPPORT
  //     clearEEPROM();
  //     #endif
  //     serveMessage(request, 200, F("All Settings erased."), F("Connect to WLED-AP to setup again"),255);
  //     doReboot = true; // may reboot immediately on dual-core system (race condition) which is desireable in this case
  //   }

  //   if (request->hasArg(F("PIN"))) {
  //     const char *pin = root[F("PIN")).c_str();
  //     uint8_t pinLen = strlen(pin);
  //     if (pinLen == 4 || pinLen == 0) {
  //       uint8_t numZeros = 0;
  //       for (uint8_t i = 0; i < pinLen; i++) numZeros += (pin[i] == '0');
  //       if (numZeros < pinLen || pinLen == 0) { // ignore 0000 input (placeholder)
  //         strlcpy(settingsPIN, pin, 5);
  //       }
  //       settingsPIN[4] = 0;
  //     }
  //   }

  //   bool pwdCorrect = !otaLock; //always allow access if ota not locked
  //   if (request->hasArg(F("OP")))
  //   {
  //     if (otaLock && strcmp(otaPass,root[F("OP")).c_str()) == 0)
  //     {
  //       // brute force protection: do not unlock even if correct if last save was less than 3 seconds ago
  //       if (millis() - lastEditTime > PIN_RETRY_COOLDOWN) pwdCorrect = true;
  //     }
  //     if (!otaLock && root[F("OP")).length() > 0)
  //     {
  //       strlcpy(otaPass,root[F("OP")).c_str(), 33); // set new OTA password
  //     }
  //   }

  //   if (pwdCorrect) //allow changes if correct pwd or no ota active
  //   {
  //     otaLock = request->hasArg(F("NO"));
  //     wifiLock = request->hasArg(F("OW"));
  //     aOtaEnabled = request->hasArg(F("AO"));
  //     //createEditHandler(correctPIN && !otaLock);
  //   }
  // }

  // #ifdef WLED_ENABLE_DMX // include only if DMX is enabled // TODO_S1: check if this is needed
  // if (subPage == SUBPAGE_DMX)
  // {
  //   int t = root[F("PU")).toInt();
  //   if (t >= 0  && t <= 63999) e131ProxyUniverse = t;

  //   t = root[F("CN")).toInt();
  //   if (t>0 && t<16) {
  //     DMXChannels = t;
  //   }
  //   t = root[F("CS")).toInt();
  //   if (t>0 && t<513) {
  //     DMXStart = t;
  //   }
  //   t = root[F("CG")).toInt();
  //   if (t>0 && t<513) {
  //     DMXGap = t;
  //   }
  //   t = root[F("SL")).toInt();
  //   if (t>=0 && t < MAX_LEDS) {
  //     DMXStartLED = t;
  //   }
  //   for (int i=0; i<15; i++) {
  //     String argname = "CH" + String((i+1));
  //     t = root[argname).toInt();
  //     DMXFixtureMap[i] = t;
  //   }
  // }
  // #endif

  // //USERMODS TODO_S1: check if this is needed
  // if (subPage == SUBPAGE_UM)
  // {
  //   if (!requestJSONBufferLock(5)) return;

  //   // global I2C & SPI pins
  //   int8_t hw_sda_pin  = !root[F("SDA")).length() ? -1 : (int)root[F("SDA")).toInt();
  //   int8_t hw_scl_pin  = !root[F("SCL")).length() ? -1 : (int)root[F("SCL")).toInt();
  //   if (i2c_sda != hw_sda_pin || i2c_scl != hw_scl_pin) {
  //     // only if pins changed
  //     uint8_t old_i2c[2] = { static_cast<uint8_t>(i2c_scl), static_cast<uint8_t>(i2c_sda) };
  //     pinManager.deallocateMultiplePins(old_i2c, 2, PinOwner::HW_I2C); // just in case deallocation of old pins

  //     PinManagerPinType i2c[2] = { { hw_sda_pin, true }, { hw_scl_pin, true } };
  //     if (hw_sda_pin >= 0 && hw_scl_pin >= 0 && pinManager.allocateMultiplePins(i2c, 2, PinOwner::HW_I2C)) {
  //       i2c_sda = hw_sda_pin;
  //       i2c_scl = hw_scl_pin;
  //       // no bus re-initialisation as usermods do not get any notification
  //       //Wire.begin(i2c_sda, i2c_scl);
  //     } else {
  //       // there is no Wire.end()
  //       DEBUG_PRINTLN(F("Could not allocate I2C pins."));
  //       i2c_sda = -1;
  //       i2c_scl = -1;
  //     }
  //   }
  //   int8_t hw_mosi_pin = !root[F("MOSI")).length() ? -1 : (int)root[F("MOSI")).toInt();
  //   int8_t hw_miso_pin = !root[F("MISO")).length() ? -1 : (int)root[F("MISO")).toInt();
  //   int8_t hw_sclk_pin = !root[F("SCLK")).length() ? -1 : (int)root[F("SCLK")).toInt();
  //   #ifdef ESP8266
  //   // cannot change pins on ESP8266
  //   if (hw_mosi_pin >= 0 && hw_mosi_pin != HW_PIN_DATASPI)  hw_mosi_pin = HW_PIN_DATASPI;
  //   if (hw_miso_pin >= 0 && hw_miso_pin != HW_PIN_MISOSPI)  hw_mosi_pin = HW_PIN_MISOSPI;
  //   if (hw_sclk_pin >= 0 && hw_sclk_pin != HW_PIN_CLOCKSPI) hw_sclk_pin = HW_PIN_CLOCKSPI;
  //   #endif
  //   if (spi_mosi != hw_mosi_pin || spi_miso != hw_miso_pin || spi_sclk != hw_sclk_pin) {
  //     // only if pins changed
  //     uint8_t old_spi[3] = { static_cast<uint8_t>(spi_mosi), static_cast<uint8_t>(spi_miso), static_cast<uint8_t>(spi_sclk) };
  //     pinManager.deallocateMultiplePins(old_spi, 3, PinOwner::HW_SPI); // just in case deallocation of old pins
  //     PinManagerPinType spi[3] = { { hw_mosi_pin, true }, { hw_miso_pin, true }, { hw_sclk_pin, true } };
  //     if (hw_mosi_pin >= 0 && hw_sclk_pin >= 0 && pinManager.allocateMultiplePins(spi, 3, PinOwner::HW_SPI)) {
  //       spi_mosi = hw_mosi_pin;
  //       spi_miso = hw_miso_pin;
  //       spi_sclk = hw_sclk_pin;
  //       // no bus re-initialisation as usermods do not get any notification
  //       //SPI.end();
  //       #ifdef ESP32
  //       //SPI.begin(spi_sclk, spi_miso, spi_mosi);
  //       #else
  //       //SPI.begin();
  //       #endif
  //     } else {
  //       //SPI.end();
  //       DEBUG_PRINTLN(F("Could not allocate SPI pins."));
  //       spi_mosi = -1;
  //       spi_miso = -1;
  //       spi_sclk = -1;
  //     }
  //   }

  //   JsonObject um = doc.createNestedObject("um");

  //   size_t args = request->args();
  //   uint16_t j=0;
  //   for (size_t i=0; i<args; i++) {
  //     String name = request->argName(i);
  //     String value = root[i);

  //     // POST request parameters are combined as <usermodname>_<usermodparameter>
  //     int umNameEnd = name.indexOf(":");
  //     if (umNameEnd<1) continue;  // parameter does not contain ":" or on 1st place -> wrong

  //     JsonObject mod = um[name.substring(0,umNameEnd)]; // get a usermod JSON object
  //     if (mod.isNull()) {
  //       mod = um.createNestedObject(name.substring(0,umNameEnd)); // if it does not exist create it
  //     }
  //     DEBUG_PRINT(name.substring(0,umNameEnd));
  //     DEBUG_PRINT(":");
  //     name = name.substring(umNameEnd+1); // remove mod name from string

  //     // if the resulting name still contains ":" this means nested object
  //     JsonObject subObj;
  //     int umSubObj = name.indexOf(":");
  //     DEBUG_PRINTF("(%d):",umSubObj);
  //     if (umSubObj>0) {
  //       subObj = mod[name.substring(0,umSubObj)];
  //       if (subObj.isNull())
  //         subObj = mod.createNestedObject(name.substring(0,umSubObj));
  //       name = name.substring(umSubObj+1); // remove nested object name from string
  //     } else {
  //       subObj = mod;
  //     }
  //     DEBUG_PRINT(name);

  //     // check if parameters represent array
  //     if (name.endsWith("[]")) {
  //       name.replace("[]","");
  //       value.replace(",",".");      // just in case conversion
  //       if (!subObj[name].is<JsonArray>()) {
  //         JsonArray ar = subObj.createNestedArray(name);
  //         if (value.indexOf(".") >= 0) ar.add(value.toFloat());  // we do have a float
  //         else                         ar.add(value.toInt());    // we may have an int
  //         j=0;
  //       } else {
  //         if (value.indexOf(".") >= 0) subObj[name].add(value.toFloat());  // we do have a float
  //         else                         subObj[name].add(value.toInt());    // we may have an int
  //         j++;
  //       }
  //       DEBUG_PRINT("[");
  //       DEBUG_PRINT(j);
  //       DEBUG_PRINT("] = ");
  //       DEBUG_PRINTLN(value);
  //     } else {
  //       // we are using a hidden field with the same name as our parameter (!before the actual parameter!)
  //       // to describe the type of parameter (text,float,int), for boolean parameters the first field contains "off"
  //       // so checkboxes have one or two fields (first is always "false", existence of second depends on checkmark and may be "true")
  //       if (subObj[name].isNull()) {
  //         // the first occurrence of the field describes the parameter type (used in next loop)
  //         if (value == "false") subObj[name] = false; // checkboxes may have only one field
  //         else                  subObj[name] = value;
  //       } else {
  //         String type = subObj[name].as<String>();  // get previously stored value as a type
  //         if (subObj[name].is<bool>())   subObj[name] = true;   // checkbox/boolean
  //         else if (type == "number") {
  //           value.replace(",",".");      // just in case conversion
  //           if (value.indexOf(".") >= 0) subObj[name] = value.toFloat();  // we do have a float
  //           else                         subObj[name] = value.toInt();    // we may have an int
  //         } else if (type == "int")      subObj[name] = value.toInt();
  //         else                           subObj[name] = value;  // text fields
  //       }
  //       DEBUG_PRINT(" = ");
  //       DEBUG_PRINTLN(value);
  //     }
  //   }
  //   usermods.readFromConfig(um);  // force change of usermod parameters
  //   DEBUG_PRINTLN(F("Done re-init usermods."));
  //   releaseJSONBufferLock();
  // }

  // #ifndef WLED_DISABLE_2D 
  // //2D panels // TODO_S1: check if this is needed
  // if (subPage == SUBPAGE_2D)
  // {
  //   strip.isMatrix = root[F("SOMP")).toInt();
  //   strip.panel.clear(); // release memory if allocated
  //   if (strip.isMatrix) {
  //     strip.panels  = MAX(1,MIN(WLED_MAX_PANELS,root[F("MPC")).toInt()));
  //     strip.panel.reserve(strip.panels); // pre-allocate memory
  //     for (uint8_t i=0; i<strip.panels; i++) {
  //       WS2812FX::Panel p;
  //       char pO[8] = { '\0' };
  //       snprintf_P(pO, 7, PSTR("P%d"), i);       // MAX_PANELS is 64 so pO will always only be 4 characters or less
  //       pO[7] = '\0';
  //       uint8_t l = strlen(pO);
  //       // create P0B, P1B, ..., P63B, etc for other PxxX
  //       pO[l] = 'B'; if (!request->hasArg(pO)) break;
  //       pO[l] = 'B'; p.bottomStart = root[pO).toInt();
  //       pO[l] = 'R'; p.rightStart  = root[pO).toInt();
  //       pO[l] = 'V'; p.vertical    = root[pO).toInt();
  //       pO[l] = 'S'; p.serpentine  = request->hasArg(pO);
  //       pO[l] = 'X'; p.xOffset     = root[pO).toInt();
  //       pO[l] = 'Y'; p.yOffset     = root[pO).toInt();
  //       pO[l] = 'W'; p.width       = root[pO).toInt();
  //       pO[l] = 'H'; p.height      = root[pO).toInt();
  //       strip.panel.push_back(p);
  //     }
  //     strip.setUpMatrix(); // will check limits
  //     strip.makeAutoSegments(true);
  //     strip.deserializeMap();
  //   } else {
  //     Segment::maxWidth  = strip.getLengthTotal();
  //     Segment::maxHeight = 1;
  //   }
  // }
  // #endif

  lastEditTime = millis();
  // do not save if factory reset or LED settings (which are saved after LED re-init)
  doSerializeConfig = subPage != SUBPAGE_LEDS && !(subPage == SUBPAGE_SEC && doReboot);
  if (subPage == SUBPAGE_UM) doReboot = root[F("RBT")]; // prevent race condition on dual core system (set reboot here, after doSerializeConfig has been set)
  #ifndef WLED_DISABLE_ALEXA
  if (subPage == SUBPAGE_SYNC) alexaInit();
  #endif
}
