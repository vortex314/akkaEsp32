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
#include <Neo6m.h>
#include <LSM303C.h>
#include <DigitalCompass.h>

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
    std::string conf="{\"board\":{\"uext\":[\"Compass\"]},\"LSM303C\":"
                     "{\"actor\":\"compass\"},\"mqtt\":{\"host\":\"limero.ddns.net\",\"port\":1883},"
                     "\"wifi\":{\"ssidPrefix\":\"Merckx\",\"password\":\"LievenMarletteEwoutRonald\"}}";
    config.load(conf);

    static MessageDispatcher defaultDispatcher(4,6000,tskIDLE_PRIORITY+1);
    static ActorSystem actorSystem(Sys::hostname(), defaultDispatcher);

//    actorSystem.actorOf<Sender>("sender");
    ActorRef& wifi = actorSystem.actorOf<Wifi>("wifi");

    ActorRef& mqtt = actorSystem.actorOf<Mqtt>("mqtt", wifi,"tcp://limero.ddns.net:1883");
    ActorRef& bridge = actorSystem.actorOf<Bridge>("bridge",mqtt);
//	defaultDispatcher.unhandled(bridge.cell())
    actorSystem.actorOf<System>("system",mqtt);
    actorSystem.actorOf<ConfigActor>("config");
    ActorRef& publisher = actorSystem.actorOf<Publisher>("publisher",mqtt);

    JsonObject cfg = config.root();
    JsonArray uexts= cfg["board"]["uext"].as<JsonArray>();

    for(int idx=0; idx<uexts.size(); idx++) {
        const char* peripheral= uexts[idx].as<const char*>();
        if ( strlen(peripheral)>0 ) {
            switch(H(peripheral)) {
            case H("Compass") : {
                const char* name=cfg["Compass"]["actor"] | "compass";
                actorSystem.actorOf<DigitalCompass>(name,new Connector(idx+1),publisher);
                break;
            }
            case H("LSM303C") : {
                const char* name=cfg["LSM303C"]["actor"] | "compass";
                actorSystem.actorOf<LSM303C>(name,new Connector(idx+1),publisher);
                break;
            }
            case H("NEO6M") : {
                const char* name=cfg["NEO6M"]["actor"] | "gps";
                actorSystem.actorOf<Neo6m>(name,new Connector(idx+1),publisher);
                break;
            }
            case H("US") : {
                const char* name=cfg["US"]["actor"] | "us";
                actorSystem.actorOf<UltraSonic>(name,new Connector(idx+1),publisher);
                break;
            }
            }
        }

    }

    config.save();
}
