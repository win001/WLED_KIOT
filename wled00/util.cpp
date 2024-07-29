#include "wled.h"
#include "fcn_declare.h"
#include "const.h"

#ifdef WLED_DISABLE_WEBSERVER
void createEditHandler(bool enable){} // creating this empty function if wled webserver is disabled to avoid undefined reference error, it is getting called in util.cpp, wled.cpp and set.cpp
#endif
//helper to get int value at a position in string
int getNumVal(const String* req, uint16_t pos)
{
  return req->substring(pos+3).toInt();
}


//helper to get int value with in/decrementing support via ~ syntax
void parseNumber(const char* str, byte* val, byte minv, byte maxv)
{
  if (str == nullptr || str[0] == '\0') return;
  if (str[0] == 'r') {*val = random8(minv,maxv?maxv:255); return;} // maxv for random cannot be 0
  bool wrap = false;
  if (str[0] == 'w' && strlen(str) > 1) {str++; wrap = true;}
  if (str[0] == '~') {
    int out = atoi(str +1);
    if (out == 0) {
      if (str[1] == '0') return;
      if (str[1] == '-') {
        *val = (int)(*val -1) < (int)minv ? maxv : min((int)maxv,(*val -1)); //-1, wrap around
      } else {
        *val = (int)(*val +1) > (int)maxv ? minv : max((int)minv,(*val +1)); //+1, wrap around
      }
    } else {
      if (wrap && *val == maxv && out > 0) out = minv;
      else if (wrap && *val == minv && out < 0) out = maxv;
      else {
        out += *val;
        if (out > maxv) out = maxv;
        if (out < minv) out = minv;
      }
      *val = out;
    }
    return;
  } else if (minv == maxv && minv == 0) { // limits "unset" i.e. both 0
    byte p1 = atoi(str);
    const char* str2 = strchr(str,'~'); // min/max range (for preset cycle, e.g. "1~5~")
    if (str2) {
      byte p2 = atoi(++str2);           // skip ~
      if (p2 > 0) {
        while (isdigit(*(++str2)));     // skip digits
        parseNumber(str2, val, p1, p2);
        return;
      }
    }
  }
  *val = atoi(str);
}


bool getVal(JsonVariant elem, byte* val, byte vmin, byte vmax) {
  if (elem.is<int>()) {
		if (elem < 0) return false; //ignore e.g. {"ps":-1}
    *val = elem;
    return true;
  } else if (elem.is<const char*>()) {
    const char* str = elem;
    size_t len = strnlen(str, 12);
    if (len == 0 || len > 10) return false;
    parseNumber(str, val, vmin, vmax);
    return true;
  }
  return false; //key does not exist
}


bool updateVal(const char* req, const char* key, byte* val, byte minv, byte maxv)
{
  const char *v = strstr(req, key);
  if (v) v += strlen(key);
  else return false;
  parseNumber(v, val, minv, maxv);
  return true;
}


//append a numeric setting to string buffer
void sappend(char stype, const char* key, int val)
{
  char ds[] = "d.Sf.";

  switch(stype)
  {
    case 'c': //checkbox
      oappend(ds);
      oappend(key);
      oappend(".checked=");
      oappendi(val);
      oappend(";");
      break;
    case 'v': //numeric
      oappend(ds);
      oappend(key);
      oappend(".value=");
      oappendi(val);
      oappend(";");
      break;
    case 'i': //selectedIndex
      oappend(ds);
      oappend(key);
      oappend(SET_F(".selectedIndex="));
      oappendi(val);
      oappend(";");
      break;
  }
}


//append a string setting to buffer
void sappends(char stype, const char* key, char* val)
{
  switch(stype)
  {
    case 's': {//string (we can interpret val as char*)
      String buf = val;
      //convert "%" to "%%" to make EspAsyncWebServer happy
      //buf.replace("%","%%");
      oappend("d.Sf.");
      oappend(key);
      oappend(".value=\"");
      oappend(buf.c_str());
      oappend("\";");
      break;}
    case 'm': //message
      oappend(SET_F("d.getElementsByClassName"));
      oappend(key);
      oappend(SET_F(".innerHTML=\""));
      oappend(val);
      oappend("\";");
      break;
  }
}


bool oappendi(int i)
{
  char s[11];
  sprintf(s, "%d", i);
  return oappend(s);
}


bool oappend(const char* txt)
{
  uint16_t len = strlen(txt);
  if ((obuf == nullptr) || (olen + len >= SETTINGS_STACK_BUF_SIZE)) { // sanity checks
#ifdef WLED_DEBUG
    DEBUG_PRINT(F("oappend() buffer overflow. Cannot append "));
    DEBUG_PRINT(len); DEBUG_PRINT(F(" bytes \t\""));
    DEBUG_PRINT(txt); DEBUG_PRINTLN(F("\""));
#endif
    return false;        // buffer full
  }
  strcpy(obuf + olen, txt);
  olen += len;
  return true;
}


void prepareHostname(char* hostname)
{
  sprintf_P(hostname, "wled-%*s", 6, escapedMac.c_str() + 6);
  const char *pC = serverDescription;
  uint8_t pos = 5;          // keep "wled-"
  while (*pC && pos < 24) { // while !null and not over length
    if (isalnum(*pC)) {     // if the current char is alpha-numeric append it to the hostname
      hostname[pos] = *pC;
      pos++;
    } else if (*pC == ' ' || *pC == '_' || *pC == '-' || *pC == '+' || *pC == '!' || *pC == '?' || *pC == '*') {
      hostname[pos] = '-';
      pos++;
    }
    // else do nothing - no leading hyphens and do not include hyphens for all other characters.
    pC++;
  }
  //last character must not be hyphen
  if (pos > 5) {
    while (pos > 4 && hostname[pos -1] == '-') pos--;
    hostname[pos] = '\0'; // terminate string (leave at least "wled")
  }
}


bool isAsterisksOnly(const char* str, byte maxLen)
{
  for (byte i = 0; i < maxLen; i++) {
    if (str[i] == 0) break;
    if (str[i] != '*') return false;
  }
  //at this point the password contains asterisks only
  return (str[0] != 0); //false on empty string
}


//threading/network callback details: https://github.com/Aircoookie/WLED/pull/2336#discussion_r762276994
bool requestJSONBufferLock(uint8_t module)
{
  unsigned long now = millis();

  while (jsonBufferLock && millis()-now < 1000) delay(1); // wait for a second for buffer lock

  if (millis()-now >= 1000) {
    DEBUG_PRINT(F("ERROR: Locking JSON buffer failed! ("));
    DEBUG_PRINT(jsonBufferLock);
    DEBUG_PRINTLN(")");
    return false; // waiting time-outed
  }

  jsonBufferLock = module ? module : 255;
  DEBUG_PRINT(F("JSON buffer locked. ("));
  DEBUG_PRINT(jsonBufferLock);
  DEBUG_PRINTLN(")");
  fileDoc = &doc;  // used for applying presets (presets.cpp)
  doc.clear();
  return true;
}


void releaseJSONBufferLock()
{
  DEBUG_PRINT(F("JSON buffer released. ("));
  DEBUG_PRINT(jsonBufferLock);
  DEBUG_PRINTLN(")");
  fileDoc = nullptr;
  jsonBufferLock = 0;
}


// extracts effect mode (or palette) name from names serialized string
// caller must provide large enough buffer for name (including SR extensions)!
uint8_t extractModeName(uint8_t mode, const char *src, char *dest, uint8_t maxLen)
{
  if (src == JSON_mode_names || src == nullptr) {
    if (mode < strip.getModeCount()) {
      char lineBuffer[256];
      //strcpy_P(lineBuffer, (const char*)pgm_read_dword(&(WS2812FX::_modeData[mode])));
      strncpy_P(lineBuffer, strip.getModeData(mode), sizeof(lineBuffer)/sizeof(char)-1);
      lineBuffer[sizeof(lineBuffer)/sizeof(char)-1] = '\0'; // terminate string
      size_t len = strlen(lineBuffer);
      size_t j = 0;
      for (; j < maxLen && j < len; j++) {
        if (lineBuffer[j] == '\0' || lineBuffer[j] == '@') break;
        dest[j] = lineBuffer[j];
      }
      dest[j] = 0; // terminate string
      return strlen(dest);
    } else return 0;
  }

  if (src == JSON_palette_names && mode > GRADIENT_PALETTE_COUNT) {
    snprintf_P(dest, maxLen, PSTR("~ Custom %d~"), 255-mode);
    dest[maxLen-1] = '\0';
    return strlen(dest);
  }

  uint8_t qComma = 0;
  bool insideQuotes = false;
  uint8_t printedChars = 0;
  char singleJsonSymbol;
  size_t len = strlen_P(src);

  // Find the mode name in JSON
  for (size_t i = 0; i < len; i++) {
    singleJsonSymbol = pgm_read_byte_near(src + i);
    if (singleJsonSymbol == '\0') break;
    if (singleJsonSymbol == '@' && insideQuotes && qComma == mode) break; //stop when SR extension encountered
    switch (singleJsonSymbol) {
      case '"':
        insideQuotes = !insideQuotes;
        break;
      case '[':
      case ']':
        break;
      case ',':
        if (!insideQuotes) qComma++;
      default:
        if (!insideQuotes || (qComma != mode)) break;
        dest[printedChars++] = singleJsonSymbol;
    }
    if ((qComma > mode) || (printedChars >= maxLen)) break;
  }
  dest[printedChars] = '\0';
  return strlen(dest);
}


