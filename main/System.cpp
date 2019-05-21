#include "System.h"
#include "Mqtt.h"

#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_spi_flash.h"

MsgClass System::LedPulseOn("LedPulseOn");
MsgClass System::LedPulseOff("LedPulseOff");

void logHeap() {
	INFO(" heap:%d stack:%d heap free :%d largest block : %d ", xPortGetFreeHeapSize(), uxTaskGetStackHighWaterMark(NULL), heap_caps_get_free_size(MALLOC_CAP_32BIT), heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
}

System::System(ActorRef& mqtt)
		: _ledGpio(DigitalOut::create(2)), _mqtt(mqtt) {
	_propertyIndex = 0;
}

System::~System() {
}

void System::preStart() {
	eb.subscribe(self(), MessageClassifier(_mqtt, Mqtt::Connected));
	eb.subscribe(self(), MessageClassifier(_mqtt, Mqtt::Disconnected));
	_ledGpio.init();
	_reportTimer = timers().startPeriodicTimer("T1", Msg("reportTimer"), 5000);
	_ledTimer = timers().startPeriodicTimer("T2", Msg("ledTimer"), 100);
}

Receive& System::createReceive() {
	return receiveBuilder().match(MsgClass::ReceiveTimeout(), [this](Msg& msg) {
		INFO(" No more messages since some time ");
	})

	.match(MsgClass("ledTimer"), [this](Msg& msg) {
		static bool ledOn = false;
		_ledGpio.write(ledOn ? 1 : 0);
		ledOn = !ledOn;
	})

	.match(MsgClass("reportTimer"), [this](Msg& msg) {logHeap();})

	.match(Mqtt::Connected, [this](Msg& msg) {timers().find(_ledTimer)->interval(500);})

	.match(Mqtt::Disconnected, [this](Msg& msg) {timers().find(_ledTimer)->interval(100);})

	.match(MsgClass::Properties(), [this](Msg& msg) {
		esp_chip_info_t chip_info;
		esp_chip_info(&chip_info);
		Msg& reply=replyBuilder(msg);
		switch (_propertyIndex ) {
			case 0 : {reply("build", __DATE__ " " __TIME__);
				break;}
			case 2: {reply("cpu", "ESP32");
				break;}
			case 3: {reply( "cores", chip_info.cores);
				break;}
			case 4: {reply("upTime", Sys::millis())("ram", 500000);
				break;}
			case 5: {reply( "heap", xPortGetFreeHeapSize());
				break;}
			case 6: {reply("flash",spi_flash_get_chip_size());
				break;}
			case 7: {reply("stack", (uint32_t)uxTaskGetStackHighWaterMark(NULL));
				break;}
			case 1: {reply("revision",chip_info.revision);
				break;}
			default:
			_propertyIndex=-1;
		};
		_propertyIndex++;
		sender().tell(reply, self());
	}).build();
}
