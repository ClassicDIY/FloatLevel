#pragma once
#include <Arduino.h>
#include "Defines.h"
#include <IotWebConfTParameter.h>
#include "IOTCallbackInterface.h"
#include <ModbusServerTCPasync.h>
#include "IOTServiceInterface.h"
#include "AnalogSensor.h"

namespace FloatLevelNS
{
	class Tank : public IOTCallbackInterface
	{
	
	public:
		Tank(): MBserver(){};
		void setup(IOTServiceInterface* pcb);
		void Process();
		//IOTCallbackInterface 
		String getSettingsHTML() ;
		iotwebconf::ParameterGroup* parameterGroup() ;
		bool validate(iotwebconf::WebRequestWrapper* webRequestWrapper);
		void onMqttConnect(bool sessionPresent);
		void onMqttMessage(char* topic, JsonDocument& doc);
		void onWiFiConnect();
	
	protected:
		boolean PublishDiscoverySub(const char *component, const char *entityName, const char *jsonElement, const char *device_class, const char *unit_of_meas, const char *icon = "");
		bool ReadyToPublish() {
			return (!_discoveryPublished);
		}
	private:
		IOTServiceInterface* _iot;
		boolean _discoveryPublished = false;
		AnalogSensor _Sensor = AnalogSensor(SensorPin);
		ModbusServerTCPasync MBserver;
		float _lastWaterLevel = 0;
	};
}

