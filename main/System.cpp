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
    _reportTimer = timers().startPeriodicTimer("REPORT_TIMER",
                                               MsgClass::TimerExpired(), 1000);
    _ledTimer =
        timers().startPeriodicTimer("LED_TIMER", MsgClass::TimerExpired(), 100);
}

Receive& System::createReceive() {
    return receiveBuilder()
        .match(MsgClass::ReceiveTimeout(),
               [this](Envelope& msg) {
                   INFO(" No more messages since some time ");
               })
        .match(MsgClass::TimerExpired(),
               [this](Envelope& msg) {
                   uint16_t k;
                   msg.scanf("i", &k);
                   if (k == _ledTimer) {
                       static bool ledOn = false;
                       _ledGpio.write(ledOn ? 1 : 0);
                       ledOn = !ledOn;
                   } else if (k == _reportTimer) {
                       logHeap();
                       esp_task_wdt_reset();
                   }
               })
        .build();
}