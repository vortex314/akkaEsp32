#include "esp_event.h"
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
#include <Config.h>
#include <ConfigActor.cpp>
#include <Echo.cpp>
#include <Bridge.cpp>
#include <Sender.cpp>
#include <System.h>
#include <Wifi.h>

#include <MqttSerial.h>

#include <Programmer.h>
#include <Controller.h>
#include <DWM1000_Tag.h>
#include <Compass.h>
#include <LSM303C.h>
#include <MotorSpeed.h>
#include <MotorServo.h>
#include <Neo6m.h>
#include <DigitalCompass.h>
#include <UltraSonic.h>
#include <Triac.h>

/*
 * ATTENTION : TIMER_TASK_PRIORITY needs to be raised to avoid wdt trigger on
 * load test
 */

Log logger(256);
ActorMsgBus eb;

#define BZERO(x) ::memset(&x, 0, sizeof(x))
#define STRINGIFY(X) #X
#define S(X) STRINGIFY(X)

extern "C" void app_main()
{

    esp_log_level_set("*", ESP_LOG_INFO);
    Sys::init();
    nvs_flash_init();
    INFO("Starting Akka on %s heap : %d ", Sys::getProcessor(),
         Sys::getFreeHeap());

    std::string str;
    config.load();
    serializeJson(config.root(), str);
    INFO("%s",str.c_str());

    std::string hostname;
#ifdef HOSTNAME
    hostname = S(HOSTNAME);
    config.root()["system"]["hostname"] = S(HOSTNAME);
#else
    hostname = config.root()["system"]["hostname"] | Sys::hostname();
#endif
    Sys::setHostname(hostname.c_str());

    static MessageDispatcher defaultDispatcher(6, 6000, tskIDLE_PRIORITY + 1);
    static ActorSystem actorSystem(Sys::hostname(), defaultDispatcher);

#ifdef MQTT_SERIAL

    ActorRef& mqtt = actorSystem.actorOf<MqttSerial>("mqttSerial");

#else
#include <Wifi.h>
#include <Mqtt.h>
#ifdef WIFI_SSID
    config.root()["wifi"]["prefix"]=S(WIFI_SSID);
#endif
#ifdef WIFI_PASS
    config.root()["wifi"]["password"]=S(WIFI_PASS);
#endif
    ActorRef& wifi = actorSystem.actorOf<Wifi>("wifi");
    ActorRef& mqtt = actorSystem.actorOf<Mqtt>("mqtt", wifi);
#endif
    ActorRef& bridge = actorSystem.actorOf<Bridge>("bridge", mqtt);
    actorSystem.actorOf<System>("system", mqtt);
    actorSystem.actorOf<ConfigActor>("config");

#ifdef REMOTE
    actorSystem.actorOf<Controller>("remote", bridge);
#endif

#ifdef PROGRAMMER
    actorSystem.actorOf<Programmer>("programmer", new Connector(PROGRAMMER),
                                    bridge);
#endif

#ifdef DWM1000_TAG
    actorSystem.actorOf<DWM1000_Tag>("tag", new Connector(DWM1000_TAG), bridge);
#endif

#ifdef COMPASS
    actorSystem.actorOf<Compass>("compass", new Connector(COMPASS),
                                 bridge);
#endif

#ifdef LSM303C
    actorSystem.actorOf<LSM303C>("lsm303c", new Connector(LSM303C), bridge);
#endif

#ifdef MOTORSPEED
    actorSystem.actorOf<MotorSpeed>("speed", new Connector(MOTORSPEED), bridge);
#endif

#ifdef MOTORSERVO
    actorSystem.actorOf<MotorServo>("steer", new Connector(MOTORSERVO), bridge);
#endif

#ifdef NEO6M
    actorSystem.actorOf<Neo6m>("neo6m", new Connector(NEO6M), bridge);
#endif

#ifdef DIGITAL_COMPASS
    actorSystem.actorOf<DigitalCompass>(name, new Connector(DIGITAL_COMPASS),
                                        bridge);
#endif

#ifdef ULTRASONIC
    actorSystem.actorOf<UltraSonic>(name, new Connector(idx), bridge);
#endif

#ifdef TRIAC
    actorSystem.actorOf<Triac>(name, new Connector(idx), bridge);
#endif
    str.clear();
    serializeJson(config.root(), str);
    INFO("%s",str.c_str());
    config.save();
}
