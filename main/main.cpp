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
#include <Uid.cpp>

using namespace std;

Log logger(256);

void setHostname() {
    nvs_flash_init();
    union {
        uint8_t my_id[6];
        uint32_t word[2];
    };
    esp_efuse_mac_get_default(my_id);
    string hn = "ESP-";
    string_format(hn, "ESP-%X%X", word[0], word[1]);
    INFO(" host : %s", hn.c_str());
    Sys::hostname(hn.c_str());
}

void logHeap() {
    INFO(" heap %d max block %d ", heap_caps_get_free_size(MALLOC_CAP_32BIT),
         heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
    //  heap_caps_print_heap_info(MALLOC_CAP_32BIT);
}

#define BZERO(x) ::memset(&x, sizeof(x), 0)

extern "C" void app_main() {
    logHeap();
    nvs_flash_init();
    Sys::init();
    setHostname();
    Mailbox defaultMailbox = *new Mailbox("default", 20000, 1000);
    Mailbox remoteMailbox = *new Mailbox("remote", 20000, 1000);
    MessageDispatcher& defaultDispatcher = *new MessageDispatcher();
    ActorSystem actorSystem(Sys::hostname(), defaultDispatcher, defaultMailbox);

    ActorRef sender = actorSystem.actorOf<Sender>("Sender");
    ActorRef mqttBridge = actorSystem.actorOf<MqttBridge>(
        Props::create()
            .withMailbox(remoteMailbox)
            .withDispatcher(defaultDispatcher),
        "mqttBridge", "tcp://test.mosquitto.org:1883");
    defaultDispatcher.attach(defaultMailbox);
    defaultDispatcher.attach(remoteMailbox);
    //    defaultDispatcher.unhandled(ActorCell::lookup(&mqttBridge));

    while (true) {
        defaultDispatcher.execute();
        if (defaultDispatcher.nextWakeup() > Sys::millis()) {
            Sys::delay(defaultDispatcher.nextWakeup() - Sys::millis());
        }
    }
}
