#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <time.h>
#include <ThreadController.h>
#include <Thread.h>
#include "Log.h"
#include "IOT.h"
#include "Tank.h"

using namespace FloatLevelNS;

IOT _iot = IOT();
ThreadController _controller = ThreadController();
Thread *_workerThreadWaterLevelMonitor = new Thread();
Tank* _tank = new Tank();

void setup()
{
	Serial.begin(115200);
	while (!Serial) {}
	// Configure main worker thread
	_workerThreadWaterLevelMonitor->onRun([] () { _tank->Process(); });
	_workerThreadWaterLevelMonitor->setInterval(20);
	_controller.add(_workerThreadWaterLevelMonitor);
	_tank->setup(&_iot);
	_iot.Init(_tank);
	logd("Setup Done");
}

void loop()
{
	_iot.Run();
	if (WiFi.isConnected())
	{
		_controller.run();
	}
}
