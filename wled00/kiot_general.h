//------------------------------------------------------------------------------
// GENERAL
//------------------------------------------------------------------------------
#define MANUFACTURER "KIOT"

#ifndef SERIAL_BAUDRATE
#define SERIAL_BAUDRATE 9600
#endif

#define HOSTNAME DEVICE
#define BUFFER_SIZE 1024
#define HEARTBEAT_INTERVAL 300  // 5 Minutes
#define UPTIME_OVERFLOW 4294967295
// #define RESTARTDELAY 3 //minimal time in sec for button press to reset
#define HUMANPRESSDELAY 50  // the delay in ms untill the press should be handled as a normal push by human. Button debounce. !!! Needs to be less than RESTARTDELAY & RESETDELAY!!!
// #define RESETDELAY 8 //Minimal time in sec for button press to reset all settings and boot to config mode
#ifndef DELAY_INIT
#define DELAY_INIT 2  // 2 seconds
#endif

#define AUTO_SAVE 0  // Keep it turned off only - Unless you understand consequences - the settingsSave() functionmight stop working properly if enabled it.

#define SETTINGS_UPDATE_TIME 60000

#define SETTINGS_MAX_LIST_COUNT 10

#ifndef LOOP_DELAY_TIME
#define LOOP_DELAY_TIME 5  // Delay for this millis in the main loop [0-250]
#endif

#define MAX_ACCURATE_USEC_DELAY 16383U

//------------------------------------------------------------------------------
// Serial
//------------------------------------------------------------------------------

#define SERIAL_SEND_P(...) serialSend_P(__VA_ARGS__)
#define SERIAL_SEND(...) serialSend(__VA_ARGS__)

// #define SERIAL_SEND_P(...) void_func()
// #define SERIAL_SEND(...) void_func()

//------------------------------------------------------------------------------

#ifndef DEVELOPEMEMT_MODE
#define DEVELOPEMEMT_MODE 0  // Disable DEVELOPEMEMT_MODE By default
#endif

#if DEVELOPEMEMT_MODE == 1

#ifndef DEBUG_SERIAL_SUPPORT
#define DEBUG_SERIAL_SUPPORT 1
#endif

// #ifndef DEBUG_SUPPORT
// #define DEBUG_SUPPORT    0
// #endif

#ifndef API_AUTH_DISABLED
#define API_AUTH_DISABLED 1
#endif

#ifndef OTA_SUPPORT
#define OTA_SUPPORT 1
#endif

#endif

#if DEVELOPEMEMT_MODE == 2
// Just enable OTA , but other things not required. so its semi developement mode
#ifndef OTA_SUPPORT
#define OTA_SUPPORT 1
#endif

#endif

//--------------------------------------------------------------------------------
// DEBUG
//--------------------------------------------------------------------------------
#ifndef DEBUG_SERIAL_SUPPORT
#define DEBUG_SERIAL_SUPPORT 0  // Enable serial debug log
#define DEBUG_PORT Serial
#endif

#ifndef DEBUG_ADD_TIMESTAMP
#define DEBUG_ADD_TIMESTAMP 1  // Add timestamp to debug messages                                      // (in millis overflowing every 1000 seconds)
#endif

#ifndef DEBUG_PORT
#define DEBUG_PORT Serial
#endif

#ifndef DEBUG_SOFTWARE_SUPPORT
#define DEBUG_SOFTWARE_SUPPORT 0  // Enable software serial debug log
#endif

#if DEBUG_SOFTWARE_SUPPORT
    #include <SoftwareSerial.h>
    SoftwareSerial swSer;
    #define DEBUG_PORT swSer
#endif
//------------------------------------------------------------------------------

// UDP debug log
// To receive the message son the destination computer use nc:
// nc -ul 8113

#define DEBUG_MESSAGE_MAX_LENGTH 80

#ifndef DEBUG_UDP_SUPPORT
#define DEBUG_UDP_SUPPORT 0  // Enable UDP debug log
#endif

#define DEBUG_UDP_IP IPAddress(192, 168, 2, 192)
#define DEBUG_UDP_PORT 8114

//------------------------------------------------------------------------------

#ifndef TELNET_SUPPORT
#define TELNET_SUPPORT 0  // Enable telnet support by default (3.34Kb)
#endif

#ifndef TELNET_STA
#define TELNET_STA 0  // By default, disallow connections via STA interface
#endif

#ifndef TELNET_AUTHENTICATION
#define TELNET_AUTHENTICATION 1  // Request password to start telnet session by default
#endif

#ifndef TELNET_PORT
#define TELNET_PORT 23  // Port to listen to telnet clients
#endif

#ifndef TELNET_MAX_CLIENTS
#define TELNET_MAX_CLIENTS 1  // Max number of concurrent telnet clients
#endif

#ifndef TELNET_SERVER
#define TELNET_SERVER TELNET_SERVER_ASYNC  // Can be either TELNET_SERVER_ASYNC (using ESPAsyncTCP) or TELNET_SERVER_WIFISERVER (using WiFiServer)
#endif

#define TERMINAL_BUFFER_SIZE 128  // Max size for commands commands

//------------------------------------------------------------------------------

#ifndef DEBUG_TELNET_SUPPORT
#define DEBUG_TELNET_SUPPORT TELNET_SUPPORT  // Enable telnet debug log if telnet is enabled too
#endif

#ifndef DEBUG_SUPPORT
#define DEBUG_SUPPORT DEBUG_SERIAL_SUPPORT || DEBUG_UDP_SUPPORT || DEBUG_TELNET_SUPPORT
#endif

#if DEBUG_SUPPORT
#define DEBUG_MSG(...) debugSend(__VA_ARGS__)
#define DEBUG_MSG_P(...) debugSend_P(__VA_ARGS__)
#endif