// extracts effect slider data (1st group after @)
uint8_t extractModeSlider(uint8_t mode, uint8_t slider, char *dest, uint8_t maxLen, uint8_t *var)
{
  dest[0] = '\0'; // start by clearing buffer

  if (mode < strip.getModeCount()) {
    String lineBuffer = FPSTR(strip.getModeData(mode));
    if (lineBuffer.length() > 0) {
      int16_t start = lineBuffer.indexOf('@');
      int16_t stop  = lineBuffer.indexOf(';', start);
      if (start>0 && stop>0) {
        String names = lineBuffer.substring(start, stop); // include @
        int16_t nameBegin = 1, nameEnd, nameDefault;
        if (slider < 10) {
          for (size_t i=0; i<=slider; i++) {
            const char *tmpstr;
            dest[0] = '\0'; //clear dest buffer
            if (nameBegin == 0) break; // there are no more names
            nameEnd = names.indexOf(',', nameBegin);
            if (i == slider) {
              nameDefault = names.indexOf('=', nameBegin); // find default value
              if (nameDefault > 0 && var && ((nameEnd>0 && nameDefault<nameEnd) || nameEnd<0)) {
                *var = (uint8_t)atoi(names.substring(nameDefault+1).c_str());
              }
              if (names.charAt(nameBegin) == '!') {
                switch (slider) {
                  case  0: tmpstr = PSTR("FX Speed");     break;
                  case  1: tmpstr = PSTR("FX Intensity"); break;
                  case  2: tmpstr = PSTR("FX Custom 1");  break;
                  case  3: tmpstr = PSTR("FX Custom 2");  break;
                  case  4: tmpstr = PSTR("FX Custom 3");  break;
                  default: tmpstr = PSTR("FX Custom");    break;
                }
                strncpy_P(dest, tmpstr, maxLen); // copy the name into buffer (replacing previous)
                dest[maxLen-1] = '\0';
              } else {
                if (nameEnd<0) tmpstr = names.substring(nameBegin).c_str(); // did not find ",", last name?
                else           tmpstr = names.substring(nameBegin, nameEnd).c_str();
                strlcpy(dest, tmpstr, maxLen); // copy the name into buffer (replacing previous)
              }
            }
            nameBegin = nameEnd+1; // next name (if "," is not found it will be 0)
          } // next slider
        } else if (slider == 255) {
          // palette
          strlcpy(dest, "pal", maxLen);
          names = lineBuffer.substring(stop+1); // stop has index of color slot names
          nameBegin = names.indexOf(';'); // look for palette
          if (nameBegin >= 0) {
            nameEnd = names.indexOf(';', nameBegin+1);
            if (!isdigit(names[nameBegin+1])) nameBegin = names.indexOf('=', nameBegin+1); // look for default value
            if (nameEnd >= 0 && nameBegin > nameEnd) nameBegin = -1;
            if (nameBegin >= 0 && var) {
              *var = (uint8_t)atoi(names.substring(nameBegin+1).c_str());
            }
          }
        }
        // we have slider name (including default value) in the dest buffer
        for (size_t i=0; i<strlen(dest); i++) if (dest[i]=='=') { dest[i]='\0'; break; } // truncate default value

      } else {
        // defaults to just speed and intensity since there is no slider data
        switch (slider) {
          case 0:  strncpy_P(dest, PSTR("FX Speed"), maxLen); break;
          case 1:  strncpy_P(dest, PSTR("FX Intensity"), maxLen); break;
        }
        dest[maxLen] = '\0'; // strncpy does not necessarily null terminate string
      }
    }
    return strlen(dest);
  }
  return 0;
}


// extracts mode parameter defaults from last section of mode data (e.g. "Juggle@!,Trail;!,!,;!;sx=16,ix=240,1d")
int16_t extractModeDefaults(uint8_t mode, const char *segVar)
{
  if (mode < strip.getModeCount()) {
    char lineBuffer[256];
    strncpy_P(lineBuffer, strip.getModeData(mode), sizeof(lineBuffer)/sizeof(char)-1);
    lineBuffer[sizeof(lineBuffer)/sizeof(char)-1] = '\0'; // terminate string
    if (lineBuffer[0] != 0) {
      char* startPtr = strrchr(lineBuffer, ';'); // last ";" in FX data
      if (!startPtr) return -1;

      char* stopPtr = strstr(startPtr, segVar);
      if (!stopPtr) return -1;

      stopPtr += strlen(segVar) +1; // skip "="
      return atoi(stopPtr);
    }
  }
  return -1;
}


void checkSettingsPIN(const char* pin) {
  if (!pin) return;
  if (!correctPIN && millis() - lastEditTime < PIN_RETRY_COOLDOWN) return; // guard against PIN brute force
  bool correctBefore = correctPIN;
  correctPIN = (strlen(settingsPIN) == 0 || strncmp(settingsPIN, pin, 4) == 0);
  if (correctBefore != correctPIN) createEditHandler(correctPIN);
  lastEditTime = millis();
}


uint16_t crc16(const unsigned char* data_p, size_t length) {
  uint8_t x;
  uint16_t crc = 0xFFFF;
  if (!length) return 0x1D0F;
  while (length--) {
    x = crc >> 8 ^ *data_p++;
    x ^= x>>4;
    crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x <<5)) ^ ((uint16_t)x);
  }
  return crc;
}


///////////////////////////////////////////////////////////////////////////////
// Begin simulateSound (to enable audio enhanced effects to display something)
///////////////////////////////////////////////////////////////////////////////
// Currently 4 types defined, to be fine tuned and new types added
// (only 2 used as stored in 1 bit in segment options, consider switching to a single global simulation type)
typedef enum UM_SoundSimulations {
  UMS_BeatSin = 0,
  UMS_WeWillRockYou,
  UMS_10_13,
  UMS_14_3
} um_soundSimulations_t;

um_data_t* simulateSound(uint8_t simulationId)
{
  static uint8_t samplePeak;
  static float   FFT_MajorPeak;
  static uint8_t maxVol;
  static uint8_t binNum;

  static float    volumeSmth;
  static uint16_t volumeRaw;
  static float    my_magnitude;

  //arrays
  uint8_t *fftResult;

  static um_data_t* um_data = nullptr;

  if (!um_data) {
    //claim storage for arrays
    fftResult = (uint8_t *)malloc(sizeof(uint8_t) * 16);

    // initialize um_data pointer structure
    // NOTE!!!
    // This may change as AudioReactive usermod may change
    um_data = new um_data_t;
    um_data->u_size = 8;
    um_data->u_type = new um_types_t[um_data->u_size];
    um_data->u_data = new void*[um_data->u_size];
    um_data->u_data[0] = &volumeSmth;
    um_data->u_data[1] = &volumeRaw;
    um_data->u_data[2] = fftResult;
    um_data->u_data[3] = &samplePeak;
    um_data->u_data[4] = &FFT_MajorPeak;
    um_data->u_data[5] = &my_magnitude;
    um_data->u_data[6] = &maxVol;
    um_data->u_data[7] = &binNum;
  } else {
    // get arrays from um_data
    fftResult =  (uint8_t*)um_data->u_data[2];
  }

  uint32_t ms = millis();

  switch (simulationId) {
    default:
    case UMS_BeatSin:
      for (int i = 0; i<16; i++)
        fftResult[i] = beatsin8(120 / (i+1), 0, 255);
        // fftResult[i] = (beatsin8(120, 0, 255) + (256/16 * i)) % 256;
        volumeSmth = fftResult[8];
      break;
    case UMS_WeWillRockYou:
      if (ms%2000 < 200) {
        volumeSmth = random8(255);
        for (int i = 0; i<5; i++)
          fftResult[i] = random8(255);
      }
      else if (ms%2000 < 400) {
        volumeSmth = 0;
        for (int i = 0; i<16; i++)
          fftResult[i] = 0;
      }
      else if (ms%2000 < 600) {
        volumeSmth = random8(255);
        for (int i = 5; i<11; i++)
          fftResult[i] = random8(255);
      }
      else if (ms%2000 < 800) {
        volumeSmth = 0;
        for (int i = 0; i<16; i++)
          fftResult[i] = 0;
      }
      else if (ms%2000 < 1000) {
        volumeSmth = random8(255);
        for (int i = 11; i<16; i++)
          fftResult[i] = random8(255);
      }
      else {
        volumeSmth = 0;
        for (int i = 0; i<16; i++)
          fftResult[i] = 0;
      }
      break;
    case UMS_10_13:
      for (int i = 0; i<16; i++)
        fftResult[i] = inoise8(beatsin8(90 / (i+1), 0, 200)*15 + (ms>>10), ms>>3);
        volumeSmth = fftResult[8];
      break;
    case UMS_14_3:
      for (int i = 0; i<16; i++)
        fftResult[i] = inoise8(beatsin8(120 / (i+1), 10, 30)*10 + (ms>>14), ms>>3);
      volumeSmth = fftResult[8];
      break;
  }

  samplePeak    = random8() > 250;
  FFT_MajorPeak = 21 + (volumeSmth*volumeSmth) / 8.0f; // walk thru full range of 21hz...8200hz
  maxVol        = 31;  // this gets feedback fro UI
  binNum        = 8;   // this gets feedback fro UI
  volumeRaw = volumeSmth;
  my_magnitude = 10000.0f / 8.0f; //no idea if 10000 is a good value for FFT_Magnitude ???
  if (volumeSmth < 1 ) my_magnitude = 0.001f;             // noise gate closed - mute

  return um_data;
}


