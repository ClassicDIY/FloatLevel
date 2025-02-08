#pragma once
#include <Arduino.h>
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
		Tank();
		void setup(IOTServiceInterface* pcb);
		void begin();
		String Process();
		//IOTCallbackInterface 
		String getSettingsHTML() ;
		iotwebconf::ParameterGroup* parameterGroup() ;
		bool validate(iotwebconf::WebRequestWrapper* webRequestWrapper);
		void onMqttConnect(bool sessionPresent);

	
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