#ifndef DEBUG_MSG
#define DEBUG_MSG(...)
#define DEBUG_MSG_P(...)
#endif

//------------------------------------------------------------------------------
// SYSTEM CHECK
//------------------------------------------------------------------------------

#ifndef SYSTEM_CHECK_ENABLED
#define SYSTEM_CHECK_ENABLED 0  // Enable crash check by default
#endif

#define SYSTEM_CHECK_TIME 60000  // The system is considered stable after these many millis
#define SYSTEM_CHECK_MAX 5       // After this many crashes on boot 
                                 // the system is flagged as unstable

// Uncomment and configure these lines to enable remote debug via udpDebug
// To receive the message son the destination computer use nc:
// nc -ul 8111

// #define DEBUG_UDP_IP            IPAddress(192, 168, 1, 4)
// #define DEBUG_UDP_PORT          8111

//--------------------------------------------------------------------------------
// EEPROM
//--------------------------------------------------------------------------------
// #define EEPROM_SIZE             4096
// #define EEPROM_RELAY_STATUS     0
// #define EEPROM_ENERGY_COUNT     1
// #define EEPROM_DIMMER_STATUS    2
// #define EEPROM_CUSTOM_RESET     5
// #define EEPROM_DATA_END         6

#define EEPROM_SIZE 4096       // EEPROM size in bytes
#define EEPROM_RELAY_STATUS 0  // Address for the relay status (1 byte)
#define EEPROM_ENERGY_COUNT 1  // Address for the energy counter (4 bytes)
#define EEPROM_DIMMER_STATUS 5
#define EEPROM_CUSTOM_RESET 6  // Address for the reset reason (1 byte)
// #define EEPROM_TEMP_ADDRESS    7
#define EEPROM_CRASH_COUNTER 7  // Address for the crash counter (1 byte)
#define EEPROM_FIRMWARE_UPDATED_FLAG 8
#define EEPROM_ROTATE_DATA 11  // Reserved for the EEPROM_ROTATE library (3 bytes)
#define EEPROM_DATA_END 14     // End of custom EEPROM data block

//------------------------------------------------------------------------------
// HEARTBEAT
//------------------------------------------------------------------------------

#ifndef HEARTBEAT_ENABLED
#define HEARTBEAT_ENABLED 1
#endif

//------------------------------------------------------------------------------
// Load average
//------------------------------------------------------------------------------

#ifndef LOADAVG_INTERVAL
#define LOADAVG_INTERVAL 30000  // Interval between calculating load average (in ms)
#endif

#ifndef LOADAVG_REPORT
#define LOADAVG_REPORT 0  // Should we report Load average over MQTT?
#endif

//-----------------------------------------------------------------------------
// Home Report
//-----------------------------------------------------------------------------
#ifndef HOME_REPORT_ENABLED
#define HOME_REPORT_ENABLED 1  // By default keep it enabled.
#endif

// -------------------------------------------------------------------------------
// SERIAL
// -------------------------------------------------------------------------------

#define SERIAL_MSG_MAX_LENGTH 150

//--------------------------------------------------------------------------------
// RESET
//--------------------------------------------------------------------------------

#define CUSTOM_RESET_HARDWARE 1
#define CUSTOM_RESET_WEB 2
#define CUSTOM_RESET_TERMINAL 3
#define CUSTOM_RESET_MQTT 4
#define CUSTOM_RESET_RPC 5
#define CUSTOM_RESET_OTA 6
#define CUSTOM_RESET_NOFUSS 8
#define CUSTOM_RESET_UPGRADE 9
#define CUSTOM_RESET_FACTORY 10

#define CUSTOM_RESET_MAX 10

#include <pgmspace.h>

// PROGMEM const char custom_reset_hardware[] = "Hardware button";
// PROGMEM const char custom_reset_web[] = "Reset from web interface";
// PROGMEM const char custom_reset_terminal[] = "Reset from terminal";
// PROGMEM const char custom_reset_mqtt[] = "Reset from MQTT";
// PROGMEM const char custom_reset_rpc[] = "Reset from RPC";
// PROGMEM const char custom_reset_ota[] = "Reset after successful OTA update";
// PROGMEM const char custom_reset_nofuss[] = "Reset after successful NoFUSS update";
// PROGMEM const char custom_reset_upgrade[] = "Reset after successful web update";
// PROGMEM const char custom_reset_factory[] = "Factory reset";
// PROGMEM const char* const custom_reset_string[] = {
//     custom_reset_hardware, custom_reset_web, custom_reset_terminal,
//     custom_reset_mqtt, custom_reset_rpc, custom_reset_ota,
//     custom_reset_nofuss, custom_reset_upgrade, custom_reset_factory
// };
//--------------------------------------------------------------------------------
// BUTTON
//--------------------------------------------------------------------------------
#ifndef BUTTON_DEBOUNCE_DELAY
#define BUTTON_DEBOUNCE_DELAY 70
#endif

#ifndef BUTTON_DBLCLICK_DELAY
#define BUTTON_DBLCLICK_DELAY 1000
#endif

#define BUTTON_LNGCLICK_DELAY 3000
#define BUTTON_LNGLNGCLICK_DELAY 5000

#define BUTTON_EVENT_NONE 0
#define BUTTON_EVENT_PRESSED 1
#define BUTTON_EVENT_CLICK 2
#define BUTTON_EVENT_DBLCLICK 3
#define BUTTON_EVENT_LNGCLICK 4
#define BUTTON_EVENT_LNGLNGCLICK 5

#define BUTTON_MODE_NONE 0
#define BUTTON_MODE_TOGGLE 1
#define BUTTON_MODE_AP 2
#define BUTTON_MODE_RESET 3
#define BUTTON_MODE_PULSE 4
#define BUTTON_MODE_FACTORY 5
#define BUTTON_MODE_CUSTOMACT   6


