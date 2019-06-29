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
#include <ConfigActor.cpp>
#include <Config.h>
#include <UltraSonic.h>
#include <Compass.h>
#include <Neo6m.h>
#include <LSM303C.h>
#include <DigitalCompass.h>
#include <Triac.h>
#include <DWM1000_Tag.h>
#include <Programmer.h>
#include <Controller.h>
#include <MotorSpeed.h>
#include <MotorServo.h>
using namespace std;

/*
 * ATTENTION : TIMER_TASK_PRIORITY needs to be raised to avoid wdt trigger on load test
 */
#define CONTROLLER  "{\"uext\":[\"controller\"],\"controller\":{\"class\":\"Controller\"},\"system\":{\"hostname\":\"remote\"},\"mqtt\":{\"host\":\"limero.ddns.net\",\"port\":1883},\"wifi\":{\"ssid\":\"Merckx\",\"password\":\"LievenMarletteEwoutRonald\"}}"
#define MOTOR "{\"uext\":[\"motor\"],\"motor\":{\"class\":\"MotorSpeed\"},\"system\":{\"hostname\":\"drive\"},\"mqtt\":{\"host\":\"limero.ddns.net\",\"port\":1883},\"wifi\":{\"ssid\":\"Merckx\",\"password\":\"LievenMarletteEwoutRonald\"}}"
#define SERVO "{\"uext\":[\"servo\"],\"servo\":{\"class\":\"MotorServo\"},\"system\":{\"hostname\":\"drive\"},\"mqtt\":{\"host\":\"limero.ddns.net\",\"port\":1883},\"wifi\":{\"ssid\":\"Merckx\",\"password\":\"LievenMarletteEwoutRonald\"}}"


#define GENERIC         "{\"uext\":[],\"mqtt\":{\"host\":\"limero.ddns.net\",\"port\":1883},\"wifi\":{\"ssid\":\"Merckx\",\"password\":\"LievenMarletteEwoutRonald\"}}"
#define DWM1000_TAG "{\"uext\":[\"dwm1000Tag\"],\"dwm1000Tag\":{\"class\":\"DWM1000_Tag\"},\"system\":{\"hostname\":\"tag\"},\"mqtt\":{\"host\":\"limero.ddns.net\",\"port\":1883},\"wifi\":{\"ssid\":\"Merckx\",\"password\":\"LievenMarletteEwoutRonald\"}}"


#define CONFIGURATION SERVO

Log logger(256);
ActorMsgBus eb;

#define BZERO(x) ::memset(&x, 0, sizeof(x))

extern void XdrTester(uint32_t max);