// enumerate all ledmapX.json files on FS and extract ledmap names if existing
void enumerateLedmaps() {
  ledMaps = 1;
  for (size_t i=1; i<WLED_MAX_LEDMAPS; i++) {
    char fileName[33];
    sprintf_P(fileName, PSTR("/ledmap%d.json"), i);
    bool isFile = WLED_FS.exists(fileName);

    #ifndef ESP8266
    if (ledmapNames[i-1]) { //clear old name
      delete[] ledmapNames[i-1];
      ledmapNames[i-1] = nullptr;
    }
    #endif

    if (isFile) {
      ledMaps |= 1 << i;

      #ifndef ESP8266
      if (requestJSONBufferLock(21)) {
        if (readObjectFromFile(fileName, nullptr, &doc)) {
          size_t len = 0;
          if (!doc["n"].isNull()) {
            // name field exists
            const char *name = doc["n"].as<const char*>();
            if (name != nullptr) len = strlen(name);
            if (len > 0 && len < 33) {
              ledmapNames[i-1] = new char[len+1];
              if (ledmapNames[i-1]) strlcpy(ledmapNames[i-1], name, 33);
            }
          }
          if (!ledmapNames[i-1]) {
            char tmp[33];
            snprintf_P(tmp, 32, PSTR("ledmap%d.json"), i);
            len = strlen(tmp);
            ledmapNames[i-1] = new char[len+1];
            if (ledmapNames[i-1]) strlcpy(ledmapNames[i-1], tmp, 33);
          }
        }
        releaseJSONBufferLock();
      }
      #endif
    }

  }
}

/*
 * Returns a new, random color wheel index with a minimum distance of 42 from pos.
 */
uint8_t get_random_wheel_index(uint8_t pos) {
  uint8_t r = 0, x = 0, y = 0, d = 0;
  while (d < 42) {
    r = random8();
    x = abs(pos - r);
    y = 255 - x;
    d = MIN(x, y);
  }
  return r;
}

/*
 * print the content of a given file to the serial console
 */
void printFileContentFS(const char* path) {
    File file = WLED_FS.open(path, "r");
    if (!file) {
        DEBUG_PRINT(F("Failed to open file for reading : "));
        DEBUG_PRINTLN(path);
        return;
    }
    DEBUG_PRINT(F("Content of "));
    DEBUG_PRINT(path);
    DEBUG_PRINTLN(F(":"));

    while (file.available()) {
        Serial.write(file.read());
    }
    DEBUG_PRINTLN();
    file.close();
}

/*
 * copied from tools.ino
 */
void wifiDisconnect() {
    WiFi.disconnect();
}

void wifiStatus() {
    // delay(200);
    if (WiFi.getMode() == WIFI_AP_STA) {
        // Serial.println("MODE AP + STA");
        DEBUG_MSG_P(PSTR("[WIFI] MODE AP + STA --------------------------------\n"));
    } else if (WiFi.getMode() == WIFI_AP) {
        //  Serial.println("MODE AP");
        DEBUG_MSG_P(PSTR("[WIFI] MODE AP --------------------------------------\n"));
    } else if (WiFi.getMode() == WIFI_STA) {
        DEBUG_MSG_P(PSTR("[WIFI] MODE STA -------------------------------------\n"));
    } else {
        //  Serial.println("MODE OFF");
        DEBUG_MSG_P(PSTR("[WIFI] MODE OFF -------------------------------------\n"));
        DEBUG_MSG_P(PSTR("[WIFI] No connection\n"));
    }

    if ((WiFi.getMode() & WIFI_AP) == WIFI_AP) {
        // DEBUG_MSG_P(PSTR("[WIFI] SSID %s\n"), jw.getAPSSID().c_str()); // TODO_S2
        DEBUG_MSG_P(PSTR("[WIFI] PASS %s\n"), getSetting("adminPass", ADMIN_PASS).c_str());
        DEBUG_MSG_P(PSTR("[WIFI] IP   %s\n"), WiFi.softAPIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] MAC  %s\n"), WiFi.softAPmacAddress().c_str());
    }

    if ((WiFi.getMode() & WIFI_STA) == WIFI_STA) {
        //  Serial.println("STA SS");
        DEBUG_MSG_P(PSTR("[WIFI] SSID %s\n"), WiFi.SSID().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] IP   %s\n"), WiFi.localIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] MAC  %s\n"), WiFi.macAddress().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] GW   %s\n"), WiFi.gatewayIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] DNS  %s\n"), WiFi.dnsIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] MASK %s\n"), WiFi.subnetMask().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] HOST %s\n"), WiFi.hostname().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] Channel %d \n"), WiFi.channel());
        DEBUG_MSG_P(PSTR("[WIFI] Bssid %s \n"), WiFi.BSSIDstr().c_str());
        
        // jw.setWifiMode(WIFI_STA);
    }
    DEBUG_MSG_P(PSTR("[WIFI] ----------------------------------------------\n"));
}

long long char2LL(char *str) {
    long long result = 0;  // Initialize result
    // Iterate through all characters of input string and update result
    for (int i = 0; str[i] != '\0'; ++i)
        result = result * 10 + str[i] - '0';
    return result;
}
void subString(char *source, char *destination, int start_index, int end_index) {
    int index;
    for (index = 0; start_index < end_index; start_index++, index++)
        destination[index] = (char)source[start_index];
    destination[index] = '\0';
}

int getBase64Len(int len) {
    return (len % 3) == 0 ? 4 * (len / 3) : 4 * ((len / 3) + 1);
}
int getCipherlength(int len) {
    // int b64_len =  getBase64Len(len);
    // return len + (len %16 == 0)? len : len+(16-len%16);
    return len + 16 - len % 16;
}

// #include <Ticker.h>
Ticker _defer_reset;
uint8_t _reset_reason = 0;
extern EEPROM_Rotate EEPROMr;

void setBoardName() {
#ifndef ESPURNA_CORE
    setSetting("boardName", DEVICE);
#endif
}

String getBoardName() {
    return getSetting("boardName", DEVICE);
}

String getCoreVersion() {
    String version = ESP.getCoreVersion();
#ifdef ARDUINO_ESP8266_RELEASE
    if (version.equals("00000000")) {
        version = String(ARDUINO_ESP8266_RELEASE);
    }
#endif
    return version;
}

String getCoreRevision() {
#ifdef ARDUINO_ESP8266_GIT_VER
    return String(ARDUINO_ESP8266_GIT_VER);
#else
    return String("");
#endif
}
// WTF
// Calling ESP.getFreeHeap() is making the system crash on a specific
// AiLight bulb, but anywhere else...
unsigned int getFreeHeap() {
    return ESP.getFreeHeap();
}

unsigned int getInitialFreeHeap() {
    static unsigned int _heap = 0;
    if (0 == _heap) {
        _heap = getFreeHeap();
    }
    return _heap;
}

unsigned int getInitialFreeStack() {
    // TODO_S2
    static unsigned int _stack = 0;
    // if (0 == _stack) {
    //     _stack = getFreeStack();
    // }
    return _stack;
}

unsigned int getUsedHeap() {
    return getInitialFreeHeap() - getFreeHeap();
}