#ifndef BUTTON_DEFAULT_MODE
#define BUTTON_DEFAULT_MODE BUTTON_MODE_TOGGLE
#endif

#ifndef DIMMER_DEBOUNCE_DELAY
#define DIMMER_DEBOUNCE_DELAY 130
#endif

// RELAY

#ifndef REL_DEB_DELAY
#define REL_DEB_DELAY 2000
#endif

#ifndef CHECK_AC_PRESENCE_BEFORE_TOGGLING
#define CHECK_AC_PRESENCE_BEFORE_TOGGLING 0
#endif


// 0 means ANY, 1 zero or one and 2 one and only one
#define RELAY_SYNC RELAY_SYNC_ANY


// Default pulse time in milli seconds
#ifndef RELAY_PULSE_TIME
#define RELAY_PULSE_TIME 1500
#endif

// Relay requests flood protection window - in seconds
#define RELAY_FLOOD_WINDOW 3

// Allowed actual relay changes inside requests flood protection window
#define RELAY_FLOOD_CHANGES 5


//--------------------------------------------------------------------------------
// I18N
//--------------------------------------------------------------------------------

#define TMP_CELSIUS 0
#define TMP_FAHRENHEIT 1
#define TMP_UNITS TMP_CELSIUS

// All defined LEDs in the board can be managed through MQTT
// except the first one when LED_AUTO is set to 1.
// If LED_AUTO is set to 1 the board will use first defined LED to show wifi status.

#define LED_AUTO 1  // Not using this - ARIHANT

#ifndef LED_PROVIDER
#define LED_PROVIDER ANOTHER_BOARD
#endif

#ifndef LED_CONFIG
#define LED_CONFIG NEOPIXEL_LED_CONFIG_RGB
#endif

#define LED_PATTERN_NONE 0
#define LED_PAT_WIFICONNECT 1
#define LED_PAT_MQTT_CONNECTED 2
#define LED_PAT_CONNECTING 3
#define LED_PAT_SETUPMODE 4
#define LED_PAT_CON_ERROR 5
#define LED_PAT_FIRM_UPDATING 6

#ifndef LED_BRIGHTNESS_INTENSITY
#define LED_BRIGHTNESS_INTENSITY 40
#endif

// -----------------------------------------------------------------------------
// WIFI & WEB
// -----------------------------------------------------------------------------
#ifndef WIFI_RECONNECT_INTERVAL
#define WIFI_RECONNECT_INTERVAL 30000        // If could not connect to WIFI, retry after this time in ms
#endif

#define WIFI_RECONNECT_LONG_INTERVAL 240000  // 4 minutes
#define WIFI_CONNECT_TIMEOUT 30000           // Connection Timeout
#define GRATUITIOUS_ARP_DELAY 20000          // Delay between continous arp packets. For resolving some issue related to dropping of wifi.

#define WIFI_MAX_NETOWRK_DISCOVERY 30
#define ADMIN_PASS "kiotisgood"
#define FORCE_CHANGE_PASS 1

#ifndef WIFI_OUTPUT_POWER
#define WIFI_OUTPUT_POWER 20
#endif

// #define HTTP_USERNAME           "admin"
// #define WS_BUFFER_SIZE          5
// #define WS_TIMEOUT              1800000
#define WEB_PORT 80  // HTTP port
// #define DNS_PORT                53

// #define WEB_MODE_NORMAL         0
// #define WEB_MODE_PASSWORD       1

// for kiot red ap_mode is already defined - AP_MODE_ALONE
#ifndef AP_MODE
#define AP_MODE AP_MODE_ONLY_IF_NOT_AVAILABLE  //AP_MODE_ALONE //  // AP_MODE_ALONE (If you want the device to into ap mode if could not connect to the network.)
#endif

#define API_ENABLED true
#define API_BUFFER_SIZE 100     // Size of the buffer for HTTP GET API responses
#define API_REAL_TIME_VALUES 1  // Show filtered/median values by default (0 => median, 1 => real time)

#define WIFI_SLEEP_MODE WIFI_NONE_SLEEP  // WIFI_NONE_SLEEP, WIFI_LIGHT_SLEEP or WIFI_MODEM_SLEEP
#ifndef WIFI_SCAN_NETWORKS
#define WIFI_SCAN_NETWORKS 0  // Set it true only for Kiot red
#endif

#ifndef WIFI_MAX_NETWORKS
#define WIFI_MAX_NETWORKS 1
#endif

#define WIFI_RSSI_1M -30          // Calibrate it with your router reading the RSSI at 1m
#define WIFI_PROPAGATION_CONST 4  // This is typically something between 2.7 to 4.3 (free space is 2)

// This option builds the firmware with the web interface embedded.
// You first have to build the data.h file that holds the contents
// of the web interface by running "gulp buildfs_embed"

#ifndef EMBEDDED_WEB
#define EMBEDDED_WEB 0
#endif

#ifndef EMBEDDED_KEYS
#define EMBEDDED_KEYS 1  // Included Server.key and Server.cer files
#endif

#ifndef WEB_SUPPORT
#define WEB_SUPPORT 1  // This is also required for api support      // Enable web support (http, api, 121.65Kb)
#endif

#ifndef WEB_SSL_ENABLED
#define WEB_SSL_ENABLED 0
#endif

// -----------------------------------------------------------------------------
// MDNS & LLMNR
// -----------------------------------------------------------------------------

#ifndef MDNS_SERVER_SUPPORT
#define MDNS_SERVER_SUPPORT 1  // Publish services using mDNS by default (1.84Kb)
#endif

#ifndef LLMNR_SUPPORT
#define LLMNR_SUPPORT 0  // Publish device using LLMNR protocol by default (1.95Kb) - requires 2.4.0
#endif

