
#ifndef HAVE_SIBLING_CONTROLLER
#define HAVE_SIBLING_CONTROLLER 0
#endif

#ifndef SENSOR_SUPPORT
#define SENSOR_SUPPORT 0
#endif

// This is useful in nexa products generally,
// where we want to use 5 tiumes toggle for restarting the module while 7 times toggling for factory set.

#ifndef BUTTON_ACTION_MULTIPLE
#define BUTTON_ACTION_MULTIPLE 0
#endif

#ifndef WIFI_LEDS_ON_ANOTHER_BOARDS
#define WIFI_LEDS_ON_ANOTHER_BOARDS 0
#endif

#ifndef USE_SWITCH_FOR_RESET
#define USE_SWITCH_FOR_RESET 0
#endif

#ifndef BUTTON_DBL_CLICK_COUNT
#define BUTTON_DBL_CLICK_COUNT 2
#endif

#ifndef BUTTON_TRIPLE_CLICK_COUNT
#define BUTTON_TRIPLE_CLICK_COUNT 0
#endif

#ifndef BUTTON1_PRESS
#define BUTTON1_PRESS BUTTON_MODE_NONE
#endif
#ifndef BUTTON2_PRESS
#define BUTTON2_PRESS BUTTON_MODE_NONE
#endif
#ifndef BUTTON3_PRESS
#define BUTTON3_PRESS BUTTON_MODE_NONE
#endif
#ifndef BUTTON4_PRESS
#define BUTTON4_PRESS BUTTON_MODE_NONE
#endif
#ifndef BUTTON5_PRESS
#define BUTTON5_PRESS BUTTON_MODE_NONE
#endif

#ifndef BUTTON1_CLICK
#define BUTTON1_CLICK BUTTON_MODE_TOGGLE
#endif
#ifndef BUTTON2_CLICK
#define BUTTON2_CLICK BUTTON_MODE_TOGGLE
#endif
#ifndef BUTTON3_CLICK
#define BUTTON3_CLICK BUTTON_MODE_TOGGLE
#endif
#ifndef BUTTON4_CLICK
#define BUTTON4_CLICK BUTTON_MODE_TOGGLE
#endif

#ifndef BUTTON1_DBLCLICK
#define BUTTON1_DBLCLICK BUTTON_MODE_AP
#endif
#ifndef BUTTON2_DBLCLICK
#define BUTTON2_DBLCLICK BUTTON_MODE_NONE
#endif
#ifndef BUTTON3_DBLCLICK
#define BUTTON3_DBLCLICK BUTTON_MODE_NONE
#endif
#ifndef BUTTON4_DBLCLICK
#define BUTTON4_DBLCLICK BUTTON_MODE_NONE
#endif

#ifndef BUTTON1_LNGCLICK
#define BUTTON1_LNGCLICK BUTTON_MODE_RESET
#endif
#ifndef BUTTON2_LNGCLICK
#define BUTTON2_LNGCLICK BUTTON_MODE_NONE
#endif
#ifndef BUTTON3_LNGCLICK
#define BUTTON3_LNGCLICK BUTTON_MODE_NONE
#endif
#ifndef BUTTON4_LNGCLICK
#define BUTTON4_LNGCLICK BUTTON_MODE_NONE
#endif

#ifndef BUTTON1_LNGLNGCLICK
#define BUTTON1_LNGLNGCLICK BUTTON_MODE_FACTORY
#endif
#ifndef BUTTON2_LNGLNGCLICK
#define BUTTON2_LNGLNGCLICK BUTTON_MODE_NONE
#endif
#ifndef BUTTON3_LNGLNGCLICK
#define BUTTON3_LNGLNGCLICK BUTTON_MODE_NONE
#endif
#ifndef BUTTON4_LNGLNGCLICK
#define BUTTON4_LNGLNGCLICK BUTTON_MODE_NONE
#endif

#ifndef BUTTON1_RELAY
#define BUTTON1_RELAY 0
#endif
#ifndef BUTTON2_RELAY
#define BUTTON2_RELAY 0
#endif
#ifndef BUTTON3_RELAY
#define BUTTON3_RELAY 0
#endif
#ifndef BUTTON4_RELAY
#define BUTTON4_RELAY 0
#endif
#ifndef BUTTON5_RELAY
#define BUTTON5_RELAY 0
#endif

