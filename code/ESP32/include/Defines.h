
#pragma once

#define STR_LEN 255                            // general string buffer size
#define CONFIG_LEN 32                         // configuration string buffer size
#define NUMBER_CONFIG_LEN 6
#define LEVEL_CONFIG_LEN 2
#define AP_TIMEOUT 30000

#define CheckBit(var,pos) ((var) & (1<<(pos))) ? true : false
#define toShort(i, v) (v[i++]<<8) | v[i++]

#define ADC_Resolution 4095.0
#define SAMPLESIZE 10