#ifndef NETBIOS_SUPPORT
#define NETBIOS_SUPPORT 0  // Publish device using NetBIOS protocol by default (1.26Kb) - requires 2.4.0
#endif

#ifndef OTA_SUPPORT
#define OTA_SUPPORT 0  // By default OTA_Support disabled
#endif

// -----------------------------------------------------------------------------
// SPIFFS
// -----------------------------------------------------------------------------

#ifndef SPIFFS_SUPPORT
#define SPIFFS_SUPPORT 0  // Do not add support for SPIFFS by default
#endif

#ifndef LITTLE_FS_SUPPORT
#define LITTLE_FS_SUPPORT 0
#endif

// -----------------------------------------------------------------------------
// OTA &
// -----------------------------------------------------------------------------

#define OTA_PORT 8266
// -----------------------------------------------------------------------------
// NOFUSS
// -----------------------------------------------------------------------------

#ifndef NOFUSS_SUPPORT
#define NOFUSS_SUPPORT 1  // Enable support for NoFuss by default (12.65Kb)
#endif

#define NOFUSS_ENABLED 1  // Perform NoFUSS updates by default

#define NOFUSS_INTERVAL 7200000  // 6 Hours
#define OTA_SSL_FP "59:19:CF:2C:E6:E5:14:B6:15:00:25:E4:EA:8B:6F:8F:26:76:C3:55"
#define OTA_SSL_ENABLED 0

#ifndef TESTING_DEVICE
#if DEVELOPEMEMT_MODE
#define TESTING_DEVICE 1
#else
#define TESTING_DEVICE 0
#endif
#endif

#if TESTING_DEVICE
#if OTA_SSL_ENABLED
#define NOFUSS_SERVER "https://staging.feturtles.com/updatefirmware"
#else
#define NOFUSS_SERVER "http://staging.feturtles.com/updatefirmware"
#endif
#else
#if OTA_SSL_ENABLED
#define NOFUSS_SERVER "https://device.kiot.io/updatefirmware"
#else
#define NOFUSS_SERVER "http://device.kiot.io/updatefirmware"
#endif
#endif

// -----------------------------------------------------------------------------
// IR Emitters
// -----------------------------------------------------------------------------

#ifndef IR_SUPPORT
#define IR_SUPPORT 0
#endif

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

#ifndef MQTT_USE_ASYNC
#define MQTT_USE_ASYNC 1
#endif

#ifndef MQTT_SSL_ENABLED
#define MQTT_SSL_ENABLED 0  // By default ssl will be enabled on MQTT
#endif
// FOR NOW NOT USING FINGERPRINT VALIDATION ALTHOUGH
#define MQTT_SSL_FINGERPRINT "76:CA:C0:54:D9:CC:13:98:3F:E3:FA:57:6E:63:51:AD:96:4D:BE:AA"

#define MQTT_ENABLED 1  // Enable MQTT connection by default
#define MQTT_SERVER ""
#define MQTT_PORT 1885
#define MQTT_TOPIC "KU/{identifier}"
#define MQTT_RETAIN false
#define MQTT_QOS 1

#ifndef MQTT_KEEPALIVE
#define MQTT_KEEPALIVE 60
#endif

#define MQTT_RECONNECT_DELAY 10000
#define MQTT_TRY_INTERVAL 30000

#ifndef MQTT_MAX_TRIES
#define MQTT_MAX_TRIES 8
#endif

#ifndef MQTT_SKIP_RETAINED
#define MQTT_SKIP_RETAINED 1
#endif

#ifndef MQTT_SKIP_TIME
#define MQTT_SKIP_TIME 1000
#endif

#ifndef MQTT_STABLE_TIME
#define MQTT_STABLE_TIME 3000
#endif

#ifndef MQTT_RECONNECT_DELAY_MIN
#define MQTT_RECONNECT_DELAY_MIN 10000   // Try to reconnect in 5 seconds upon disconnection
#endif

#ifndef MQTT_RECONNECT_DELAY_STEP
#define MQTT_RECONNECT_DELAY_STEP 10000  // Increase the reconnect delay in 2 seconds after each failed attempt
#endif

#ifndef MQTT_RECONNECT_DELAY_MAX
#define MQTT_RECONNECT_DELAY_MAX 30000   // Set reconnect time to 30 seconds at most
#endif

#ifndef MQTT_HOME_PING_RETAINED
#define MQTT_HOME_PING_RETAINED false
#endif

#define MQTT_USE_JSON 1
#define MAX_CONFIG_TOPIC_SIZE 10

#define MQTT_TOPIC_JSON "data"
#define MQTT_TOPIC_ACTION "action"
#define MQTT_TOPIC_PING "ping"
#define MQTT_TOPIC_IRACTION "ir"
#define MQTT_TOPIC_CUSTOM "custom"
#define MQTT_TOPIC_RELAY "switch"
#define MQTT_TOPIC_LOCK "lock"
#define MQTT_TOPIC_SENSOR "sns"
#define MQTT_TOPIC_TUYA_SENSOR "tuyasns"
#define MQTT_TOPIC_CONFIG "config"
#define MQTT_TOPIC_GENERAL "gen"
#define MQTT_TOPIC_GENERIC_PRODUCT "gep"
#define MQTT_TOPIC_THERMOSTAT    "tmst"
#define MQTT_TOPIC_CUSTOM_DP "cmd"
#define MQTT_TOPIC_CUSTOM_COMMANDS "customCmd"

