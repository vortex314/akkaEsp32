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
#include <Config.h>
#include <ConfigActor.cpp>

#include <Echo.cpp>
#include <Bridge.cpp>
#include <Sender.cpp>
#include <System.h>
#include <Wifi.h>

using namespace std;

/*
 * ATTENTION : TIMER_TASK_PRIORITY needs to be raised to avoid wdt trigger on
 * load test
 */


// make -DTRIAC=1 -DHOSTNAME=triac -DULTRASONIC=2


Log logger(256);
ActorMsgBus eb;

#define BZERO(x) ::memset(&x, 0, sizeof(x))

extern "C" void app_main() {
    esp_log_level_set("*", ESP_LOG_WARN);
    Sys::init();
    nvs_flash_init();
    INFO("Starting Akka on %s heap : %d ", Sys::getProcessor(),
         Sys::getFreeHeap());
#ifdef CONFIGURATION
    config.load(CONFIGURATION);
#else
    config.load();
#endif
    std::string hostname;
    config.setNameSpace("system");
    config.get("hostname", hostname, Sys::hostname());
    Sys::setHostname(hostname.c_str());

    static MessageDispatcher defaultDispatcher(4, 6000, tskIDLE_PRIORITY + 1);
    static ActorSystem actorSystem(Sys::hostname(), defaultDispatcher);
    JsonObject cfg = config.root();

#ifdef MQTT_SERIAL
#include <MqttSerial.h>

    ActorRef& mqtt = actorSystem.actorOf<MqttSerial>("mqttSerial");
#else
#include <Wifi.h>
#include <Mqtt.h>

    ActorRef& wifi = actorSystem.actorOf<Wifi>("wifi");
    ActorRef& mqtt = actorSystem.actorOf<Mqtt>("mqtt", wifi);
#endif
    ActorRef& bridge = actorSystem.actorOf<Bridge>("bridge", mqtt);
    actorSystem.actorOf<System>("system", mqtt);
    actorSystem.actorOf<ConfigActor>("config");

#ifdef REMOTE
#include <Controller.h>
    actorSystem.actorOf<Controller>("remote", bridge);
#endif

#ifdef PROGRAMMER
#include <Programmer.h>
    actorSystem.actorOf<Programmer>("programmer", new Connector(PROGRAMMER),
                                    bridge);
#endif

#ifdef DWM1000_TAG
#include <DWM1000_Tag.h>
    actorSystem.actorOf<DWM1000_Tag>("tag", new Connector(DWM1000_TAG), bridge);
#endif

#ifdef COMPASS
#include <Compass.h>
    actorSystem.actorOf<DigitalCompass>("compass", new Connector(COMPASS), bridge);
#endif

#ifdef LSM303C
#include <LSM303C.h>
    actorSystem.actorOf<LSM303C>("lsm303c", new Connector(LSM303C), bridge);
#endif

#ifdef MOTORSPEED
#include <MotorSpeed.h>
    actorSystem.actorOf<MotorSpeed>("speed", new Connector(MOTORSPEED), bridge);
#endif

#ifdef MOTORSERVO
#include <MotorServo.h>
    actorSystem.actorOf<MotorServo>("steer", new Connector(MOTORSERVO), bridge);
#endif

#ifdef NEO6M
#include <Neo6m.h>
    actorSystem.actorOf<Neo6m>("neo6m", new Connector(NEO6M), bridge);
#endif

#ifdef DIGITAL_COMPASS
#include <DigitalCompass.h>
    actorSystem.actorOf<DigitalCompass>(name, new Connector(DIGITAL_COMPASS),
                                        bridge);
#endif

#ifdef ULTRASONIC
#include <UltraSonic.h>
    actorSystem.actorOf<UltraSonic>(name, new Connector(idx), bridge);
#endif

#ifdef TRIAC
#include <Triac.h>
    actorSystem.actorOf<Triac>(name, new Connector(idx), bridge);
#endif

    config.save();
}
