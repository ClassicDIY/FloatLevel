#pragma once
#include <Arduino.h>

namespace FloatLevelNS
{
	class Sensor
	{
		int _sensorPin; // Defines the pin that the sensor is connected to
		
	public:
		Sensor(int sensorPin);
		~Sensor();
		float WaterLevel();
	private:
		float AddReading(float val);
		float _rollingSum;
		int _numberOfSummations;
	};
}
