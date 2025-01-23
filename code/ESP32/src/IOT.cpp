#include "IOT.h"
#include <sys/time.h>
#include <EEPROM.h>
#include "time.h"
#include "Log.h"
#include "IotWebConfOptionalGroup.h"
#include <IotWebConfTParameter.h>

namespace FloatLevelNS
{
AsyncMqttClient _mqttClient;
TimerHandle_t mqttReconnectTimer;
DNSServer _dnsServer;
WebServer* _pWebServer;
HTTPUpdateServer _httpUpdater;
IotWebConf _iotWebConf(TAG, &_dnsServer, _pWebServer, TAG, CONFIG_VERSION);

#define STRING_LEN 128
static const char chooserValues[][STRING_LEN] = { "red", "blue", "darkYellow" };
static const char chooserNames[][STRING_LEN] = { "Red", "Blue", "Dark yellow" };

char _level1[LEVEL_CONFIG_LEN];
char _level2[LEVEL_CONFIG_LEN];
char _level3[LEVEL_CONFIG_LEN];
char _level4[LEVEL_CONFIG_LEN];

char _tankName[IOTWEBCONF_WORD_LEN];
char _willTopic[STR_LEN];
char _mqttServer[IOTWEBCONF_WORD_LEN];
char _mqttPort[NUMBER_CONFIG_LEN];
char _mqttUserName[IOTWEBCONF_WORD_LEN];
char _mqttUserPassword[IOTWEBCONF_WORD_LEN];
char _rootTopicPrefix[64];
u_int _uniqueId = 0;

iotwebconf::IntTParameter<int16_t> levelParam1 = iotwebconf::Builder<iotwebconf::IntTParameter<int16_t>>("level1").label("Level 1").defaultValue(20).min(1).max(100).step(1).placeholder("1..100").build();
iotwebconf::IntTParameter<int16_t> levelParam2 = iotwebconf::Builder<iotwebconf::IntTParameter<int16_t>>("level2").label("Level 2").defaultValue(40).min(1).max(100).step(1).placeholder("1..100").build();
iotwebconf::IntTParameter<int16_t> levelParam3 = iotwebconf::Builder<iotwebconf::IntTParameter<int16_t>>("level3").label("Level 3").defaultValue(60).min(1).max(100).step(1).placeholder("1..100").build();
iotwebconf::IntTParameter<int16_t> levelParam4 = iotwebconf::Builder<iotwebconf::IntTParameter<int16_t>>("level4").label("Level 4").defaultValue(80).min(1).max(100).step(1).placeholder("1..100").build();

iotwebconf::OptionalParameterGroup MQTT_group = iotwebconf::OptionalParameterGroup("MQTT", "MQTT", false);
iotwebconf::TextParameter mqttServerParam = iotwebconf::TextParameter("MQTT server", "mqttServer", _mqttServer, IOTWEBCONF_WORD_LEN);
iotwebconf::NumberParameter mqttPortParam = iotwebconf::NumberParameter("MQTT port", "mqttSPort", _mqttPort, NUMBER_CONFIG_LEN, "text", NULL, "1883");
iotwebconf::TextParameter mqttUserNameParam = iotwebconf::TextParameter("MQTT user", "mqttUser", _mqttUserName, IOTWEBCONF_WORD_LEN);
iotwebconf::PasswordParameter mqttUserPasswordParam = iotwebconf::PasswordParameter("MQTT password", "mqttPass", _mqttUserPassword, IOTWEBCONF_WORD_LEN, "password");
iotwebconf::TextParameter mqttTankNameParam = iotwebconf::TextParameter("MQTT Tank Name", "mqttTankName", _tankName, IOTWEBCONF_WORD_LEN, "Tank1");

iotwebconf::OptionalGroupHtmlFormatProvider optionalGroupHtmlFormatProvider;

void publishDiscovery()
{
	char buffer[STR_LEN];
	JsonDocument doc; // MQTT discovery
	doc["name"] = _iotWebConf.getThingName();
	sprintf(buffer, "%s_%X_WindSpeed", _iotWebConf.getThingName(), _uniqueId);
	doc["unique_id"] = buffer;
	doc["unit_of_measurement"] = "km-h";
	doc["stat_t"] = "~/stat";
	doc["stat_tpl"] = "{{ value_json.windspeed}}";
	doc["avty_t"] = "~/tele/LWT";
	doc["pl_avail"] = "Online";
	doc["pl_not_avail"] = "Offline";
	JsonObject device = doc["device"].to<JsonObject>(); 
	device["name"] = _iotWebConf.getThingName();
	device["sw_version"] = CONFIG_VERSION;
	device["manufacturer"] = "ClassicDIY";
	sprintf(buffer, "ESP32-Bit (%X)", _uniqueId);
	device["model"] = buffer;
	device["identifiers"] = _uniqueId;
	doc["~"] = _rootTopicPrefix;
	String s;
	serializeJson(doc, s);
	char configurationTopic[64];
	sprintf(configurationTopic, "%s/sensor/%s/%X/WindSpeed/config", HOME_ASSISTANT_PREFIX, _iotWebConf.getThingName(), _uniqueId);
	if (_mqttClient.publish(configurationTopic, 0, true, s.c_str(), s.length()) == 0)
	{
		loge("**** Configuration payload exceeds MAX MQTT Packet Size");
	}
}

void onMqttConnect(bool sessionPresent)
{
	logd("Connected to MQTT. Session present: %d", sessionPresent);
	publishDiscovery();
	_mqttClient.publish(_willTopic, 0, false, "Online");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
	logw("Disconnected from MQTT. Reason: %d", (int8_t)reason);
	if (WiFi.isConnected())
	{
		xTimerStart(mqttReconnectTimer, 0);
	}
}

void connectToMqtt()
{
	if (WiFi.isConnected())
	{
		if (MQTT_group.isActive() && strlen(_mqttServer) > 0) // mqtt configured
		{
			logi("Connecting to MQTT...");
			int len = strlen(_iotWebConf.getThingName());
			strncpy(_rootTopicPrefix, _iotWebConf.getThingName(), len);
			if (_rootTopicPrefix[len - 1] != '/')
			{
				strcat(_rootTopicPrefix, "/");
			}
			strcat(_rootTopicPrefix, _tankName);

			sprintf(_willTopic, "%s/tele/LWT", _rootTopicPrefix);
			_mqttClient.setWill(_willTopic, 0, true, "Offline");
			_mqttClient.connect();
			logd("rootTopicPrefix: %s", _rootTopicPrefix);
		}
	}
}

void WiFiEvent(WiFiEvent_t event)
{
	logd("[WiFi-event] event: %d", event);
	String s;
	JsonDocument doc;
	switch (event)
	{
	case SYSTEM_EVENT_STA_GOT_IP:
		// logd("WiFi connected, IP address: %s", WiFi.localIP().toString().c_str());
		doc["IP"] = WiFi.localIP().toString().c_str();
		doc["ApPassword"] = TAG;
		serializeJson(doc, s);
		s += '\n';
		Serial.printf(s.c_str()); // send json to flash tool
		configTime(0, 0, NTP_SERVER);
		printLocalTime();
		xTimerStart(mqttReconnectTimer, 0);
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		logw("WiFi lost connection");
		xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
		break;
	default:
		break;
	}
}

void onMqttPublish(uint16_t packetId)
{
	logd("Publish acknowledged.  packetId: %d", packetId);
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
	logd("MQTT Message arrived [%s]  qos: %d len: %d index: %d total: %d", topic, properties.qos, len, index, total);
	printHexString(payload, len);
}

IOT::IOT(WebServer* pWebServer)
{
	_pWebServer = pWebServer;
}

/**
 * Handle web requests to "/" path.
 */
void handleSettings()
{
	// -- Let IotWebConf test and handle captive portal requests.
	if (_iotWebConf.handleCaptivePortal())
	{
		logd("Captive portal");
		// -- Captive portal request were already served.
		return;
	}
	logd("handleSettings");
	String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
	s += "<title>";
	s += _iotWebConf.getThingName();
	s += "</title></head><body>";
	s += _iotWebConf.getThingName();
	s += "<ul>";
	s += "<li>MQTT server: ";
	s += _mqttServer;
	s += "</ul>";
	s += "<ul>";
	s += "<li>MQTT port: ";
	s += _mqttPort;
	s += "</ul>";
	s += "<ul>";
	s += "<li>MQTT user: ";
	s += _mqttUserName;
	s += "</ul>";
	s += "<ul>";
	s += "<li>Tank Name: ";
	s += _tankName;
	s += "</ul>";
	s += "Go to <a href='config'>configure page</a> to change values.";
	s += "</body></html>\n";
	_pWebServer->send(200, "text/html", s);
}

void configSaved()
{
	logi("Configuration was updated.");
}

void IOT::Init()
{
	pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);
	_iotWebConf.setStatusPin(WIFI_STATUS_PIN);
	_iotWebConf.setConfigPin(WIFI_AP_PIN);