String buildTime() {
    const char time_now[] = __TIME__;  // hh:mm:ss
    unsigned int hour = atoi(&time_now[0]);
    unsigned int minute = atoi(&time_now[3]);
    unsigned int second = atoi(&time_now[6]);

    const char date_now[] = __DATE__;  // Mmm dd yyyy
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    unsigned int month = 0;
    for (int i = 0; i < 12; i++) {
        if (strncmp(date_now, months[i], 3) == 0) {
            month = i + 1;
            break;
        }
    }
    unsigned int day = atoi(&date_now[3]);
    unsigned int year = atoi(&date_now[7]);

    char buffer[20];
    snprintf_P(
        buffer, sizeof(buffer), PSTR("%04d-%02d-%02d %02d:%02d:%02d"),
        year, month, day, hour, minute, second);

    return String(buffer);
}

unsigned long getUptime() {
    static unsigned long last_uptime = 0;
    static unsigned char uptime_overflows = 0;

    if (millis() < last_uptime) ++uptime_overflows;
    last_uptime = millis();
    unsigned long uptime_seconds = uptime_overflows * (UPTIME_OVERFLOW / 1000) + (last_uptime / 1000);

    return uptime_seconds;
}

#if HEARTBEAT_ENABLED

/* void heartbeat() {
    unsigned long uptime_seconds = getUptime();
    unsigned int free_heap = ESP.getFreeHeap();

    //DEBUG_MSG_P(PSTR("[MAIN] Time: %s\n"), (char *) NTP.getTimeDateString().c_str());

    DEBUG_MSG_P(PSTR("[MAIN] Uptime: %ld seconds\n"), uptime_seconds);
    DEBUG_MSG_P(PSTR("[MAIN] Free heap: %d bytes\n"), free_heap);

#if ENABLE_ADC_VCC
    DEBUG_MSG_P(PSTR("[MAIN] Power: %d mV\n"), ESP.getVcc());
#endif

#if NTP_SUPPORT
    if (ntpSynced()) DEBUG_MSG_P(PSTR("[MAIN] Time: %s\n"), (char *)ntpDateTime().c_str());
#endif

    if (!canSendMqttMessage()) {
        return;
    }

#if (MQTT_REPORT_STATUS)
    // Dont Encrypt and Dont Stack
    mqttSend(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE, false, true, false, true);
#endif

#if (MQTT_REPORT_LAST_RESET_REASON)
    if (!mqtt_firstbeat_reported) {
        // mqttSend(MQTT_TOPIC_RESET_REASON, ESP.getResetReason().c_str(), true);
        unsigned char custom_reset = resetReason();
        char last_reset_rsn_buffer[45];
        if (custom_reset > 0) {
            strcpy_P(last_reset_rsn_buffer, custom_reset_string[custom_reset - 1]);

        } else {
            strcpy(last_reset_rsn_buffer, (char *)ESP.getResetReason().c_str());
            // DEBUG_MSG_P(PSTR("Last reset reason: %s\n"), (char *) ESP.getResetReason().c_str());
        }
        mqttSend(MQTT_TOPIC_RESET_REASON, String(last_reset_rsn_buffer).c_str(), true);
    }
#endif

// #if (MQTT_REPORT_INTERVAL)
//     if(!mqtt_firstbeat_reported){
//         mqttSend(MQTT_TOPIC_INTERVAL, HEARTBEAT_INTERVAL / 1000);
//     }
// #endif
#if (MQTT_REPORT_APP)
    if (!mqtt_firstbeat_reported) {
        mqttSend(MQTT_TOPIC_APP, APP_NAME, true);
        mqttSend(MQTT_TOPIC_BUILD_DATE, __DATE__, true);
    }
#endif
#if (MQTT_REPORT_VERSION)
    if (!mqtt_firstbeat_reported) {
        mqttSend(MQTT_TOPIC_VERSION, APP_VERSION, true);
#if HAVE_SIBLING_CONTROLLER
        mqttSend(MQTT_TOPIC_ATMVERSION, getSetting("ATMVERSION", ATM_DEFAULT_VERSION).c_str(), true);
#endif
    }
#endif
#if (MQTT_REPORT_HOSTNAME)
    if (!mqtt_firstbeat_reported) {
        mqttSend(MQTT_TOPIC_HOSTNAME, getSetting("hostname").c_str(), true);
    }
#endif
#if (MQTT_REPORT_IP)
    if (!wifi_connected_beat) {
        mqttSend(MQTT_TOPIC_IP, getIP().c_str(), true);
    }
#endif
#if (MQTT_REPORT_MAC)
    if (!mqtt_firstbeat_reported) {
        mqttSend(MQTT_TOPIC_MAC, WiFi.macAddress().c_str(), true);
    }
#endif
#if (MQTT_REPORT_NETWORK)
    if (!wifi_connected_beat) {
        mqttSend(MQTT_TOPIC_NETWORK, getNetwork().c_str(), true);
    }
#endif
#if (MQTT_REPORT_RSSI)
    if (!wifi_connected_beat) {
        mqttSend(MQTT_TOPIC_RSSI, String(WiFi.RSSI()).c_str(), true);
    }
#endif
#if (MQTT_REPORT_UPTIME)
    mqttSend(MQTT_TOPIC_UPTIME, String(uptime_seconds).c_str(), true);
#if ENABLE_INFLUXDB
    influxDBSend(MQTT_TOPIC_UPTIME, String(uptime_seconds).c_str());
#endif
#endif
#if (MQTT_REPORT_FREEHEAP)
    mqttSend(MQTT_TOPIC_FREEHEAP, String(free_heap).c_str(), true);
#if ENABLE_INFLUXDB
    influxDBSend(MQTT_TOPIC_FREEHEAP, String(free_heap).c_str());
#endif
#endif
#if (LOADAVG_REPORT)
    mqttSend(MQTT_TOPIC_LOADAVG, String(systemLoadAverage()).c_str(), true);
#endif

#if MQTT_REPORT_RELAY
    // Send All Relay Status also only once after its connected to wifi. otherwise no need.
    // if(!wifi_connected_beat){
    relayAllMQTT();
    // }
#endif
#if MQTT_REPORT_CONFIG
    if (!config_mqtt_reported) {
        config_mqtt_reported = settingsMQTT();
    }
#endif
#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
#if (MQTT_REPORT_COLOR)
    mqttSend(MQTT_TOPIC_COLOR, lightColor().c_str(), true);
#endif

#endif
#if (MQTT_REPORT_VCC)
#if ENABLE_ADC_VCC
    if (!wifi_connected_beat) {
        mqttSend(MQTT_TOPIC_VCC, String(ESP.getVcc()).c_str(), true);
    }
#endif
#endif
} */