#define MQTT_TOPIC_BEAT "beat"
#define MQTT_TOPIC_TEMP "temp"
#define MQTT_TOPIC_LED "led"
#define MQTT_TOPIC_COLOR "color"
#define MQTT_TOPIC_BUTTON "button"
#define MQTT_TOPIC_IP "ip"
#define MQTT_TOPIC_VERSION "version"
#define MQTT_TOPIC_REVISION "rev"
#define MQTT_TOPIC_ATMVERSION "atm_v"
#define MQTT_TOPIC_UPTIME "uptime"
#define MQTT_TOPIC_FREEHEAP "freeheap"
#define MQTT_TOPIC_LOADAVG "load"
#define MQTT_TOPIC_VCC "vcc"
#define MQTT_TOPIC_STATUS "status"
#define MQTT_TOPIC_MAC "mac"
#define MQTT_TOPIC_RSSI "rssi"
#define MQTT_TOPIC_SSID "ssid"  // DEPRECATED NOW - Using NETWORK INSTEAD OF THIS NOW
#define MQTT_TOPIC_NETWORK "network"
#define MQTT_TOPIC_APP "app"
#define MQTT_TOPIC_BUILD_DATE "buildDate"
#define MQTT_TOPIC_INTERVAL "interval"
#define MQTT_TOPIC_RESET_REASON "rstres"
#define MQTT_TOPIC_HOSTNAME "hostname"
#define MQTT_TOPIC_TIME "time"
#define MQTT_TOPIC_POWER "power"
#define MQTT_TOPIC_COLOR "color"
#define MQTT_TOPIC_CURTAIN "curt"
#define MQTT_TOPIC_ALEXA "alexa"
#define MQTT_TOPIC_CONFIG_HOME "home"
#define MQTT_TOPIC_CONFIG_MQTT "mqtt"
#define MQTT_TOPIC_CONFIG_NOFUSS "nof"
#define MQTT_TOPIC_NTP "ntp"
#define MQTT_TOPIC_SYSTEM "sys"
#define MQTT_TOPIC_WIFI "wifi"
#define MQTT_TOPIC_INFO "info"
#define MQTT_TOPIC_EVENT "ev"
#define MQTT_TOPIC_EVENT_LARGE "event"
// Light module
#define MQTT_TOPIC_CHANNEL "channel"
#define MQTT_TOPIC_COLOR_RGB "rgb"
#define MQTT_TOPIC_COLOR_HSV "hsv"
#define MQTT_TOPIC_ANIM_MODE "anim_mode"
#define MQTT_TOPIC_ANIM_SPEED "anim_speed"
#define MQTT_TOPIC_BRIGHTNESS "brightness"
#define MQTT_TOPIC_MIRED "mired"
#define MQTT_TOPIC_KELVIN "kelvin"
#define MQTT_TOPIC_TRANSITION "transition"

#define EVENT_CONFIG_RCVD "CFG_RCVD"
#define EVENT_LOCK_OPERATION "LOCK_OP"
#define EVENT_LOCK_PAIRING_REQUEST "LOCK_PR"
#define EVENT_PING_FAULT "PING_FAULT"
#define EVENT_NOFUSS_UPDATE_ERROR "OTA_ERR"
#define EVENT_NOFUSS_UPDATE_SUCCESS "OTA_SUC"

// Power module
#define MQTT_TOPIC_POWER_ACTIVE "power"
#define MQTT_TOPIC_CURRENT "current"
#define MQTT_TOPIC_VOLTAGE "voltage"
#define MQTT_TOPIC_POWER_APPARENT "apparent"
#define MQTT_TOPIC_POWER_REACTIVE "reactive"
#define MQTT_TOPIC_POWER_FACTOR "factor"
#define MQTT_TOPIC_ENERGY_DELTA "energy_delta"
#define MQTT_TOPIC_ENERGY_TOTAL "energy_total"

// crash
#define MQTT_TOPIC_CRASH        "crash"

// Retained message
#define MQTT_HANDLE_RET_IR false
#define MQTT_HANDLE_RET_RELAY false
#define MQTT_HANDLE_RET_SETTINGS true
#define MQTT_HANDLE_RET_CURTAIN false
#define MQTT_HANDLE_RET_SENSORS false
#define MQTT_HANDLE_RET_PING false
#define MQTT_MANDATE_ENC_SETTINGS true
#define MQTT_MANDATE_ENC_RELAYS true
#define MQTT_MANDATE_ENC_IR_SMALL true
#define MQTT_HANDLE_RET_LOCK false
#define MQTT_HANDLE_RET_THERMOSTAT false

// MQTT_QOS
#define MQTT_QOS_IR 1
#define MQTT_QOS_RELAY 2
#define MQTT_QOS_SETTINGS 2
#define MQTT_QOS_CURTAIN 0
#define MQTT_QOS_SENSORS 0
#define MQTT_QOS_SPING 0
#define MQTT_QOS_CURTAIN 0

//Dont Encrypt messages
#define DONT_ENCRYPT_HOME_PING true

// BUFFER SIZES
#define BUFFER_SENSOR_STRING 200

// Periodic reports
#define MQTT_REPORT_STATUS 1
#define MQTT_REPORT_CONFIG 1
#define MQTT_REPORT_IP 1
#define MQTT_REPORT_SSID 1
#define MQTT_REPORT_NETWORK 1
#define MQTT_REPORT_MAC 0
#define MQTT_REPORT_RSSI 1
#define MQTT_REPORT_UPTIME 1
#define MQTT_REPORT_FREEHEAP 1
#define MQTT_REPORT_VCC 0
#define MQTT_REPORT_RELAY 0 //TODO_S2 no relay as of now
#define MQTT_REPORT_HOSTNAME 0
#define MQTT_REPORT_APP 0
#define MQTT_REPORT_VERSION 1
#define MQTT_REPORT_INTERVAL 0
#define MQTT_REPORT_LAST_RESET_REASON 1
#define MQTT_REPORT_FIRSTBEAT 1

// Config updates
#define MQTT_REPORT_CONFIG_POWER 0

