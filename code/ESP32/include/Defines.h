
#pragma once

#define HOME_ASSISTANT_PREFIX "homeassistant" // MQTT prefix used in autodiscovery
#define STR_LEN 255                            // general string buffer size
#define CONFIG_LEN 32                         // configuration string buffer size
#define NUMBER_CONFIG_LEN 6
#define LEVEL_CONFIG_LEN 2
#define AP_TIMEOUT 30000
#define MAX_PUBLISH_RATE 30000
#define MIN_PUBLISH_RATE 1000
#define CheckBit(var,pos) ((var) & (1<<(pos))) ? true : false
#define toShort(i, v) (v[i++]<<8) | v[i++]

#define ADC_Resolution 4095.0
#define SAMPLESIZE 10

#define BUTTON_1 21
#define BUTTON_2 19
#define BUTTON_3 32
#define BUTTON_4 33