// Added by KIOT
// TODO: THIS COULD BE A BAD IDEA. since pin 0 is also used sometimes in button, in case of child controllers.
// However, we are taking care in the relay provider status and using this pin only if relay provider is relay_provider_esp
#ifndef RELAY1_PIN
#define RELAY1_PIN 0
#endif

#ifndef RELAY2_PIN
#define RELAY2_PIN 0
#endif

#ifndef RELAY3_PIN
#define RELAY3_PIN 0
#endif

#ifndef RELAY4_PIN
#define RELAY4_PIN 0
#endif

#ifndef RELAY5_PIN
#define RELAY5_PIN 0
#endif

#ifndef RELAY1_INVERSE
#define RELAY1_INVERSE false
#endif

#ifndef RELAY2_INVERSE
#define RELAY2_INVERSE false
#endif

#ifndef RELAY3_INVERSE
#define RELAY3_INVERSE false
#endif

#ifndef RELAY4_INVERSE
#define RELAY4_INVERSE false
#endif

#ifndef RELAY5_INVERSE
#define RELAY5_INVERSE false
#endif

// =============================================================

#ifndef RELAY1_LED
#define RELAY1_LED 0
#endif
#ifndef RELAY2_LED
#define RELAY2_LED 0
#endif
#ifndef RELAY3_LED
#define RELAY3_LED 0
#endif
#ifndef RELAY4_LED
#define RELAY4_LED 0
#endif

#ifndef WIFI_LED
#define WIFI_LED 1
#endif

#ifndef USE_DELAYED_REPORTING
#define USE_DELAYED_REPORTING 0  // Delayed reporting/saving - useful in case of devices where we are using AC Switches and power supply fluctuates.
#endif


// Relay providers
#ifndef RELAY_PROVIDER
#define RELAY_PROVIDER RELAY_PROVIDER_SERIAL
#endif

// Default boot mode: 0 means OFF, 1 ON and 2 whatever was before
#ifndef RELAY_BOOT_MODE
#define RELAY_BOOT_MODE RELAY_MODE_SAME
#endif

// Default pulse mode: 0 means no pulses, 1 means normally off, 2 normally on
#ifndef RELAY_PULSE_MODE
#define RELAY_PULSE_MODE RELAY_PULSE_NONE
#endif

#ifndef RELAY_TYPE
#define RELAY_TYPE RELAY_TYPE_NORMAL
#endif

#ifndef DIMMER_PROVIDER
#define DIMMER_PROVIDER DIMMER_PROVIDER_SERIAL
#endif

#ifndef DIMMING_LEVELS
#define DIMMING_LEVELS 1
#endif

// Do not save relay state after these many milliseconds
#ifndef RELAY_SAVE_DELAY
#define RELAY_SAVE_DELAY 1000
#endif

// Pulse with in milliseconds for a latched relay
#ifndef RELAY_LATCHING_PULSE
#define RELAY_LATCHING_PULSE 1000
#endif
// report relay status
// Used in relayToggle - Usually true for all hardwares except doorbell, where we dont want to save relay status or report a manual switch event. 
#ifndef REPORT_AND_SAVE_RELAY
#define REPORT_AND_SAVE_RELAY 1
#endif


#ifndef DIMPOS1_ON_STATE
#define DIMPOS1_ON_STATE 1
#endif

#ifndef DIMPOS2_ON_STATE
#define DIMPOS2_ON_STATE 1
#endif

#ifndef DIMPOS3_ON_STATE
#define DIMPOS3_ON_STATE 1
#endif

#ifndef DIMPOS4_ON_STATE
#define DIMPOS4_ON_STATE 1
#endif
// Cheating Dimmer - in case of NExa 1 plus 1 switch where we dont have one less pins for the dimmer. We cheat and geive same speed for level 2 and level 3.
#ifndef CHEAT_DIMMER
#define CHEAT_DIMMER 0
#endif

// Light provider
#ifndef LIGHT_PROVIDER
#define LIGHT_PROVIDER LIGHT_PROVIDER_NONE
#endif

