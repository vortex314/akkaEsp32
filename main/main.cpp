#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "esp_log.h"
#include "mqtt_client.h"

static const char* TAG = "MQTT_EXAMPLE";

static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;
#include <Log.h>
#include <stdio.h>
#include <string>

// esp_err_t event_handler(void *ctx, system_event_t *event) { return ESP_OK; }

#include "driver/spi_master.h"
#include "esp_system.h"

#include <Akka.cpp>
#include <Native.cpp>
#include <Echo.cpp>
#include <Bridge.cpp>
#include <Mqtt.h>
#include <Sender.cpp>
#include <System.h>
#include <Wifi.h>
#include <Publisher.cpp>
#include <ConfigActor.cpp>
#include <Config.h>
#include <UltraSonic.h>
#include <Compass.h>
#include <Gps.h>

using namespace std;

/*
 * ATTENTION : TIMER_TASK_PRIORITY needs to be raised to avoid wdt trigger on load test
 */

Log logger(256);
ActorMsgBus eb;

#define BZERO(x) ::memset(&x, 0, sizeof(x))

extern void XdrTester(uint32_t max);


extern "C" void app_main()
{
    Sys::init();
    nvs_flash_init();
    INFO("Starting Akka on %s heap : %d ", Sys::getProcessor(), Sys::getFreeHeap());
    std::string output;
    config.load();

    static MessageDispatcher defaultDispatcher(3,6000,tskIDLE_PRIORITY+1);
    static ActorSystem actorSystem(Sys::hostname(), defaultDispatcher);

//    actorSystem.actorOf<Sender>("sender");
    ActorRef& wifi = actorSystem.actorOf<Wifi>("wifi");

    ActorRef& mqtt = actorSystem.actorOf<Mqtt>("mqtt", wifi,"tcp://limero.ddns.net:1883");
    ActorRef& bridge = actorSystem.actorOf<Bridge>("bridge",mqtt);
//	defaultDispatcher.unhandled(bridge.cell())
    actorSystem.actorOf<System>("system",mqtt);
    actorSystem.actorOf<ConfigActor>("config");
    ActorRef& publisher = actorSystem.actorOf<Publisher>("publisher",mqtt);

    config.setNameSpace("Gps");
    uint32_t uextNumber;
    config.get("connector",uextNumber,0);
    if ( uextNumber ) {
        actorSystem.actorOf<Gps>("gps",new Connector(uextNumber),mqtt);
    }

    config.setNameSpace("Compass");
    config.get("connector",uextNumber,0);
    if ( uextNumber ) {
        actorSystem.actorOf<Compass>("compass",new Connector(uextNumber),mqtt);
    }

    config.setNameSpace("UltraSonic");
    config.get("connector",uextNumber,0);
    if ( uextNumber ) {
        actorSystem.actorOf<UltraSonic>("ultraSonic",new Connector(uextNumber),mqtt);
    }
    config.save();
}
