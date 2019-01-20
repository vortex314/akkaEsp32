#include "System.h"
#include "Mqtt.h"

#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_spi_flash.h"


void logHeap() {
	INFO(" heap:%d stack:%d heap free :%d largest block : %d ",
			xPortGetFreeHeapSize(), uxTaskGetStackHighWaterMark(NULL),
			heap_caps_get_free_size(MALLOC_CAP_32BIT),
			heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
}

System::System(ActorRef& mqtt) :
		_ledGpio(DigitalOut::create(2)),_mqtt(mqtt) {
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

	.match(Mqtt::Connected,
			[this](Msg& msg) {timers().find(_ledTimer)->interval(1000);})

	.match(Mqtt::Disconnected,
			[this](Msg& msg) {timers().find(_ledTimer)->interval(100);})

	.match(MsgClass::Properties(), [this](Msg& msg) {
		esp_chip_info_t chip_info;
		esp_chip_info(&chip_info);
		sender().tell(replyBuilder(msg)
				("build", __DATE__ " " __TIME__)
				("cpu", "ESP32")
				( "cores", chip_info.cores)
				("upTime", Sys::millis())("ram", 500000)
				( "heap", xPortGetFreeHeapSize())
				("flash",spi_flash_get_chip_size())
				("stack", (uint32_t)uxTaskGetStackHighWaterMark(NULL))
				("revision",chip_info.revision),
				self());
	}).build();
}
