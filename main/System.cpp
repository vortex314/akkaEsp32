#include "Mqtt.h"
#include "System.h"

#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_task_wdt.h"

void logHeap() {
	INFO(" heap:%d stack:%d heap free :%d largest block : %d ",
	     xPortGetFreeHeapSize(),
	     uxTaskGetStackHighWaterMark(NULL),
	     heap_caps_get_free_size(MALLOC_CAP_32BIT),
	     heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
}

System::System(va_list args) : _ledGpio(DigitalOut::create(2)) {
	_mqtt = va_arg(args,ActorRef) ;
}

System::~System() {}

void System::preStart() {
	eb.subscribe(self(), MessageClassifier(_mqtt, Mqtt::Connected));
	eb.subscribe(self(), MessageClassifier(_mqtt, Mqtt::Disconnected));
	_ledGpio.init();
	_reportTimer =
	    timers().startPeriodicTimer("REPORT_TIMER", TimerExpired(), 5000);
	_ledTimer = timers().startPeriodicTimer("LED_TIMER", TimerExpired(), 100);
}

Receive& System::createReceive() {
	return receiveBuilder()

	       .match(ReceiveTimeout(),
	[this](Envelope& msg) {
		INFO(" No more messages since some time ");
	})

	.match(TimerExpired(),
	[this](Envelope& msg) {
		uint16_t k;
		msg.get(UID_TIMER, k);
		if (Uid(k) == _ledTimer) {
			static bool ledOn = false;
			_ledGpio.write(ledOn ? 1 : 0);
			ledOn = !ledOn;
		} else if (Uid(k) == _reportTimer) {
			logHeap();
		}
	})

	.match(
	    Mqtt::Connected,
	[this](Envelope& msg) { timers().find(_ledTimer)->interval(1000); })

	.match(
	    Mqtt::Disconnected,
	[this](Envelope& msg) { timers().find(_ledTimer)->interval(100); })

	.match(Properties(),[this](Envelope& msg) {
		sender().tell(msg.reply()
		              ("cpu","ESP32")
		              ("procs",2)
		              ("upTime",Sys::millis())
		              ("ram",500000)
		              ("heap",xPortGetFreeHeapSize())
		              ,self());
	})
	.build();
}