#define MQTT_STATUS_ONLINE "1"
#define MQTT_STATUS_OFFLINE "0"

#define MQTT_ACTION_RESET "reset"

#define MQTT_CONNECT_EVENT 0
#define MQTT_DISCONNECT_EVENT 1
#define MQTT_MESSAGE_EVENT 2

// Custom get and set postfixes
// Use something like "/status" or "/set", with leading slash
#define MQTT_USE_GETTER "/get"
#define MQTT_USE_SETTER "/set"

// If a user is online in the app - he pings on home topic and device goes to active mqtt state and send device updates continously in the gap of 15 seconds.
// for the next 2 minutes on any action.
// But if there is no user for 2 minutes device will not send any updates to the topic.

#ifndef MQTT_REPORT_EVENT_TO_HOME
#define MQTT_REPORT_EVENT_TO_HOME 1
#endif

#ifndef MQTT_ACTIVE_STATE_REPORT
#define MQTT_ACTIVE_STATE_REPORT 1
#endif

#define MQTT_ACTIVE_STATE_TIMEOUT 120000            // 2 Minutes
#define MQTT_ACTIVE_STATE_HOME_PONG_INTERVAL 12000  // 12 seconds
#if MQTT_ACTIVE_STATE_REPORT
#endif

// Error Events reporting to server.
#ifndef MQTT_REPORT_EVENT_NOFUSS_ERROR
#define MQTT_REPORT_EVENT_NOFUSS_ERROR 1
#endif

#ifndef MQTT_REPORT_EVENT_OTA_SUCCESS
#define MQTT_REPORT_EVENT_OTA_SUCCESS 1
#endif

// -----------------------------------------------------------------------------
// HTTPSERVER
// -----------------------------------------------------------------------------

#ifndef HTTPSERVER_SUPPORT
#define HTTPSERVER_SUPPORT 0  // Enable Http support by default (2.56Kb)
#endif

#define HTTPSERVER_ENABLED 0  // HttpServer disabled by default
#define HTTPSERVER_APIKEY ""  // Default API KEY

#define HTTPSERVER_USE_ASYNC 1  // Use AsyncClient instead of WiFiClientSecure

#define HTTP_QUEUE_MAX_SIZE 5

#define SERVER_CONFIG_URL "/deviceroutes/config"
#define SERVER_RPC_URL "/deviceroutes/action"

// HTTPSERVER OVER SSL
// Using HTTPSERVER over SSL works well but generates problems with the web interface,
// so you should compile it with WEB_SUPPORT to 0.
// When HTTPSERVER_USE_ASYNC is 1, requires ASYNC_TCP_SSL_ENABLED to 1 and ESP8266 Arduino Core 2.4.0.
#define HTTPSERVER_USE_SSL 0  // Use secure connection
#define HTTPSERVER_FINGERPRINT "45 71 78 9E 29 A8 36 8F D8 F8 43 3E 28 C4 56 E8 11 63 69 D8"

#if TESTING_DEVICE
#define HTTPSERVER_HOST "staging.feturtles.com"
#else
#define HTTPSERVER_HOST "device.kiot.io"
#endif

#if HTTPSERVER_USE_SSL
#define HTTPSERVER_PORT 443
#else
#define HTTPSERVER_PORT 80
#endif
#define HTTPSERVER_URL "/device"
#define HTTPSERVER_GET_CONFIG_PATH "/config"

#define HTTPSERVER_MIN_INTERVAL 3000  // Minimum interval between POSTs (in millis)

// #define ASYNC_TCP_SSL_ENABLED 1
#ifndef ASYNC_TCP_SSL_ENABLED
#if HTTPSERVER_USE_SSL && HTTPSERVER_USE_ASYNC
#warning "DIsabling Httpserver Support"
#undef HTTPSERVER_SUPPORT  // Thingspeak in ASYNC mode requires ASYNC_TCP_SSL_ENABLED
#endif
#endif

#ifndef HTTP_CLIENT_SUPPORT
#define HTTP_CLIENT_SUPPORT 0
#endif

// Http Configuration Support
#ifndef HTTP_CONF_SUPPORT
#define HTTP_CONF_SUPPORT 1  // Enabling Http Conf support by default
#endif

#if HTTP_CONF_SUPPORT
#define HTTP_CONF_RETRY_GAP 12000
// TEMPORARY
#define HTTP_FETCH_RPC_RETRY_GAP 30000
#endif

// -----------------------------------------------------------------------------
// POWER METERING
// -----------------------------------------------------------------------------

// Available power-metering providers (do not change this)
#define POWER_PROVIDER_NONE 0x00
#define POWER_PROVIDER_EMON_ANALOG 0x10
#define POWER_PROVIDER_EMON_ADC121 0x11
#define POWER_PROVIDER_HLW8012 0x20
#define POWER_PROVIDER_V9261F 0x30
#define POWER_PROVIDER_ECH1560 0x40
#define POWER_PROVIDER_SERIAL 0x50

// Available magnitudes (do not change this)
#define POWER_MAGNITUDE_CURRENT 1
#define POWER_MAGNITUDE_VOLTAGE 2
#define POWER_MAGNITUDE_ACTIVE 4
#define POWER_MAGNITUDE_APPARENT 8
#define POWER_MAGNITUDE_REACTIVE 16
#define POWER_MAGNITUDE_POWER_FACTOR 32
#define POWER_MAGNITUDE_ALL 63

// No power provider defined (do not change this)
#ifndef POWER_PROVIDER
#define POWER_PROVIDER POWER_PROVIDER_NONE
#endif

// Identify available magnitudes (do not change this)
#if (POWER_PROVIDER == POWER_PROVIDER_HLW8012) || (POWER_PROVIDER == POWER_PROVIDER_V9261F) || (POWER_PROVIDER == POWER_PROVIDER_SERIAL)
#define POWER_HAS_ACTIVE 1
#else
#define POWER_HAS_ACTIVE 0
#endif

