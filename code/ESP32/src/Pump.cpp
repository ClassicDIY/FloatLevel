#include <Arduino.h>
#include "Log.h"
#include "HelperFunctions.h"
#include "Defines.h"
#include "Pump.h"

namespace FloatLevelNS
{
iotwebconf::ParameterGroup Pump_group = iotwebconf::ParameterGroup("pumps", "Pumps");
iotwebconf::IntTParameter<int16_t> levelParam1 = iotwebconf::Builder<iotwebconf::IntTParameter<int16_t>>("level1").label("Level 1").defaultValue(20).min(1).max(100).step(1).placeholder("1..100").build();
iotwebconf::IntTParameter<int16_t> levelParam2 = iotwebconf::Builder<iotwebconf::IntTParameter<int16_t>>("level2").label("Level 2").defaultValue(40).min(1).max(100).step(1).placeholder("1..100").build();
iotwebconf::IntTParameter<int16_t> levelParam3 = iotwebconf::Builder<iotwebconf::IntTParameter<int16_t>>("level3").label("Level 3").defaultValue(60).min(1).max(100).step(1).placeholder("1..100").build();
iotwebconf::IntTParameter<int16_t> levelParam4 = iotwebconf::Builder<iotwebconf::IntTParameter<int16_t>>("level4").label("Level 4").defaultValue(80).min(1).max(100).step(1).placeholder("1..100").build();


Pump::Pump(int pumpNumber, int gpioPin)
{
	 _pumpNumber = pumpNumber;
     _gpioPin = gpioPin;
}

String Pump::getRootHTML() {
	String s;
	s += "Pump:";
		s += "<ul>";
		s += htmlConfigEntry<int16_t>(levelParam1.label, levelParam1.value());
		s += htmlConfigEntry<int16_t>(levelParam2.label, levelParam2.value());
		s += htmlConfigEntry<int16_t>(levelParam3.label, levelParam3.value());
		s += htmlConfigEntry<int16_t>(levelParam4.label, levelParam4.value());
		s += "</ul>";;
	return s;
}

iotwebconf::ParameterGroup* Pump::parameterGroup() {
	return &Pump_group;
}

bool Pump::validate(iotwebconf::WebRequestWrapper* webRequestWrapper) {
	 if ( requiredParam(webRequestWrapper, levelParam1) == false) return false;
	 if ( requiredParam(webRequestWrapper, levelParam2) == false) return false;
	 if ( requiredParam(webRequestWrapper, levelParam3) == false) return false;
	 if ( requiredParam(webRequestWrapper, levelParam4) == false) return false;
	return true;
}

void Pump::setup(IOTServiceInterface* pcb){
    logd("setup");
	_pcb = pcb;

	Pump_group.addItem(&levelParam1);
    Pump_group.addItem(&levelParam2);
    Pump_group.addItem(&levelParam3);
    Pump_group.addItem(&levelParam4);

	pinMode(BUTTON_1, OUTPUT);
	pinMode(BUTTON_2, OUTPUT);
	pinMode(BUTTON_3, OUTPUT);
	pinMode(BUTTON_4, OUTPUT);
}

void Pump::Process(float waterLevel)
{
    boolean s1 = waterLevel > levelParam1.value();
    boolean s2 = waterLevel > levelParam2.value();
    boolean s3 = waterLevel > levelParam3.value();
    boolean s4 = waterLevel > levelParam4.value();
    
    digitalWrite(BUTTON_1, s1);
    digitalWrite(BUTTON_2, s2);
    digitalWrite(BUTTON_3, s3);
    digitalWrite(BUTTON_4, s4);

	logi("Pump 1 is %s", s1 ? "on" : "off");
    logi("Pump 2 is %s", s2 ? "on" : "off");
    logi("Pump 3 is %s", s3 ? "on" : "off");
    logi("Pump 4 is %s", s4 ? "on" : "off");

    String s;
    JsonDocument doc;
    doc.clear();
    doc["WaterLevel"] = waterLevel;
    serializeJson(doc, s);
    _pcb->Publish("stat", s.c_str(), false);
}

}