	// setup EEPROM parameters

   	MQTT_group.addItem(&mqttServerParam);
	MQTT_group.addItem(&mqttPortParam);
   	MQTT_group.addItem(&mqttUserNameParam);
	MQTT_group.addItem(&mqttUserPasswordParam);
	MQTT_group.addItem(&mqttTankNameParam);
	_iotWebConf.setHtmlFormatProvider(&optionalGroupHtmlFormatProvider);

	_iotWebConf.addSystemParameter(&levelParam1);
	_iotWebConf.addSystemParameter(&levelParam2);
	_iotWebConf.addSystemParameter(&levelParam3);
	_iotWebConf.addSystemParameter(&levelParam4);

	_iotWebConf.addParameterGroup(&MQTT_group);

	// setup callbacks for IotWebConf
	_iotWebConf.setConfigSavedCallback(&configSaved);
	// _iotWebConf.setFormValidator(&formValidator);
	_iotWebConf.setupUpdateServer(
      [](const char* updatePath) { _httpUpdater.setup(_pWebServer, updatePath); },
      [](const char* userName, char* password) { _httpUpdater.updateCredentials(userName, password); });

	if (digitalRead(FACTORY_RESET_PIN) == LOW)
	{
		EEPROM.begin(IOTWEBCONF_CONFIG_START + IOTWEBCONF_CONFIG_VERSION_LENGTH );
		for (byte t = 0; t < IOTWEBCONF_CONFIG_VERSION_LENGTH; t++)
		{
			EEPROM.write(IOTWEBCONF_CONFIG_START + t, 0);
		}
		EEPROM.commit();
		EEPROM.end();
		_iotWebConf.resetWifiAuthInfo();
		logw("Factory Reset!");
	}
	mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(5000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
	WiFi.onEvent(WiFiEvent);
	boolean validConfig = _iotWebConf.init();
	if (!validConfig)
	{
		logw("!invalid configuration!");
		_mqttServer[0] = '\0';
		_mqttPort[0] = '\0';
		_mqttUserName[0] = '\0';
		_mqttUserPassword[0] = '\0';
		_iotWebConf.resetWifiAuthInfo();
	}
	else
	{
		_iotWebConf.skipApStartup(); // Set WIFI_AP_PIN to gnd to force AP mode
		if (_mqttServer[0] != '\0') // skip if factory reset
		{
			logd("Valid configuration!");
			_clientsConfigured = true;
			// setup MQTT
			_mqttClient.onConnect(onMqttConnect);
			_mqttClient.onDisconnect(onMqttDisconnect);
			_mqttClient.onMessage(onMqttMessage);
			_mqttClient.onPublish(onMqttPublish);
			IPAddress ip;
			int port = atoi(_mqttPort);
			if (ip.fromString(_mqttServer))
			{
				_mqttClient.setServer(ip, port);
			}
			else
			{
				_mqttClient.setServer(_mqttServer, port);
			}
			_mqttClient.setCredentials(_mqttUserName, _mqttUserPassword);
		}
	}
	// generate unique id from mac address NIC segment
	uint8_t chipid[6];
	esp_efuse_mac_get_default(chipid);
	_uniqueId = chipid[3] << 16;
	_uniqueId += chipid[4] << 8;
	_uniqueId += chipid[5];
	// Set up required URL handlers on the web server.
	_pWebServer->on("/settings", handleSettings);
	_pWebServer->on("/config", [] { _iotWebConf.handleConfig(); });
	_pWebServer->onNotFound([]() { _iotWebConf.handleNotFound(); });
}

void IOT::Process(float waterLevel)
{
	logi("Float number 1 is %s", (waterLevel > levelParam1.value()) ? "on" : "off");


	if (_mqttClient.connected())
	{
		String s;
		JsonDocument doc;
		doc.clear();
		doc["WaterLevel"] = waterLevel;
		serializeJson(doc, s);
		publish("stat", s.c_str());
	}
}

boolean IOT::Run()
{
	bool rVal = false;
	_iotWebConf.doLoop();
	if (_clientsConfigured && WiFi.isConnected())
	{
		rVal = _mqttClient.connected();
	}
	else
	{
		if (Serial.peek() == '{')
		{
			String s = Serial.readStringUntil('}');
			s += "}";
			JsonDocument doc;
			DeserializationError err = deserializeJson(doc, s);
			if (err)
			{
				loge("deserializeJson() failed: %s", err.c_str());
			}
			else
			{
				if (doc["ssid"].is<const char*>() && doc["password"].is<const char*>())
				{
					iotwebconf::Parameter *p = _iotWebConf.getWifiSsidParameter();
					strcpy(p->valueBuffer, doc["ssid"]);
					logd("Setting ssid: %s", p->valueBuffer);
					p = _iotWebConf.getWifiPasswordParameter();
					strcpy(p->valueBuffer, doc["password"]);
					logd("Setting password: %s", p->valueBuffer);
					p = _iotWebConf.getApPasswordParameter();
					strcpy(p->valueBuffer, TAG); // reset to default AP password
					_iotWebConf.saveConfig();
					esp_restart();
				}
				else
				{
					logw("Received invalid json: %s", s.c_str());
				}
			}
		}
		else
		{
			Serial.read(); // discard data
		}
	}
	return rVal;
}

void IOT::publish(const char *subtopic, const char *value, boolean retained)
{
	if (_mqttClient.connected())
	{
		char buf[64];
		sprintf(buf, "%s/%s", _rootTopicPrefix, subtopic);
		_mqttClient.publish(buf, 0, retained, value);
	}
}


} // namespace SkyeTracker