#if (POWER_PROVIDER == POWER_PROVIDER_HLW8012)
#define POWER_HAS_ENERGY 1
#else
#define POWER_HAS_ENERGY 0
#endif

#define POWER_VOLTAGE 230                             // Default voltage
#define POWER_MIN_READ_INTERVAL 2000                  // Minimum read interval
#ifndef POWER_READ_INTERVAL
#define POWER_READ_INTERVAL 5000                      // Default reading interval (6 seconds)
#endif
#define POWER_REPORT_INTERVAL 180000                  // Default report interval (3 minute)
#define POWER_CURRENT_DECIMALS 2                      // Decimals for current values
#define POWER_VOLTAGE_DECIMALS 0                      // Decimals for voltage values
#define POWER_POWER_DECIMALS 0                        // Decimals for power values
#define POWER_ENERGY_DECIMALS 3                       // Decimals for energy values
#define POWER_ENERGY_DECIMALS_KWH 6                   // Decimals for energy values
#define POWER_ENERGY_FACTOR 1                         // Watt * seconds == Joule (Ws == J)
#define POWER_ENERGY_FACTOR_KWH (1. / 1000. / 3600.)  // kWh

#if POWER_PROVIDER == POWER_PROVIDER_HLW8012
#define HLW8012_USE_INTERRUPTS 1           // Use interrupts to trap HLW8012 signals
#define HLW8012_SEL_CURRENT HIGH           // SEL pin to HIGH to measure current
#define HLW8012_CURRENT_R 0.001            // Current resistor
#define HLW8012_VOLTAGE_R_UP (5 * 470000)  // Upstream voltage resistor
#define HLW8012_VOLTAGE_R_DOWN (1000)      // Downstream voltage resistor
#endif

// NTP
// -----------------------------------------------------------------------------

#define NTP_SERVER "pool.ntp.org"
#define NTP_TIMEOUT 2000     // Set NTP request timeout to 2 seconds (issue #452)
#define NTP_TIME_OFFSET 330  // Gmt+5:30
#define NTP_DAY_LIGHT true
#define NTP_SYNC_INTERVAL 60      // NTP initial check every minute
#define NTP_UPDATE_INTERVAL 1800  // NTP check every 30 minutes
#define NTP_START_DELAY 1000      // Delay NTP start 1 second

#ifndef NTP_SUPPORT
#define NTP_SUPPORT 0  // By default Disabling NTP_SUPPORT
#endif

// This setting defines whether Alexa support should be built into the firmware
#ifndef ALEXA_SUPPORT
#define ALEXA_SUPPORT 0
#endif

// This is default value for the alexaEnabled setting that defines whether
// this device should be discoberable and respond to Alexa commands.
// Both ALEXA_SUPPORT and alexaEnabled should be 1 for Alexa support to work.
#ifndef ALEXA_ENABLED
#define ALEXA_ENABLED 0
#endif

// Sensors
#if TEMP_SENS_PROVIDER == DALLAS
#ifndef TEMP_SENS_CORRECTION
#define TEMP_SENS_CORRECTION 1
#endif
#endif

#if TEMP_SENS_PROVIDER == LM35
#define ANALOG_SUPPORT 1
#endif

// #define SENSOR_TEMP         "DHT"

#define HTTP_ENABLED 0

// #define WEB_SERVER_URL          "http://device.kiot.io/"

// #define GPIO_NONE           0x99 // COmmented here -> moved to types.h

// Atmega
// Untill this version of atmega, we did not have the feature of reporting the version.
// After this version we have functionality for reporting featire in atmega.
#define ATM_DEFAULT_VERSION "2.1.0"

// Curtain
#define MAXIMUM_CURTAIN_OPERATION_TIME 50000 // increased operating time for larger windows

// BLE module
#ifndef HM11_SUPPORT
#define HM11_SUPPORT 0
#endif

#define BLE_TIME_MAX_CONN 5000
#define BLE_TIME_MAX_WAIT_SESSION 7000
// -----------------------------------------------------------------------------
// LIGHT
// -----------------------------------------------------------------------------

// LIGHT_PROVIDER_DIMMER can have from 1 to 5 different channels.
// They have to be defined for each device in the hardware.h file.
// If 3 or more channels first 3 will be considered RGB.
// Usual configurations are:
// 1 channels => W
// 2 channels => WW
// 3 channels => RGB
// 4 channels => RGBW
// 5 channels => RGBWW

#ifndef LIGHT_SAVE_ENABLED
#define LIGHT_SAVE_ENABLED 0  // Light channel values saved by default after each change
#endif

#ifndef LIGHT_COMMS_DELAY
#define LIGHT_COMMS_DELAY 100  // Delay communication after light update (in ms)
#endif

#ifndef LIGHT_SAVE_DELAY
#define LIGHT_SAVE_DELAY 8  // Persist color after 5 seconds to avoid wearing out
#endif

#ifndef LIGHT_MAX_PWM

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
#define LIGHT_MAX_PWM 255
#endif

#if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER
#define LIGHT_MAX_PWM 10000  // 10000 * 200ns => 2 kHz
#endif

#endif  // LIGHT_MAX_PWM

#ifndef LIGHT_LIMIT_PWM
#define LIGHT_LIMIT_PWM LIGHT_MAX_PWM  // Limit PWM to this value (prevent 100% power)
#endif

#ifndef LIGHT_MAX_VALUE
#define LIGHT_MAX_VALUE 255  // Maximum light value
#endif

#ifndef LIGHT_MAX_BRIGHTNESS
#define LIGHT_MAX_BRIGHTNESS 255  // Maximun brightness value
#endif

