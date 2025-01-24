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
#include "IOTServiceInterface.h"
#include "IOTCallbackInterface.h"

namespace FloatLevelNS
{
class IOT : public IOTServiceInterface
{
public:
    IOT(WebServer* pWebServer);
    void Init(IOTCallbackInterface* iotCB);

    boolean Run();
    void Publish(const char *subtopic, const char *value, boolean retained = false);
    void Publish(const char *subtopic, float value, boolean retained = false);
    void PublishMessage(const char* topic, JsonDocument& payload);
    std::string getRootTopicPrefix();
    u_int getUniqueId() { return _uniqueId;};
    std::string getThingName();
private:
    bool _clientsConfigured = false;
    IOTCallbackInterface* _iotCB;
    u_int _uniqueId = 0; // unique id from mac address NIC segment
};
} // namespace FloatLevelNS