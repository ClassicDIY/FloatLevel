#include <Arduino.h>
#include "ModbusServerTCPasync.h"
#include "IotWebConfOptionalGroup.h"
#include <IotWebConfTParameter.h>
#include "CoilData.h"
#include "Log.h"
#include "WebLog.h"
#include "HelperFunctions.h"
#include "Tank.h"
#include <WebSocketsServer.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "html.h"


namespace FloatLevelNS
{
	WebLog _webLog = WebLog();
	AsyncWebServer asyncServer(ASYNC_WEBSERVER_PORT);
	WebSocketsServer _webSocket = WebSocketsServer(WSOCKET_HOME_PORT);

	iotwebconf::ParameterGroup Pump_group = iotwebconf::ParameterGroup("pumps", "Level");
	iotwebconf::IntTParameter<int16_t> levelParam4 = iotwebconf::Builder<iotwebconf::IntTParameter<int16_t>>("level4").label("Overflow").defaultValue(80).min(1).max(100).step(1).placeholder("1..100").build();
	iotwebconf::IntTParameter<int16_t> levelParam3 = iotwebconf::Builder<iotwebconf::IntTParameter<int16_t>>("level3").label("Start Lag").defaultValue(60).min(1).max(100).step(1).placeholder("1..100").build();
	iotwebconf::IntTParameter<int16_t> levelParam2 = iotwebconf::Builder<iotwebconf::IntTParameter<int16_t>>("level2").label("Start Lead").defaultValue(40).min(1).max(100).step(1).placeholder("1..100").build();
	iotwebconf::IntTParameter<int16_t> levelParam1 = iotwebconf::Builder<iotwebconf::IntTParameter<int16_t>>("level1").label("Stop").defaultValue(20).min(1).max(100).step(1).placeholder("1..100").build();

	iotwebconf::OptionalParameterGroup Modbus_group = iotwebconf::OptionalParameterGroup("modbus", "Modbus", true);
	iotwebconf::IntTParameter<int16_t> modbusPort = iotwebconf::Builder<iotwebconf::IntTParameter<int16_t>>("modbusPort").label("Modbus Port").defaultValue(502).build();
	iotwebconf::IntTParameter<int16_t> modbusID = iotwebconf::Builder<iotwebconf::IntTParameter<int16_t>>("modbusID").label("Modbus ID").defaultValue(1).min(0).max(247).step(1).placeholder("1..10").build();

	String Tank::getSettingsHTML()
	{
		String s;
		s += "Tank:";
		s += "<ul>";
		s += htmlConfigEntry<int16_t>(levelParam4.label, levelParam4.value());
		s += htmlConfigEntry<int16_t>(levelParam3.label, levelParam3.value());
		s += htmlConfigEntry<int16_t>(levelParam2.label, levelParam2.value());
		s += htmlConfigEntry<int16_t>(levelParam1.label, levelParam1.value());	
		s += "</ul>";

		s += "Modbus:";
		s += "<ul>";
		s += htmlConfigEntry<int16_t>(modbusPort.label, modbusPort.value());
		s += htmlConfigEntry<int16_t>(modbusID.label, modbusID.value());
		s += "</ul>";

		return s;
	}

	iotwebconf::ParameterGroup *Tank::parameterGroup()
	{
		return &Pump_group;
	}

	bool Tank::validate(iotwebconf::WebRequestWrapper *webRequestWrapper)
	{
		if (requiredParam(webRequestWrapper, levelParam1) == false)
			return false;
		if (requiredParam(webRequestWrapper, levelParam2) == false)
			return false;
		if (requiredParam(webRequestWrapper, levelParam3) == false)
			return false;
		if (requiredParam(webRequestWrapper, levelParam4) == false)
			return false;
		if (levelParam1.value() >= levelParam2.value())
		{
			levelParam1.errorMessage = "Stop must be less than Start Lead";
			return false;
		}
		if (levelParam2.value() >= levelParam3.value())
		{
			levelParam2.errorMessage = "Start Lead must be less than Start Lag";
			return false;
		}
		if (levelParam3.value() >= levelParam4.value())
		{
			levelParam3.errorMessage = "Start Lag must be less than Overflow";
			return false;
		}
		return true;
	}

