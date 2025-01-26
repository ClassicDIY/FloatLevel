#pragma once
#include <Arduino.h>
#include "IOTCallbackInterface.h"
#include <IotWebConfTParameter.h>
#include "IOTServiceInterface.h"

namespace FloatLevelNS
{
	class Pump : public IOTCallbackInterface
	{
	
	public:
		Pump(int pumpNumber, int gpioPin);
		void setup(IOTServiceInterface* pcb);
		void Process(float waterLevel);
		//IOTCallbackInterface 
		String getRootHTML() ;
		iotwebconf::ParameterGroup* parameterGroup() ;
		bool validate(iotwebconf::WebRequestWrapper* webRequestWrapper);
		void onMqttConnect(bool sessionPresent);
		
	protected:
		boolean PublishDiscoverySub(const char *component, const char *entityName, const char *jsonElement, const char *device_class, const char *unit_of_meas, const char *icon = "");
		bool ReadyToPublish() {
			return (!_discoveryPublished);
		}
	private:
		int _pumpNumber;
		int _gpioPin; // Defines the pin that the pump relay is connected to
		IOTServiceInterface* _pcb;
		boolean _discoveryPublished = false;
	};
}
