
#pragma once
#define TAG "FloatLevel"

#define WATCHDOG_TIMER 600000 // time in ms to trigger the watchdog

#define STR_LEN 255                            // general string buffer size
#define CONFIG_LEN 32                         // configuration string buffer size
#define NUMBER_CONFIG_LEN 6
#define LEVEL_CONFIG_LEN 2
#define AP_TIMEOUT 30000
#define DEFAULT_AP_PASSWORD "12345678"

#define CheckBit(var,pos) ((var) & (1<<(pos))) ? true : false
#define toShort(i, v) (v[i++]<<8) | v[i++]

#define ADC_Resolution 4095.0
#define SAMPLESIZE 20

#define ASYNC_WEBSERVER_PORT 7667
#define IOTCONFIG_PORT 80
#define WSOCKET_LOG_PORT 7668
#define WSOCKET_HOME_PORT 7669