	void Tank::setup(IOTServiceInterface *iot)
	{
		logd("setup");
		_iot = iot;
		Pump_group.addItem(&levelParam4);
		Pump_group.addItem(&levelParam3);
		Pump_group.addItem(&levelParam2);
		Pump_group.addItem(&levelParam1);
		Modbus_group.addItem(&modbusPort);
		Modbus_group.addItem(&modbusID);
		Pump_group.addItem(&Modbus_group);
		pinMode(PUMP_1, OUTPUT);
		pinMode(PUMP_2, OUTPUT);
		pinMode(PUMP_3, OUTPUT);
		pinMode(PUMP_4, OUTPUT);

	}

	void Tank::onWiFiConnect()
	{
		_lastWaterLevel = 0;
		if (!MBserver.isRunning())
		{
			MBserver.start(modbusPort.value(), 5, 0); // listen for modbus requests
		}
		
		// READ_INPUT_REGISTER
		auto modbusFC04 = [this](ModbusMessage request) -> ModbusMessage{
			
			ModbusMessage response; 
			uint16_t addr = 0;		
			uint16_t words = 0;		
			request.get(2, addr);	
			request.get(4, words);	
			logd("READ_INPUT_REGISTER %d %d[%d]", request.getFunctionCode(), addr, words);
			if ((addr + words) > 5)
			{
				logw("READ_INPUT_REGISTER error: %d", (addr + words));
				response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
			}
			uint16_t WaterLevel = (uint16_t)_Sensor.Level();
			response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
			response.add(WaterLevel);
			return response;
		};
		// READ_COIL
		auto modbusFC01 = [this](ModbusMessage request) -> ModbusMessage{

			ModbusMessage response; // The Modbus message we are going to give back
			uint16_t start = 0;
			uint16_t numCoils = 0;
			request.get(2, start, numCoils);
			logd("READ_COIL %d %d[%d]", request.getFunctionCode(), start, numCoils);
			// Address overflow?
			if ((start + numCoils) > 4)
			{
				logw("READ_COIL error: %d", (start + numCoils));
				response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
			}
			CoilData myCoils(4);
  			myCoils.set(0, digitalRead(PUMP_1));
  			myCoils.set(1, digitalRead(PUMP_2));
			myCoils.set(2, digitalRead(PUMP_3));
			myCoils.set(3, digitalRead(PUMP_4));
			vector<uint8_t> coilset = myCoils.slice(start, numCoils);
			response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)coilset.size(), coilset);
			return response;
		};
		MBserver.registerWorker(modbusID.value(), READ_INPUT_REGISTER, modbusFC04);
		MBserver.registerWorker(modbusID.value(), READ_COIL, modbusFC01);

		asyncServer.begin();
		_webLog.begin(&asyncServer);
		_webSocket.begin();
		_webSocket.onEvent([this](uint8_t num, WStype_t type, uint8_t *payload, size_t length)
						   { 
			if (type == WStype_DISCONNECTED)
			{
				logi("[%u] Home Page Disconnected!\n", num);
			}
			else if (type == WStype_CONNECTED)
			{
				logi("[%u] Home Page Connected!\n", num);
				_lastWaterLevel = -1; //force a broadcast
			} });

		asyncServer.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
			String page = home_html;
			page.replace("{n}", _iot->getThingName().c_str());
			page.replace("{v}", CONFIG_VERSION);
			page.replace("{hp}", String(WSOCKET_HOME_PORT));
			page.replace("{cp}", String(IOTCONFIG_PORT));
			request->send(200, "text/html", page);
		});
	}

	void Tank::Process()
	{
		String s;
		float waterLevel = _Sensor.Level();
		if (waterLevel >= 0 && abs(_lastWaterLevel - waterLevel) > 1.0) // limit broadcast to 1% change
		{
			waterLevel = waterLevel <= 1.0 ? 0 : waterLevel;
			_lastWaterLevel = waterLevel;
			boolean s1 = waterLevel > levelParam1.value();
			boolean s2 = waterLevel > levelParam2.value();
			boolean s3 = waterLevel > levelParam3.value();
			boolean s4 = waterLevel > levelParam4.value();

			digitalWrite(PUMP_1, s1);
			digitalWrite(PUMP_2, s2);
			digitalWrite(PUMP_3, s3);
			digitalWrite(PUMP_4, s4);

			JsonDocument doc;
			doc.clear();
			doc["level"] = waterLevel;
			doc["pump1"] = digitalRead(PUMP_1) ? "on" : "off";
			doc["pump2"] = digitalRead(PUMP_2) ? "on" : "off";
			doc["pump3"] = digitalRead(PUMP_3) ? "on" : "off";
			doc["pump4"] = digitalRead(PUMP_4) ? "on" : "off";
			serializeJson(doc, s);
			_iot->Online();
			_iot->Publish("readings", s.c_str(), false);
			_webSocket.broadcastTXT(s);
			logd("Water Level: %f JSON: %s", waterLevel, s.c_str());
		}
		_webLog.process();
		_webSocket.loop();
		return;
	}

	void Tank::onMqttConnect(bool sessionPresent)
	{
		if (ReadyToPublish())
		{
			logd("Publishing discovery ");
			char buffer[STR_LEN];
			JsonDocument doc;
			JsonObject device = doc["device"].to<JsonObject>();
			device["name"] = _iot->getSubtopicName();
			device["sw_version"] = CONFIG_VERSION;
			device["manufacturer"] = "ClassicDIY";
			sprintf(buffer, "ESP32-Bit (%X)", _iot->getUniqueId());
			device["model"] = buffer;

			JsonObject origin = doc["origin"].to<JsonObject>();
			origin["name"] = TAG;

			JsonArray identifiers = device["identifiers"].to<JsonArray>();
			sprintf(buffer, "%X", _iot->getUniqueId());
			identifiers.add(buffer);

			JsonObject components = doc["components"].to<JsonObject>();
			JsonObject level = components["level"].to<JsonObject>();
			level["platform"] = "sensor";
			level["name"] = "level";
			level["unit_of_measurement"] = "%";
			level["value_template"] = "{{ value_json.level }}";
			sprintf(buffer, "%X_level",  _iot->getUniqueId());
			level["unique_id"] = buffer;
			level["icon"] = "mdi:hydraulic-oil-level";

			JsonObject pump1 = components["pump1"].to<JsonObject>();
			pump1["platform"] = "sensor";
			pump1["name"] = "pump1";
			pump1["value_template"] = "{{ value_json.pump1 }}";
			sprintf(buffer, "%X_pump1",  _iot->getUniqueId());
			pump1["unique_id"] = buffer;
			pump1["icon"] = "mdi:pump";

			JsonObject pump2 = components["pump2"].to<JsonObject>();
			pump2["platform"] = "sensor";
			pump2["name"] = "pump2";
			pump2["value_template"] = "{{ value_json.pump2 }}";
			sprintf(buffer, "%X_pump2",  _iot->getUniqueId());
			pump2["unique_id"] = buffer;
			pump2["icon"] = "mdi:pump";

			JsonObject pump3 = components["pump3"].to<JsonObject>();
			pump3["platform"] = "sensor";
			pump3["name"] = "pump3";
			pump3["value_template"] = "{{ value_json.pump3 }}";
			sprintf(buffer, "%X_pump3",  _iot->getUniqueId());
			pump3["unique_id"] = buffer;
			pump3["icon"] = "mdi:pump";

			JsonObject pump4 = components["pump4"].to<JsonObject>();
			pump4["platform"] = "sensor";
			pump4["name"] = "pump4";
			pump4["value_template"] = "{{ value_json.pump4 }}";
			sprintf(buffer, "%X_pump4",  _iot->getUniqueId());
			pump4["unique_id"] = buffer;
			pump4["icon"] = "mdi:pump";

			sprintf(buffer, "%s/stat/readings", _iot->getRootTopicPrefix().c_str());
			doc["state_topic"] = buffer;
			sprintf(buffer, "%s/tele/LWT", _iot->getRootTopicPrefix().c_str());
			doc["availability_topic"] = buffer;
			doc["pl_avail"] = "Online";
			doc["pl_not_avail"] = "Offline";

			_iot->PublishHADiscovery(doc);
			_discoveryPublished = true;
		}
	}
	void Tank::onMqttMessage(char *topic, JsonDocument &doc)
	{
		logd("onMqttMessage %s", topic);
		// if (doc.containsKey("command"))
		// {
		// 	if (strcmp(doc["command"], "Write Coil") == 0)
		// 	{

		// 	}
		// }
	}
}