void heartbeat_new() {
    // Serial.println("Heartbeat New");

    unsigned long uptime_seconds = getUptime();
    unsigned int free_heap = ESP.getFreeHeap();

    //DEBUG_MSG_P(PSTR("[MAIN] Time: %s\n"), (char *) NTP.getTimeDateString().c_str());

    DEBUG_MSG_P(PSTR("[MAIN] Uptime: %ld seconds\n"), uptime_seconds);
    DEBUG_MSG_P(PSTR("[MAIN] Free heap: %d bytes\n"), free_heap);

#if ENABLE_ADC_VCC
    DEBUG_MSG_P(PSTR("[MAIN] Power: %d mV\n"), ESP.getVcc());
#endif

#if NTP_SUPPORT
    if (ntpSynced()) DEBUG_MSG_P(PSTR("[MAIN] Time: %s\n"), (char *)ntpDateTime().c_str());
#endif

    if (!canSendMqttMessage()) {
        return;
    }

#if (MQTT_REPORT_STATUS)
    // Dont Encrypt and Dont Stack
    mqttSend(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE, false, true, false, true);
#endif

    StaticJsonDocument<300> jsonBuffer;
    JsonObject root = jsonBuffer.to<JsonObject>();

#if (MQTT_REPORT_LAST_RESET_REASON)
    if (!reset_reason_reported) {
        uint8_t custom_reset = resetReason();
        if (custom_reset > 0) {
            root[MQTT_TOPIC_RESET_REASON] = custom_reset;
        } else {
            char last_reset_rsn_buffer[45] = "";
            uint8_t rsn = 0;
            strncpy(last_reset_rsn_buffer, (char *)ESP.getResetReason().c_str(),  sizeof(last_reset_rsn_buffer) -1);
            if (strcmp(last_reset_rsn_buffer, "Power on") == 0) {
                rsn = 1;
            } else if (strcmp(last_reset_rsn_buffer, "Hardware Watchdog") == 0) {
                rsn = 2;
            } else if (strcmp(last_reset_rsn_buffer, "Exception") == 0) {
                rsn = 3;
            } else if (strcmp(last_reset_rsn_buffer, "Software Watchdog") == 0) {
                rsn = 4;
            } else if (strcmp(last_reset_rsn_buffer, "Software/System restart") == 0) {
                rsn = 5;
            } else if (strcmp(last_reset_rsn_buffer, "Deep-Sleep Wake") == 0) {
                rsn = 6;
            } else if (strcmp(last_reset_rsn_buffer, "External System") == 0) {
                rsn = 7;
            }
            root[MQTT_TOPIC_RESET_REASON] = rsn + 20;
        }
    }
#endif

#if (MQTT_REPORT_APP)
    if (!mqtt_firstbeat_reported) {
        root[MQTT_TOPIC_APP] = APP_NAME;
        // root[MQTT_TOPIC_BUILD_DATE] = __DATE__;
        // mqttSend(MQTT_TOPIC_APP, APP_NAME, true);
        // mqttSend(MQTT_TOPIC_BUILD_DATE, __DATE__, true);
    }
#endif
#if (MQTT_REPORT_VERSION)
    if (!mqtt_firstbeat_reported) {
        root[MQTT_TOPIC_VERSION] = APP_VERSION;
        root[MQTT_TOPIC_REVISION] = APP_REVISION;
#if HAVE_SIBLING_CONTROLLER
        root[MQTT_TOPIC_ATMVERSION] = getSetting("ATMVERSION", ATM_DEFAULT_VERSION);
#endif
        // mqttSend(MQTT_TOPIC_VERSION, APP_VERSION, true);
    }
#endif
#if (MQTT_REPORT_HOSTNAME)
    if (!mqtt_firstbeat_reported) {
        root[MQTT_TOPIC_HOSTNAME] = getSetting("hostname");
        // mqttSend(MQTT_TOPIC_HOSTNAME, getSetting("hostname").c_str(), true);
    }
#endif
#if (MQTT_REPORT_IP)
    if (!wifi_connected_beat) {
        // root[MQTT_TOPIC_IP] = getIP();  // need device local ip
        root[MQTT_TOPIC_IP] = Network.localIP(); // TOSO_S1
        // mqttSend(MQTT_TOPIC_IP, getIP().c_str(),true);
    }
#endif
#if (MQTT_REPORT_MAC)
    if (!mqtt_firstbeat_reported) {
        root[MQTT_TOPIC_MAC] = WiFi.macAddress();
        // mqttSend(MQTT_TOPIC_MAC, WiFi.macAddress().c_str(),true);
    }
#endif
#if (MQTT_REPORT_NETWORK)
    if (!wifi_connected_beat) {
        // root[MQTT_TOPIC_NETWORK] = getNetwork(); // need device ssid
        // serializeNetworks(root[MQTT_TOPIC_NETWORK]);; // TOSO_S1
        // mqttSend(MQTT_TOPIC_NETWORK, getNetwork().c_str(),true);
    }
#endif
#if (MQTT_REPORT_RSSI)
    if (!wifi_connected_beat) {
        root[MQTT_TOPIC_RSSI] = String(WiFi.RSSI());
        // mqttSend(MQTT_TOPIC_RSSI, String(WiFi.RSSI()).c_str(),true);
    }
#endif
#if (MQTT_REPORT_UPTIME)
    root[MQTT_TOPIC_UPTIME] = String(uptime_seconds);
// mqttSend(MQTT_TOPIC_UPTIME, String(uptime_seconds).c_str(), true);
#if ENABLE_INFLUXDB
// influxDBSend(MQTT_TOPIC_UPTIME, String(uptime_seconds).c_str());
#endif
#endif
#if (MQTT_REPORT_FREEHEAP)
    // Do not send it for the first time - Mostly, it'll have good free heap only. 
    if(wifi_connected_beat){
        root[MQTT_TOPIC_FREEHEAP] = String(free_heap);
    }
// mqttSend(MQTT_TOPIC_FREEHEAP, String(free_heap).c_str(), true);
#if ENABLE_INFLUXDB
// influxDBSend(MQTT_TOPIC_FREEHEAP, String(free_heap).c_str());
#endif
#endif
#if (LOADAVG_REPORT)
    root[MQTT_TOPIC_LOADAVG] = String(systemLoadAverage());
    // mqttSend(MQTT_TOPIC_LOADAVG, String(systemLoadAverage()).c_str(), true);
#endif

#if MQTT_REPORT_RELAY
    // Send All Relay Status also only once after its connected to wifi. otherwise no need.
    // if(!wifi_connected_beat){
    if(relayCount() > 0){
        relayAllMQTT();
    }
    // }
#endif
#if MQTT_REPORT_CONFIG
    if (!config_mqtt_reported) {
        config_mqtt_reported = settingsMQTT();
    }
#endif
#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
#if (MQTT_REPORT_COLOR)
    root[MQTT_TOPIC_COLOR] = lightColor();
    // mqttSend(MQTT_TOPIC_COLOR, lightColor().c_str(), true);
#endif

#endif
#if (MQTT_REPORT_VCC)
#if ENABLE_ADC_VCC
    if (!wifi_connected_beat) {
        root[MQTT_TOPIC_VCC] = String(ESP.getVcc());
        // mqttSend(MQTT_TOPIC_VCC, String(ESP.getVcc()).c_str(), true);
    }
#endif
#endif
    String output;
    // root.printTo(output);
    serializeJson(root, output);
    if (mqttSend(MQTT_TOPIC_BEAT, output.c_str(), false)) {
        mqtt_firstbeat_reported = true;
        wifi_connected_beat = true;
        reset_reason_reported = true;
        // DEBUG_MSG_P(PSTR("No Surprise, We sent the heartbeat \n"));
    }
}

#endif  /// HEARTBEAT_ENABLED
// MQTT REPORTING EVENT
bool reportEventToMQTT(const char *ev, const char *meta) {
    if (!canSendMqttMessage()) return false;
    char messageBuffer[200] = "";
    StaticJsonDocument<256> root;
    root["e"] = ev;
    root["e_m"] = meta;
    serializeJson(root, messageBuffer, sizeof(messageBuffer));
    return mqttSend(MQTT_TOPIC_EVENT_LARGE, messageBuffer, false, false, true,
                    false);
}

bool reportEventToMQTT(const char * ev, JsonObject& meta){
    if(!canSendMqttMessage()) return false;
    char messageBuffer[512] = "";
    StaticJsonDocument<512> root;
    root["e"] = ev;
    root["e_m_json"] = meta;
    serializeJson(root, messageBuffer, sizeof(messageBuffer));
    return mqttSend(MQTT_TOPIC_EVENT_LARGE, messageBuffer, false, false, true, false);
}

#if ESP_ARDUINO_CORE_IS == 1
extern "C" uint32_t _SPIFFS_start;
extern "C" uint32_t _SPIFFS_end;
#elif ESP_ARDUINO_CORE_IS == 2
extern "C" uint32_t _FS_start;
extern "C" uint32_t _FS_end;
#endif

