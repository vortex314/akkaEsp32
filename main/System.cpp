#include "Mqtt.h"
#include "System.h"

#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_task_wdt.h"

void logHeap() {
	INFO(" heap:%d stack:%d ",
	     xPortGetFreeHeapSize(),
//	     heap_caps_get_free_size(MALLOC_CAP_32BIT),
//	     heap_caps_get_largest_free_block(MALLOC_CAP_32BIT),
	     uxTaskGetStackHighWaterMark(NULL));
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
	_extern = context().system().actorFor("ESP32-56895/system");
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
			esp_task_wdt_reset();
			Envelope envelope(self(), _extern, MsgClass("Reset"));
			//                      _extern.tell(self(), envelope);
		}
	})
	.match(
	    Mqtt::Connected,
	[this](Envelope& msg) { timers().find(_ledTimer)->interval(1000); })
	.match(
	    Mqtt::Disconnected,
	[this](Envelope& msg) { timers().find(_ledTimer)->interval(100); })
	.match(Properties(),[this](Envelope& msg) {
		INFO(" Properties requested ");

		Msg m(PropertiesReply());
		m("cpu","ESP32");
		m("procs",2);
		m("upTime",Sys::millis());
		m("ram",500000);
		m("heap",xPortGetFreeHeapSize());
		sender().tell(m,self());
	})
	.build();
}
