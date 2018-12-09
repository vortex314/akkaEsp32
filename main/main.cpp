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
#include <Echo.cpp>
#include <MqttBridge.h>
#include <Sender.cpp>
#include <System.h>
#include <Uid.cpp>
#include <Wifi.h>

using namespace std;

Log logger(256);

#define BZERO(x) ::memset(&x, sizeof(x), 0)

void blink_task(void* pvParameter) {
    nvs_flash_init();

    INFO("Starting Akka ");
    Sys::init();
    MessageDispatcher& defaultDispatcher = *new MessageDispatcher();
    INFO("");
    Mailbox defaultMailbox = *new Mailbox("default", 20000, 1000);
    INFO("");
    Mailbox remoteMailbox = *new Mailbox("remote", 20000, 1000);
    INFO("");
    ActorSystem actorSystem(Sys::hostname(), defaultDispatcher, defaultMailbox);

    INFO("");
    ActorRef sender = actorSystem.actorOf<Sender>("Sender");
    ActorRef wifi = actorSystem.actorOf<Wifi>("Wifi");
    ActorRef system = actorSystem.actorOf<System>("System");

    /*   ActorRef mqttBridge = actorSystem.actorOf<MqttBridge>(
           Props::create()
               .withMailbox(remoteMailbox)
               .withDispatcher(defaultDispatcher),
           "mqttBridge", "tcp://test.mosquitto.org:1883");*/
    defaultDispatcher.attach(defaultMailbox);
    defaultDispatcher.attach(remoteMailbox);
    //    defaultDispatcher.unhandled(ActorCell::lookup(&mqttBridge));

    while (true) {
        defaultDispatcher.execute();
        if (defaultDispatcher.nextWakeup() > Sys::millis()) {
            uint32_t delay = (defaultDispatcher.nextWakeup() - Sys::millis());
            if (delay > 10) {
                if (delay > 10000) {
                    WARN(" big delay %u", delay);
                } else {
                    vTaskDelay(delay / 10); // is in 10 msec multiples
                }
            }
        }
    }
}

extern "C" void app_main() {
    xTaskCreate(&blink_task, "blink_task", 5000, NULL, tskIDLE_PRIORITY + 1,
                NULL);
}