unsigned long info_ota_space() {
    return (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
}

unsigned int sectors(size_t size) {
    return (int)(size + SPI_FLASH_SEC_SIZE - 1) / SPI_FLASH_SEC_SIZE;
}
unsigned int info_bytes2sectors(size_t size) {
    return (int)(size + SPI_FLASH_SEC_SIZE - 1) / SPI_FLASH_SEC_SIZE;
}

unsigned long info_filesystem_space() {
// #if ESP_ARDUINO_CORE_IS == 1
//     return ((uint32_t)&_SPIFFS_end - (uint32_t)&_SPIFFS_start);
// #elif ESP_ARDUINO_CORE_IS == 2
//     return ((uint32_t)&_FS_end - (uint32_t)&_FS_start);
// #endif
    return 0;
}

unsigned long info_eeprom_space() {
    return EEPROMr.reserved() * SPI_FLASH_SEC_SIZE;
}

void _info_print_memory_layout_line(const char *name, unsigned long bytes, bool reset) {
    static unsigned long index = 0;
    if (reset) index = 0;
    if (0 == bytes) return;
    unsigned int _sectors = info_bytes2sectors(bytes);
    DEBUG_MSG_P(PSTR("[INIT] %-20s: %8lu bytes / %4d sectors (%4d to %4d)\n"), name, bytes, _sectors, index, index + _sectors - 1);
    index += _sectors;
}

void _info_print_memory_layout_line(const char *name, unsigned long bytes) {
    _info_print_memory_layout_line(name, bytes, false);
}
void infoMemory(const char *name, unsigned int total_memory, unsigned int free_memory) {
    DEBUG_MSG_P(
        PSTR("[MAIN] %-6s: %5u bytes initially | %5u bytes used (%2u%%) | %5u bytes free (%2u%%)\n"),
        name,
        total_memory,
        total_memory - free_memory,
        100 * (total_memory - free_memory) / total_memory,
        free_memory,
        100 * free_memory / total_memory);
}

void info() {
    DEBUG_MSG_P(PSTR("\n\n"));
    DEBUG_MSG_P(PSTR("[INIT] %s %s\n"), (char *)APP_NAME, (char *)APP_VERSION);
    DEBUG_MSG_P(PSTR("[INIT] %s\n"), (char *)APP_AUTHOR);
    DEBUG_MSG_P(PSTR("[INIT] %s\n\n"), (char *)APP_WEBSITE);
    DEBUG_MSG_P(PSTR("[INIT] CPU chip ID: 0x%06X\n"), ESP.getChipId());
    DEBUG_MSG_P(PSTR("[INIT] CPU frequency: %u MHz\n"), ESP.getCpuFreqMHz());
    DEBUG_MSG_P(PSTR("[INIT] SDK version: %s\n"), ESP.getSdkVersion());
    DEBUG_MSG_P(PSTR("[INIT] Core version: %s\n"), getCoreVersion().c_str());
    DEBUG_MSG_P(PSTR("[INIT] Core revision: %s\n"), getCoreRevision().c_str());
    DEBUG_MSG_P(PSTR("\n"));
    DEBUG_MSG_P(PSTR("[INIT] Sketch MD5 : %s \n"), ESP.getSketchMD5().c_str());
    DEBUG_MSG_P(PSTR("\n"));
    // -------------------------------------------------------------------------

    FlashMode_t mode = ESP.getFlashChipMode();
    DEBUG_MSG_P(PSTR("[INIT] Flash chip ID: 0x%06X\n"), ESP.getFlashChipId());
    DEBUG_MSG_P(PSTR("[INIT] Flash speed: %u Hz\n"), ESP.getFlashChipSpeed());
    DEBUG_MSG_P(PSTR("[INIT] Flash mode: %s\n"), mode == FM_QIO ? "QIO" : mode == FM_QOUT ? "QOUT" : mode == FM_DIO ? "DIO" : mode == FM_DOUT ? "DOUT" : "UNKNOWN");
    DEBUG_MSG_P(PSTR("\n"));

    _info_print_memory_layout_line("Flash size (CHIP)", ESP.getFlashChipRealSize(), true);
    _info_print_memory_layout_line("Flash size (SDK)", ESP.getFlashChipSize(), true);
    _info_print_memory_layout_line("Reserved", 1 * SPI_FLASH_SEC_SIZE, true);
    _info_print_memory_layout_line("Firmware size", ESP.getSketchSize());
    _info_print_memory_layout_line("Max OTA size", info_ota_space());
    _info_print_memory_layout_line("SPIFFS size", info_filesystem_space());
    _info_print_memory_layout_line("EEPROM size", info_eeprom_space());
    _info_print_memory_layout_line("Reserved", 4 * SPI_FLASH_SEC_SIZE);
    DEBUG_MSG_P(PSTR("\n"));

    // DEBUG_MSG_P(PSTR("[INIT] EEPROM sectors: %s\n"), (char *)eepromSectors().c_str()); TODO_S2
    DEBUG_MSG_P(PSTR("\n"));
    DEBUG_MSG_P(PSTR("[INIT] Firmware MD5: %s\n"), (char *) ESP.getSketchMD5().c_str());
    // -------------------------------------------------------------------------

#if SPIFFS_SUPPORT
    FSInfo fs_info;
    bool fs = SPIFFS.info(fs_info);
    if (fs) {
        DEBUG_MSG_P(PSTR("[INIT] SPIFFS total size: %8u bytes / %4d sectors\n"), fs_info.totalBytes, sectors(fs_info.totalBytes));
        DEBUG_MSG_P(PSTR("[INIT]        used size:  %8u bytes\n"), fs_info.usedBytes);
        DEBUG_MSG_P(PSTR("[INIT]        block size: %8u bytes\n"), fs_info.blockSize);
        DEBUG_MSG_P(PSTR("[INIT]        page size:  %8u bytes\n"), fs_info.pageSize);
        DEBUG_MSG_P(PSTR("[INIT]        max files:  %8u\n"), fs_info.maxOpenFiles);
        DEBUG_MSG_P(PSTR("[INIT]        max length: %8u\n"), fs_info.maxPathLength);
    } else {
        DEBUG_MSG_P(PSTR("[INIT] No SPIFFS partition\n"));
    }
    DEBUG_MSG_P(PSTR("\n"));
#endif

    // -------------------------------------------------------------------------
    #if defined(SMART_DONGLE)
    DEBUG_MSG_P(PSTR("[INIT] DeviceId2 : %s\n"),getSetting("deviceId2").c_str());
    #endif
    DEBUG_MSG_P(PSTR("[INIT] BOARD: %s\n"), getBoardName().c_str());
    DEBUG_MSG_P(PSTR("[INIT] SUPPORT:"));

#if ALEXA_SUPPORT
    DEBUG_MSG_P(PSTR(" ALEXA"));
#endif
#if BROKER_SUPPORT
    DEBUG_MSG_P(PSTR(" BROKER"));
#endif
#if DEBUG_SERIAL_SUPPORT
    DEBUG_MSG_P(PSTR(" DEBUG_SERIAL"));
#endif
#if DEBUG_TELNET_SUPPORT
    DEBUG_MSG_P(PSTR(" DEBUG_TELNET"));
#endif
#if DEBUG_UDP_SUPPORT
    DEBUG_MSG_P(PSTR(" DEBUG_UDP"));
#endif
#if DOMOTICZ_SUPPORT
    DEBUG_MSG_P(PSTR(" DOMOTICZ"));
#endif
#if HOMEASSISTANT_SUPPORT
    DEBUG_MSG_P(PSTR(" HOMEASSISTANT"));
#endif
#if I2C_SUPPORT
    DEBUG_MSG_P(PSTR(" I2C"));
#endif
#if INFLUXDB_SUPPORT
    DEBUG_MSG_P(PSTR(" INFLUXDB"));
#endif
#if LLMNR_SUPPORT
    DEBUG_MSG_P(PSTR(" LLMNR"));
#endif
#if MDNS_SERVER_SUPPORT
    DEBUG_MSG_P(PSTR(" MDNS_SERVER"));
#endif
#if MDNS_CLIENT_SUPPORT
    DEBUG_MSG_P(PSTR(" MDNS_CLIENT"));
#endif
#if NETBIOS_SUPPORT
    DEBUG_MSG_P(PSTR(" NETBIOS"));
#endif
#if NOFUSS_SUPPORT
    DEBUG_MSG_P(PSTR(" NOFUSS"));
#endif
#if NTP_SUPPORT
    DEBUG_MSG_P(PSTR(" NTP"));
#endif
#if RF_SUPPORT
    DEBUG_MSG_P(PSTR(" RF"));
#endif
#if SCHEDULER_SUPPORT
    DEBUG_MSG_P(PSTR(" SCHEDULER"));
#endif
#if SENSOR_SUPPORT
    DEBUG_MSG_P(PSTR(" SENSOR"));
#endif
#if SPIFFS_SUPPORT
    DEBUG_MSG_P(PSTR(" SPIFFS"));
#endif
#if SSDP_SUPPORT
    DEBUG_MSG_P(PSTR(" SSDP"));
#endif
#if TELNET_SUPPORT
    DEBUG_MSG_P(PSTR(" TELNET"));
#endif
#if TERMINAL_SUPPORT
    DEBUG_MSG_P(PSTR(" TERMINAL"));
#endif
#if HTTPSERVER_SUPPORT
    DEBUG_MSG_P(PSTR(" HTTPSERVER"));
#endif
#if WEB_SUPPORT
    DEBUG_MSG_P(PSTR(" WEB"));
#endif

// #if SENSOR_SUPPORT // TODO_S2

//     DEBUG_MSG_P(PSTR("\n[INIT] SENSORS:"));

// #if ANALOG_SUPPORT
//     DEBUG_MSG_P(PSTR(" ANALOG"));
// #endif
// #if BMX280_SUPPORT
//     DEBUG_MSG_P(PSTR(" BMX280"));
// #endif
// #if DALLAS_SUPPORT
//     DEBUG_MSG_P(PSTR(" DALLAS"));
// #endif
// #if DHT_SUPPORT
//     DEBUG_MSG_P(PSTR(" DHTXX"));
// #endif
// #if DIGITAL_SUPPORT
//     DEBUG_MSG_P(PSTR(" DIGITAL"));
// #endif
// #if ECH1560_SUPPORT
//     DEBUG_MSG_P(PSTR(" ECH1560"));
// #endif
// #if EMON_ADC121_SUPPORT
//     DEBUG_MSG_P(PSTR(" EMON_ADC121"));
// #endif
// #if EMON_ADS1X15_SUPPORT
//     DEBUG_MSG_P(PSTR(" EMON_ADX1X15"));
// #endif
// #if EMON_ANALOG_SUPPORT
//     DEBUG_MSG_P(PSTR(" EMON_ANALOG"));
// #endif
// #if EVENTS_SUPPORT
//     DEBUG_MSG_P(PSTR(" EVENTS"));
// #endif
// #if HLW8012_SUPPORT
//     DEBUG_MSG_P(PSTR(" HLW8012"));
// #endif
// #if MHZ19_SUPPORT
//     DEBUG_MSG_P(PSTR(" MHZ19"));
// #endif
// #if PMSX003_SUPPORT
//     DEBUG_MSG_P(PSTR(" PMSX003"));
// #endif
// #if SHT3X_I2C_SUPPORT
//     DEBUG_MSG_P(PSTR(" SHT3X_I2C"));
// #endif
// #if SI7021_SUPPORT
//     DEBUG_MSG_P(PSTR(" SI7021"));
// #endif
// #if V9261F_SUPPORT
//     DEBUG_MSG_P(PSTR(" V9261F"));
// #endif

// #endif  // SENSOR_SUPPORT

    DEBUG_MSG_P(PSTR("\n\n"));

    // -------------------------------------------------------------------------

    unsigned char reason = resetReason();
    if (reason > 0) {
        char buffer[32];
        // strcpy_P(buffer, custom_reset_string[reason - 1]); TODO_S2
        DEBUG_MSG_P(PSTR("[INIT] Last reset reason: %s\n"), buffer);
    } else {
        DEBUG_MSG_P(PSTR("[INIT] Last reset reason: %s\n"), (char *)ESP.getResetReason().c_str());
    }

    DEBUG_MSG_P(PSTR("[INIT] Settings size: %u bytes\n"), settingsSize());
    DEBUG_MSG_P(PSTR("[INIT] Free heap: %u bytes\n"), getFreeHeap());
#if ADC_VCC_ENABLED
    DEBUG_MSG_P(PSTR("[INIT] Power: %u mV\n"), ESP.getVcc());
#endif

    // DEBUG_MSG_P(PSTR("[INIT] Power saving delay value: %lu ms\n"), _loopDelay); TODO_S2
#if SYSTEM_CHECK_ENABLED
    if (!systemCheck()) DEBUG_MSG_P(PSTR("\n[INIT] Device is in SAFE MODE\n"));
#endif

    DEBUG_MSG_P(PSTR("\n"));
}

void infoProduction() {
    Serial.print("[App]: ");
    Serial.println(APP_NAME);

    Serial.print("[Version]: ");
    Serial.println(APP_VERSION);

    Serial.print("[Device]: ");
    Serial.println(DEVICE);

    Serial.print("[Cv]: ");
    Serial.println(SETTINGS_VERSION);

    Serial.print("[Sv]: ");
    Serial.println(SENSORS_VERSION);

    Serial.print("[AppRevision]: ");
    Serial.println(APP_REVISION);

    Serial.print("[ESPCore]: ");
    Serial.println(getCoreVersion());

    Serial.print("[ESPSDK]: ");
    Serial.println(ESP.getSdkVersion());

    Serial.print("[ChipId]: ");
    Serial.println(ESP.getChipId());

    Serial.print("[Identifier]: ");
    Serial.println(getSetting("identifier"));
    
    #if defined(SMART_DONGLE)
    Serial.print("[DeviceId2]: ");
    Serial.println(getSetting("deviceId2"));
    #endif

    Serial.print("[MD5]: ");
    Serial.println(ESP.getSketchMD5());

    if (WiFi.getMode() == WIFI_AP_STA) {
        SERIAL_SEND("[WIFI] MODE AP + STA --------------------------------\n");
    } else if (WiFi.getMode() == WIFI_AP) {
        SERIAL_SEND("[WIFI] MODE AP --------------------------------------\n");
    } else if (WiFi.getMode() == WIFI_STA) {
        SERIAL_SEND("[WIFI] MODE STA -------------------------------------\n");
    } else {
        SERIAL_SEND("[WIFI] MODE OFF -------------------------------------\n");
        SERIAL_SEND("[WIFI] No connection\n");
    }

    if ((WiFi.getMode() & WIFI_AP) == WIFI_AP) {
        // SERIAL_SEND("[WIFI] SSID %s\n", jw.getAPSSID().c_str()); TODO_S2
        // SERIAL_SEND("[WIFI] PASS %s\n", getSetting("adminPass", ADMIN_PASS).c_str());
        SERIAL_SEND("[WIFI] IP   %s\n", WiFi.softAPIP().toString().c_str());
        SERIAL_SEND("[WIFI] MAC  %s\n", WiFi.softAPmacAddress().c_str());
    }

    if ((WiFi.getMode() & WIFI_STA) == WIFI_STA) {
        SERIAL_SEND("[WIFI] SSID %s\n", WiFi.SSID().c_str());
        SERIAL_SEND("[WIFI] IP   %s\n", WiFi.localIP().toString().c_str());
        SERIAL_SEND("[WIFI] MAC  %s\n", WiFi.macAddress().c_str());
        SERIAL_SEND("[WIFI] GW   %s\n", WiFi.gatewayIP().toString().c_str());
        SERIAL_SEND("[WIFI] DNS  %s\n", WiFi.dnsIP().toString().c_str());
        SERIAL_SEND("[WIFI] MASK %s\n", WiFi.subnetMask().toString().c_str());
        SERIAL_SEND("[WIFI] HOST %s\n", WiFi.hostname().c_str());
        // jw.setWifiMode(WIFI_STA);
    }
    SERIAL_SEND("[WIFI] ----------------------------------------------\n");
}

String getAdminPass() {
    return getSetting("adminPass", ADMIN_PASS);
}


// -----------------------------------------------------------------------------

unsigned char resetReason() {
    static unsigned char status = 255;
    if (status == 255) {
        status = EEPROMr.read(EEPROM_CUSTOM_RESET);
        if (status > 0) resetReason(0);
        if (status > CUSTOM_RESET_MAX) status = 0;
    }
    return status;
}

void resetReason(unsigned char reason) {
    EEPROMr.write(EEPROM_CUSTOM_RESET, reason);
    // EEPROMr.commit();
    saveSettings();
}

void reset(unsigned char reason) {
    // resetReason(reason);
    // ESP.restart();
    _reset_reason = reason;
}
void resetActual() {
    ESP.restart();
}

void deferredReset(unsigned long delay, unsigned char reason) {
    resetReason(reason);
    // relaySave(); TODO_S1
    _defer_reset.once_ms(delay, reset, reason);
}

bool checkNeedsReset() {
    return _reset_reason > 0;
}



// -----------------------------------------------------------------------------

// --------------------------- Atmega Related --------------------------------------------------
// TODO_S2
// void controllerversionRetrieve() {
//     #if RELAY_PROVIDER == RELAY_PROVIDER_KU && (KU_SERIAL_SUPPORT)
//         //TODO: implement new command for version here
//     #else
//     Serial.println("");
//     SERIAL_SEND_P(PSTR("%s %s \n"), COMMAND_CONFIG, COMMAND_VINFO);
//     #endif
// }

#if HAVE_SIBLING_CONTROLLER
void setAtmVersion(String version) {
    if (!getSetting("ATMVERSION", "").equals(version)) {
        setSetting("ATMVERSION", version);
        saveSettings();
    }
}

String getAtmVersion() {
    return getSetting("ATMVERSION", ATM_DEFAULT_VERSION);
}
#endif

// -----------------------------------------------------------------------------

// --------------------------- JSON Helpers TODO_S2--------------------------------------------------
// JsonVariant cloneJsonVariant(JsonBuffer &jb, JsonVariant prototype) {
//     if (prototype.is<JsonObject>()) {
//         const JsonObject &protoObj = prototype;
//         JsonObject &newObj = jb.createObject();
//         for (const auto &kvp : protoObj) {
//             newObj[jb.strdup(kvp.key)] = cloneJsonVariant(jb, kvp.value);
//         }
//         return newObj;
//     }

//     if (prototype.is<JsonArray>()) {
//         const JsonArray &protoArr = prototype;
//         JsonArray &newArr = jb.createArray();
//         for (const auto &elem : protoArr) {
//             newArr.add(cloneJsonVariant(jb, elem));
//         }
//         return newArr;
//     }

//     if (prototype.is<char *>()) {
//         return jb.strdup(prototype.as<const char *>());
//     }

//     return prototype;
// }

// -----------------------------------------------------------------------------

// #if SYSTEM_CHECK_ENABLED

// // Call this method on boot with start=true to increase the crash counter
// // Call it again once the system is stable to decrease the counter
// // If the counter reaches SYSTEM_CHECK_MAX then the system is flagged as unstable
// // setting _systemOK = false;
// //
// // An unstable system will only have serial access, WiFi in AP mode and OTA

// bool _systemStable = true;

// void systemCheck(bool stable) {
//     unsigned char value = EEPROM.read(EEPROM_CRASH_COUNTER);
//     if (stable) {
//         value = 0;
//         DEBUG_MSG_P(PSTR("[MAIN] System OK\n"));
//     } else {
//         if (++value > SYSTEM_CHECK_MAX) {
//             _systemStable = false;
//             value = 0;
//             DEBUG_MSG_P(PSTR("[MAIN] System UNSTABLE\n"));
//         }
//     }
//     EEPROM.write(EEPROM_CRASH_COUNTER, value);
//     EEPROM.commit();
// }

// bool systemCheck() {
//     return _systemStable;
// }

// void systemCheckLoop() {
//     static bool checked = false;
//     if (!checked && (millis() > SYSTEM_CHECK_TIME)) {
//         // Check system as stable
//         systemCheck(true);
//         checked = true;
//     }
// }

// #endif

char *ltrim(char *s) {
    char *p = s;
    while ((unsigned char)*p == ' ') ++p;
    return p;
}

double roundTo(double num, unsigned char positions) {
    double multiplier = 1;
    while (positions-- > 0) multiplier *= 10;
    return round(num * multiplier) / multiplier;
}
void nice_delay(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) delay(1);
}