// DHT provider
#ifndef DHT_ON_BOARD
#define DHT_ON_BOARD 0
#endif

#ifndef DUMMY_RELAY_COUNT
#define DUMMY_RELAY_COUNT     0
#endif

// MDNS
#ifndef DEFAULT_DISABLE_MDNS
#define DEFAULT_DISABLE_MDNS 0
#endif

// BUZZER
#ifndef BUZZER_SUPPORT
#define BUZZER_SUPPORT 0
#endif


#ifndef BUZZER_BEEP_INTERVAL
#define BUZZER_BEEP_INTERVAL    200
#endif

// KU Communication
#ifndef KU_SERIAL_SUPPORT
#define KU_SERIAL_SUPPORT   0
#endif

#ifndef PRODUCT_INFO_QUERY_INTERVAL
#define PRODUCT_INFO_QUERY_INTERVAL 2000 //ms
#endif

#ifndef HEARTBEAT_QUERY_INTERVAL
#define HEARTBEAT_QUERY_INTERVAL 25000 //ms
#endif

// Child Controller
#ifndef ENABLE_CONTROLLER_SETTINGS
#define ENABLE_CONTROLLER_SETTINGS 0
#endif

//--------------------------------------------------------------------------------
// TUYA switch & dimmer support
//--------------------------------------------------------------------------------
#ifndef TUYA_SUPPORT
#define TUYA_SUPPORT                0
#endif

#ifndef TUYA_DEBUG
#define TUYA_DEBUG  0
#endif

#ifndef TUYA_SENSOR_SUPPORT
#define TUYA_SENSOR_SUPPORT 0
#endif

#ifndef TUYA_SENSOR_TYPE
#define TUYA_SENSOR_TYPE TY_SENSOR_UNKNOWN
#endif

#ifndef TU_COMMAND_TYPE
#define TU_COMMAND_TYPE   TU_COMMAND_TYPE_NORMAL
#endif

#ifndef TUYA_SERIAL
#define TUYA_SERIAL                 Serial
#endif

#ifndef ENABLE_SEND_ACK_TUYA_DP_UPDATE
#define ENABLE_SEND_ACK_TUYA_DP_UPDATE  0
#endif

#ifndef SWITCH_MODE_DEFAULT
#define SWITCH_MODE_DEFAULT SWITCH_MANUAL_OVERRIDE_DISABLED
#endif

#ifndef MQTT_SETTINGS_ENABLED
#define MQTT_SETTINGS_ENABLED   1
#endif

#ifndef THERMOSTAT_PROVIDER
#define THERMOSTAT_PROVIDER  THERMOSTAT_PROVIDER_NONE
#endif

#ifndef SENSOR_PROVIDER
#define SENSOR_PROVIDER       SENSOR_PROVIDER_NONE
#endif

#ifndef CURTAIN_PROVIDER
#define CURTAIN_PROVIDER CURTAIN_PROVIDER_ESP
#endif

#ifndef CURTAIN_SUPPORT
#define CURTAIN_SUPPORT 0
#endif

#ifndef LOW_POWER_OPTIMIZATION
#define LOW_POWER_OPTIMIZATION 0
#endif

#ifndef POWER_CURRENT_SUPPORT
#define POWER_CURRENT_SUPPORT 0
#endif

#ifndef POWER_VOLTAGE_SUPPORT
#define POWER_VOLTAGE_SUPPORT 0
#endif

#ifndef RELAY_UPDATE_ON_BOOT_DELAY
#define RELAY_UPDATE_ON_BOOT_DELAY 1000 * 3
#endif

#ifndef RGB_BACKLGT_SUPPORTED
#define RGB_BACKLGT_SUPPORTED 0
#endif

#ifndef MONO_BACKLGT_SUPPORTED
#define MONO_BACKLGT_SUPPORTED 0
#endif

#ifndef IR_LED_TEST
#define IR_LED_TEST 0
#endif

#ifndef ENCRYPT_CLI
#define ENCRYPT_CLI 0
#endif

#ifndef REPORT_TRIGGER_SOURCE
#define REPORT_TRIGGER_SOURCE 0 // only implemented for relay and dimmer (curtain, power, light, sensor, thermostat not implemented)
#endif
