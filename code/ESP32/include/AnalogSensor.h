#pragma once
#include <Arduino.h>
#include <sstream> 
#include <string>
#include "defines.h"

namespace FloatLevelNS
{
	class AnalogSensor
	{
	public:
		
		AnalogSensor(int sensorPin);
		~AnalogSensor();
		std::string Pin();
		float Level();

	private:
		int _sensorPin; // Defines the pin that the sensor is connected to
		float AddReading(float val);
		float _rollingSum;
		int _numberOfSummations;
		int _count;
	};
}