#define LIGHT_MIN_MIREDS 153  // Default to the Philips Hue value that HA also use.
#define LIGHT_MAX_MIREDS 500  // https://developers.meethue.com/documentation/core-concepts

#ifndef LIGHT_STEP
#define LIGHT_STEP 32  // Step size
#endif

#ifndef LIGHT_USE_COLOR
#define LIGHT_USE_COLOR 1  // Use 3 first channels as RGB
#endif

#ifndef LIGHT_USE_WHITE
#define LIGHT_USE_WHITE 1  // Use the 4th channel as (Warm-)White LEDs
#endif

#ifndef LIGHT_USE_CCT
#define LIGHT_USE_CCT 0  // Use the 5th channel as Coldwhite LEDs, LIGHT_USE_WHITE must be 1.
#endif

// Used when LIGHT_USE_WHITE AND LIGHT_USE_CCT is 1 - (1000000/Kelvin = MiReds)
// Warning! Don't change this yet, NOT FULLY IMPLEMENTED!
#define LIGHT_COLDWHITE_MIRED 153  // Coldwhite Strip, Value must be __BELOW__ W2!! (Default: 6535 Kelvin/153 MiRed)
#define LIGHT_WARMWHITE_MIRED 500  // Warmwhite Strip, Value must be __ABOVE__ W1!! (Default: 2000 Kelvin/500 MiRed)

#ifndef LIGHT_USE_GAMMA
#define LIGHT_USE_GAMMA 0  // Use gamma correction for color channels
#endif

#ifndef LIGHT_USE_CSS
#define LIGHT_USE_CSS 1  // Use CSS style to report colors (1=> "#FF0000", 0=> "255,0,0")
#endif

#ifndef LIGHT_USE_RGB
#define LIGHT_USE_RGB 0  // Use RGB color selector (1=> RGB, 0=> HSV)
#endif

#ifndef LIGHT_WHITE_FACTOR
#define LIGHT_WHITE_FACTOR 1  // When using LIGHT_USE_WHITE with uneven brightness LEDs, 
                              // this factor is used to scale the white channel to match brightness
#endif

#ifndef LIGHT_USE_TRANSITIONS
#define LIGHT_USE_TRANSITIONS 1  // Transitions between colors
#endif

#ifndef LIGHT_TRANSITION_STEP
#define LIGHT_TRANSITION_STEP 10  // Time in millis between each transtion step
#endif

#ifndef LIGHT_TRANSITION_TIME
#define LIGHT_TRANSITION_TIME 500  // Time in millis from color to color
#endif

#ifndef DEFAULT_COLOR
#define DEFAULT_COLOR "#FFFFFF"
#endif

// Some Jugaadu thing. 
#ifndef CAN_DISABLE_WHITE
#define CAN_DISABLE_WHITE 0
#endif


//====================
// OTHERS
//===================
#ifndef USE_EEPROM_RESET
#define USE_EEPROM_RESET 0
#endif

#ifndef DELAY_RESET_RESET_COUNTER
#define DELAY_RESET_RESET_COUNTER 5000
#endif
// =========
// door bell
// =========

#define DEFAULT_DOOR_BELL_DELAY 30000
#define DOOR_BELL_MODE  "dbm"
#define DOOR_BELL_DEFAULT_MODE  1 // Normal mode
#define DOOR_BELL_SILENT_MODE  2 // scilent mode 
#define DOOR_BELL_TIMED_MODE  3 //timed mode 


#define THERMOSTAT_PROVIDER_NONE 0
#define THERMOSTAT_PROVIDER_TUYA 1

#define SENSOR_PROVIDER_NONE 0
#define SENSOR_PROVIDER_TUYA 1

// Relay types
#define RELAY_TYPE_TOGGLE       1
#define RELAY_TYPE_DIMMER       2
#define RELAY_TYPE_CURTAIN      3

//Sensor Types
#define SENSOR_TYPE_TEMP       1
#define SENSOR_TYPE_HUMID      2
#define SENSOR_TYPE_POWER      3

//Relay Feature Types
#define RELAY_FEATURE_TYPE_FAN        0
#define RELAY_FEATURE_TYPE_LIGHT      1
#define RELAY_FEATURE_TYPE_HEAT       2

#define SETTING_RELAY_MONO_BACKLGT_SUPPORT "vRgblgtm"
#define SETTING_RELAY_MONO_BACKLGT_CFG      "vRgblgtCfgm"
#define SETTING_RELAY_MONO_BACKLGT_CFG_DEF  "{\"v\":[50,5,100]}"

#define SETTING_RELAY_RGB_BACKLGT_SUPPORT "vRgblgt"
#define SETTING_RELAY_RGB_BACKLGT_CFG "vRgblgtCfg"
#define SETTING_RELAY_RGB_BACKLGT_CFG_DEF "{\"v\":[255,0,0,255,255,150,50,10,20]}"

// virtual switch reassignement configuration
// #define MAX_VIRTUAL_SWITCH_COUNT 5
#define SETTINGS_VIRTUAL_SWITCH_INDEX_PREFIX "vSw"
#define SETTINGS_VIRTUAL_SWITCH_INDEX_DEF "0"

// light dimmer flicker configuration
// #define MAX_DIMMER_FLICK_CONFIG_COUNT 5
#define SETTINGS_DIMMER_FLICK_CONFIG_INDEX_PREFIX "vDmrFlickCfg"
#define SETTINGS_DIMMER_FLICK_CONFIG_INDEX_DEF "0"

// trigger source to report to the server
#define TRIGGER_SOURCE_WF 1
#define TRIGGER_SOURCE_WAVE  2
#define TRIGGER_SOURCE_TOUCH 3
#define TRIGGER_SOURCE_REMOTE 4
#define TRIGGER_SOURCE_SCENES 5