extern "C" void app_main()
{
    esp_log_level_set("*", ESP_LOG_WARN);
    /*	esp_log_level_set("MQTT_CLIENT", ESP_LOG_DEBUG);
    	esp_log_level_set("TRANSPORT_TCP", ESP_LOG_DEBUG);
    	esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    	esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    	esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
    	*/

    Sys::init();
    nvs_flash_init();
    INFO("Starting Akka on %s heap : %d ", Sys::getProcessor(),Sys::getFreeHeap());
    INFO(" hash test : %d vs %d ",H("$dst"),H("ESP32-12857/wifi"));
    std::string output;
#ifdef GENERIC
    std::string conf =
        "{\"uext\":[],\"mqtt\":{\"host\":\"limero.ddns.net\",\"port\":1883},\"wifi\":{\"ssid\":\"Merckx\",\"password\":\"LievenMarletteEwoutRonald\"}}";
    config.setNameSpace("system");
    config.set("hostname",Sys::hostname());
#endif


    std::string conf4 =
        "{\"uext\":[\"programmer\"],\"programmer\":{\"class\":\"Programmer\"},\"mqtt\":{\"host\":\"limero.ddns.net\",\"port\":1883},\"wifi\":{\"ssid\":\"Merckx\",\"password\":\"LievenMarletteEwoutRonald\"}}";

    std::string conf2 =
        "{\"uext\":[\"compass\",\"us\"],\"compass\":{\"class\":\"DigitalCompass\"},\"us\":{\"class\":\"UltraSonic\"},\"mqtt\":{\"host\":\"limero.ddns.net\",\"port\":1883},\"wifi\":{\"ssid\":\"Merckx\",\"password\":\"LievenMarletteEwoutRonald\"}}";
    std::string conf1 =
        "{\"uext\":[\"gps\",\"us\"],\"gps\":{\"class\":\"NEO6M\"},\"us\":{\"class\":\"UltraSonic\"},\"mqtt\":{\"host\":\"limero.ddns.net\",\"port\":1883},\"wifi\":{\"ssid\":\"Merckx\",\"password\":\"LievenMarletteEwoutRonald\"}}";
    std::string conf0 =
        "{\"uext\":[\"triac\"],\"triac\":{\"class\":\"Triac\"},\"us\":{\"class\":\"UltraSonic\"},\"mqtt\":{\"host\":\"limero.ddns.net\",\"port\":1883},\"wifi\":{\"ssid\":\"Merckx\",\"password\":\"LievenMarletteEwoutRonald\"}}";
    config.load(CONFIGURATION);
    std::string hostname;
    config.setNameSpace("system");
    config.get("hostname",hostname,Sys::hostname());
    Sys::setHostname(hostname.c_str());
    INFO(" config %s",conf.c_str());

    static MessageDispatcher defaultDispatcher(4, 6000, tskIDLE_PRIORITY + 1);
    static ActorSystem actorSystem(Sys::hostname(), defaultDispatcher);

    ActorRef& wifi = actorSystem.actorOf<Wifi>("wifi");
    ActorRef& mqtt = actorSystem.actorOf<Mqtt>("mqtt", wifi,"tcp://limero.ddns.net:1883");
    ActorRef& bridge = actorSystem.actorOf<Bridge>("bridge", mqtt);
    actorSystem.actorOf<System>("system", mqtt);
    actorSystem.actorOf<ConfigActor>("config");

    JsonObject cfg = config.root();
    JsonArray uexts = cfg["uext"].as<JsonArray>();
    int idx = 0;
    for (const char* name : uexts) {
        idx++;				// starts at 1
        const char* peripheral = cfg[name]["class"] | "";
        if (strlen(peripheral) > 0) {
            switch (H(peripheral)) {
            case H("Controller"): {
                actorSystem.actorOf<Controller>(name,bridge);
                break;
            }
            case H("Programmer"): {
                actorSystem.actorOf<Programmer>(name, new Connector(idx),
                                                mqtt);
                break;
            }
            case H("DWM1000_Tag"): {
                actorSystem.actorOf<DWM1000_Tag>(name, new Connector(idx),
                                                 bridge);
                break;
            }
            case H("Compass"): {
                actorSystem.actorOf<DigitalCompass>(name, new Connector(idx),
                                                    bridge);
                break;
            }
            case H("LSM303C"): {
                actorSystem.actorOf<LSM303C>(name, new Connector(idx),
                                             bridge);
                break;
            }
            case H("MotorSpeed"): {
                actorSystem.actorOf<MotorSpeed>(name, new Connector(idx), bridge);
                break;
            }
            case H("MotorServo"): {
                actorSystem.actorOf<MotorServo>(name, new Connector(idx), bridge);
                break;
            }
            case H("NEO6M"): {
                actorSystem.actorOf<Neo6m>(name, new Connector(idx), bridge);
                break;
            }
            case H("DigitalCompass"): {
                actorSystem.actorOf<DigitalCompass>(name, new Connector(idx),
                                                    bridge);
                break;
            }
            case H("UltraSonic"): {
                actorSystem.actorOf<UltraSonic>(name, new Connector(idx),
                                                bridge);
                break;
            }
            case H("Triac"): {
                actorSystem.actorOf<Triac>(name, new Connector(idx), bridge);
                break;
            }
            default: {
                ERROR("peripheral '%s' not found", peripheral);
            }
            }
        } else {
            ERROR("peripheral '%s' class not found ", peripheral);
        }

    }

    config.save();

}
