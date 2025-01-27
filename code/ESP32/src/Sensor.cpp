#include <Arduino.h>
#include "Log.h"
#include "Defines.h"
#include "Sensor.h"

namespace FloatLevelNS
{
Sensor::Sensor(int sensorPin)
{
	_sensorPin = sensorPin;
	_count = 0;
	_numberOfSummations = 0;
	_rollingSum = 0;
}

Sensor::~Sensor()
{
}

// WWater level in 0 -> 100% range
float Sensor::WaterLevel()
{
	float rVal = 1;
	// Get the mv value from the analog pin connected to the Sensor
	double sensorVoltage = analogReadMilliVolts(_sensorPin);

	// Convert voltage value to a % level using range of max and min voltages and level for the Sensor
	if (sensorVoltage > SensorVoltageMin)
	{
		// For voltages above minimum value, use the linear relationship to calculate level.
		rVal = (sensorVoltage - SensorVoltageMin) * SensorWaterLevelMax / (SensorVoltageMax - SensorVoltageMin);
	}
	float ws = AddReading(rVal);
	ws = roundf(ws  * 10); // round to 1 decimal place

	#ifdef LOG_SENSOR_VOLTAGE
	if (_count++ > 100)
	{
		logd("Sensor voltage: %f, WaterLevel: %f, rVal:%f", sensorVoltage, ws / 10, rVal);
		_count = 0;
	}
	#endif
	return ws / 10;
}

float Sensor::AddReading(float val)
{
	float currentAvg = 0.0;
	if (_numberOfSummations > 0)
	{
		currentAvg = _rollingSum / _numberOfSummations;
	}
	if (_numberOfSummations < SAMPLESIZE)
	{
		_numberOfSummations++;
	}
	else
	{
		_rollingSum -= currentAvg;
	}
	_rollingSum += val;
	return _rollingSum / _numberOfSummations;
}
} // namespace SensorNS