void very_nice_delay(uint32_t usec) {
    for (; usec > MAX_ACCURATE_USEC_DELAY; usec -= MAX_ACCURATE_USEC_DELAY)
        delayMicroseconds(MAX_ACCURATE_USEC_DELAY);
    delayMicroseconds(static_cast<uint16_t>(usec));
}

// Wrestle Arduino into making strings that may contain zeros.
// Taken inspiration from Embedis Library
String rightString(const char *s, size_t length) {
    String str;
    str.reserve(length);
    for (size_t i = 0; i < length; i++) str += s[i];
    return str;
}

bool compareTrimmedStrings(const char* str1, const char* str2){
    uint8_t l1 = trimmedLength(str1);
    uint8_t l2 = trimmedLength(str2);
    if(l1!=l2){
        return false;
    }
    for(uint8_t i = 0; i < l1; i++){
        if(str1[i] != str2[i]) return false;
    }
    return true;

}

uint8_t trimmedLength(const char* str){
    uint8_t full = strlen(str);
    if(!full) return 0;
    while(full--){
        if(*(str + full) != ' '){
            // cout << *(str + full) << endl;
            return ++full;
        }
        
    }
    return 0;
}

#if ASYNC_TCP_SSL_ENABLED || HTTPSERVER_USE_SSL

