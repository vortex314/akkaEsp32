#include "System.h"

#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_task_wdt.h"

void logHeap() {
    INFO(" heap:%d max_block:%d stack:%d ",
         heap_caps_get_free_size(MALLOC_CAP_32BIT),
         heap_caps_get_largest_free_block(MALLOC_CAP_32BIT),
         uxTaskGetStackHighWaterMark(NULL));
}

System::System(va_list args) : _ledGpio(DigitalOut::create(2)) {}

System::~System() {}

void System::preStart() {
    _ledGpio.init();
    _reportTimer =
        timers().startPeriodicTimer("REPORT_TIMER", TimerExpired, 5000);
    _ledTimer = timers().startPeriodicTimer("LED_TIMER", TimerExpired, 300);
    _extern = context().system().actorFor("ESP32-56895/system");
}

Receive& System::createReceive() {
    return receiveBuilder()
        .match(ReceiveTimeout,
               [this](Envelope& msg) {
                   INFO(" No more messages since some time ");
               })
        .match(TimerExpired,
               [this](Envelope& msg) {
                   uint16_t k;
                   msg.scanf("i", &k);
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
        .build();
}