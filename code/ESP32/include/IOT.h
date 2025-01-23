#pragma once

#include "WiFi.h"
#include "ArduinoJson.h"
#include <EEPROM.h>
extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
}
#include "Log.h"
#include "AsyncMqttClient.h"
#include <IotWebConf.h>
#include <IotWebConfUsing.h>
#include <IotWebConfESP32HTTPUpdateServer.h>
#include "Defines.h"

namespace FloatLevelNS
{
class IOT
{
public:
    IOT(WebServer* pWebServer);
    void Init();
    void Process(float waterLevel);
    boolean Run();
    void publish(const char *subtopic, const char *value, boolean retained = false);
private:
    bool _clientsConfigured = false;
};
} // namespace FloatLevelNS