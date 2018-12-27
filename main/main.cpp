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
#include <Bridge.cpp>
#include <Mqtt.h>
#include <Sender.cpp>
#include <System.h>
#include <Wifi.h>

using namespace std;

Log logger(256);
ActorMsgBus eb;

#define BZERO(x) ::memset(&x, 0, sizeof(x))

extern void XdrTester(uint32_t max);



void akkaMainTask(void* pvParameter) {
	nvs_flash_init();

	Sys::delay(1000);
	XdrTester(10000);

	INFO("Starting Akka ");
	//    INFO(">>> %d %s", Wifi::Connected.id(), Wifi::Connected.label());

	Sys::init();
	Mailbox defaultMailbox("default", 2048);
	Mailbox mqttMailbox("mqtt", 2048);

	MessageDispatcher defaultDispatcher;
	MessageDispatcher mqttDispatcher;

	ActorSystem actorSystem(Sys::hostname(), defaultDispatcher, defaultMailbox);

	ActorRef sender = actorSystem.actorOf<Sender>("Sender");
	ActorRef wifi = actorSystem.actorOf<Wifi>("Wifi");
	ActorRef mqtt = actorSystem.actorOf<Mqtt>("mqtt", wifi,"tcp://limero.ddns.net:1883");
	ActorRef bridge = actorSystem.actorOf<Bridge>("bridge",mqtt);
	ActorRef system = actorSystem.actorOf<System>("System",mqtt);

	defaultDispatcher.attach(defaultMailbox);
	defaultDispatcher.unhandled(bridge.cell());


	defaultDispatcher.execute();

}

extern "C" void app_main() {
	xTaskCreate(&akkaMainTask, "akkaMainTask", 5000, NULL, tskIDLE_PRIORITY + 1,
	            NULL);
}
