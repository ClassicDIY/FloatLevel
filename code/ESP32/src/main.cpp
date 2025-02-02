#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <time.h>
#include <ThreadController.h>
#include <Thread.h>
#include "ModbusServerTCPasync.h"
#include "Log.h"
#include "IOT.h"
#include "Sensor.h"
#include "Tank.h"

#define WATCHDOG_TIMER 600000 // time in ms to trigger the watchdog

using namespace FloatLevelNS;

WebServer _webServer(80);
WebSocketsServer _webSocket = WebSocketsServer(81);
boolean _wsConnected = false;
ThreadController _controller = ThreadController();
Thread *_workerThreadWaterLevelMonitor = new Thread();
IOT _iot = IOT(&_webServer);
Tank* _tank = new Tank();

hw_timer_t *_watchdogTimer = NULL;

void IRAM_ATTR resetModule()
{
	// ets_printf("watchdog timer expired - rebooting\n");
	esp_restart();
}

void init_watchdog()
{
	if (_watchdogTimer == NULL)
	{
		_watchdogTimer = timerBegin(0, 80, true);					   // timer 0, div 80
		timerAttachInterrupt(_watchdogTimer, &resetModule, true);	   // attach callback
		timerAlarmWrite(_watchdogTimer, WATCHDOG_TIMER * 1000, false); // set time in us
		timerAlarmEnable(_watchdogTimer);							   // enable interrupt
	}
}

void feed_watchdog()
{
	if (_watchdogTimer != NULL)
	{
		timerWrite(_watchdogTimer, 0); // feed the watchdog
	}
}

void runWaterLevelMonitor()
{
	String s = _tank->Process();
	if (_wsConnected)
	{
		_webSocket.broadcastTXT(s.c_str(), s.length());
	}
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t lenght)
{
	switch (type)
	{
	case WStype_DISCONNECTED:
		logd("[%u] Disconnected!\r\n", num);
		_wsConnected = false;
		break;
	case WStype_CONNECTED:
	{
		IPAddress ip = _webSocket.remoteIP(num);
		logd("Client #[%u] connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
		_wsConnected = true;
		_tank->begin();
	}
	break;
	case WStype_ERROR:
		logd("[%u] WStype_ERROR!\r\n", num);
		break;
	default:
		logd("WStype: %u\r\n", type);
		break;
	}
}

void setupFileSystem()
{
	if (!SPIFFS.begin())
	{
		logd("Failed to mount file system");
	}
	else
	{
		File root = SPIFFS.open("/");
		File file = root.openNextFile();
		while (file)
		{
			logd("FILE: %s", file.name());
			file = root.openNextFile();
		}
	}
}

void WiFiEvent(WiFiEvent_t event)
{
	switch (event)
	{
	case SYSTEM_EVENT_STA_GOT_IP:
		// start webSocket server
		_wsConnected = false;
		_webSocket.begin();
		_webSocket.onEvent(webSocketEvent);
		_tank->begin();
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		_webSocket.close();
		break;
	default:
		break;
	}
}

void handleRoot()
{
	File f = SPIFFS.open("/index.htm", "r");
	if (!f)
	{
		logw("***********index.htm not uploaded to device, use PlatformIO -> Upload Filesystem Image***********");
		return;
	}
	_webServer.streamFile(f, "text/html");
}

void setup()
{
	Serial.begin(115200);
	while (!Serial)
	{
		; // wait for serial port to connect.
	}
	// Configure main worker thread
	_workerThreadWaterLevelMonitor->onRun(runWaterLevelMonitor);
	_workerThreadWaterLevelMonitor->setInterval(20);
	_controller.add(_workerThreadWaterLevelMonitor);
	setupFileSystem();
	WiFi.onEvent(WiFiEvent);
	_tank->setup(&_iot);
	_iot.Init(_tank);
	init_watchdog();
	_webServer.on("/", handleRoot);
	logd("Setup Done");
}

void loop()
{
	_iot.Run();
	if (WiFi.isConnected())
	{
		_controller.run();
		_webSocket.loop();
	}
	feed_watchdog();
}