bool sslCheckFingerPrint(const char *fingerprint) {
    return (strlen(fingerprint) == 59);
}

bool sslFingerPrintArray(const char *fingerprint, unsigned char *bytearray) {
    // check length (20 2-character digits ':' or ' ' separated => 20*2+19 = 59)
    if (!sslCheckFingerPrint(fingerprint)) return false;

    // walk the fingerprint
    for (unsigned int i = 0; i < 20; i++) {
        bytearray[i] = strtol(fingerprint + 3 * i, NULL, 16);
    }

    return true;
}

bool sslFingerPrintChar(const char *fingerprint, char *destination) {
    // check length (20 2-character digits ':' or ' ' separated => 20*2+19 = 59)
    if (!sslCheckFingerPrint(fingerprint)) return false;

    // copy it
    strncpy(destination, fingerprint, 59);

    // walk the fingerprint replacing ':' for ' '
    for (unsigned char i = 0; i < 59; i++) {
        if (destination[i] == ':') destination[i] = ' ';
    }

    return true;
}

#endif

bool debouncedRead(uint8_t pin, bool status, unsigned int time){
    unsigned int ones = 0;
    unsigned int zeroes = 0;
    unsigned long start = millis();
    while (millis() >= start && (millis() - start < time))
    {
        bool switch_status = digitalRead(pin);
        switch_status == HIGH ? ones++ : zeroes++;
    }
    // DEBUG_MSG_P(PSTR("Zeroes : %d, Ones %d \n"), zeroes, ones);
    return status ? ones > zeroes : zeroes > ones;
}

// A very novice function - which can be improved in future. 
bool isValidMacAddress(char * mac){
    if(strlen(mac) != 12){
        return false;
    }
    return true;
}

// Imported from 
// https://github.com/gmag11/EnigmaIOT/blob/2fd868448cb674eeb9c8ee9e5aa50e9521775982/src/helperFunctions.cpp#L104

uint8_t* str2mac (const char* macAddrString, uint8_t* macBytes) {
	const char cSep = ':';

	if (!macBytes) {
		return NULL;
	}

	for (int i = 0; i < 6; ++i) {
		unsigned int iNumber = 0;
		char ch;

		//Convert letter into lower case.
		ch = tolower (*macAddrString++);

		if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f')) {
			return NULL;
		}

		//Convert into number. 
		//  a. If character is digit then ch - '0'
		//	b. else (ch - 'a' + 10) it is done 
		//	      because addition of 10 takes correct value.
		iNumber = isdigit (ch) ? (ch - '0') : (ch - 'a' + 10);
		ch = tolower (*macAddrString);

		if ((i < 5 && ch != cSep) ||
			(i == 5 && ch != '\0' && !isspace (ch))) {
			++macAddrString;

			if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f')) {
				return NULL;
			}

			iNumber <<= 4;
			iNumber += isdigit (ch) ? (ch - '0') : (ch - 'a' + 10);
			ch = *macAddrString;

			if (i < 5 && ch != cSep) {
				return NULL;
			}
		}
		/* Store result.  */
		macBytes[i] = (unsigned char)iNumber;
		/* Skip cSep.  */
		++macAddrString;
	}
	return macBytes;
}

/*
    eepromrotate.ino functions
*/

void eepromRotate(bool value) {
    // Enable/disable EEPROM rotation only if we are using more sectors than the
    // reserved by the memory layout
    if (EEPROMr.size() > EEPROMr.reserved()) {
        if (value) {
            DEBUG_MSG_P(PSTR("[EEPROM] Reenabling EEPROM rotation\n"));
        } else {
            DEBUG_MSG_P(PSTR("[EEPROM] Disabling EEPROM rotation\n"));
        }
        EEPROMr.rotate(value);
    }
}

String eepromSectors() {
    String response;
    for (uint32_t i = 0; i < EEPROMr.size(); i++) {
        if (i > 0) response = response + String(", ");
        response = response + String(EEPROMr.base() - i);
    }
    return response;
}
void eepromSectorsDebug() {
    DEBUG_MSG_P(PSTR("[MAIN] EEPROM sectors: %s\n"), (char *)eepromSectors().c_str());
    DEBUG_MSG_P(PSTR("[MAIN] EEPROM current: %lu\n"), eepromCurrent());
    Serial.println(eepromSectors());
    Serial.println(eepromCurrent());
}

uint32_t eepromCurrent() {
    return EEPROMr.current();
}
void eepromBackup(uint32_t index) {
    EEPROMr.backup(index);
}

#if DEBUG_SERIAL_SUPPORT

void _eepromInitCommands() {
    /* settingsRegisterCommand(F("EEPROMDUMP"), [](Embedis* e) {
        EEPROMr.dump(settingsSerial());
        DEBUG_MSG_P(PSTR("\n+OK\n"));
    });

    settingsRegisterCommand(F("FLASHDUMP"), [](Embedis* e) {
        if (e->argc < 2) {
            DEBUG_MSG_P(PSTR("-ERROR: Wrong arguments\n"));
            return;
        }
        uint32_t sector = String(e->argv[1]).toInt();
        uint32_t max = ESP.getFlashChipSize() / SPI_FLASH_SEC_SIZE;
        if (sector >= max) {
            DEBUG_MSG_P(PSTR("-ERROR: Sector out of range\n"));
            return;
        }
        EEPROMr.dump(settingsSerial(), sector);
        DEBUG_MSG_P(PSTR("\n+OK\n"));
    });
 */
}

#endif

// -----------------------------------------------------------------------------

void eepromSetup() {
#ifdef EEPROM_ROTATE_SECTORS
    EEPROMr.size(EEPROM_ROTATE_SECTORS);
#else
    // If the memory layout has more than one sector reserved use those,
    // otherwise calculate pool size based on memory size.
    if (EEPROMr.size() == 1) {
        if (EEPROMr.last() > 1000) {  // 4Mb boards
            EEPROMr.size(4);
        } else if (EEPROMr.last() > 250) {  // 1Mb boards
            EEPROMr.size(2);
        }
    }
#endif
#if DEVELOPEMEMT_MODE
    eepromSectorsDebug();
#endif

    EEPROMr.offset(EEPROM_ROTATE_DATA);
    EEPROMr.begin(EEPROM_SIZE);

#if DEBUG_SERIAL_SUPPORT
    _eepromInitCommands();
#endif
}

