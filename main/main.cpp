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
#include <Publisher.cpp>
#include <ConfigActor.cpp>
#include <Config.h>
#include <UltraSonic.h>
#include <Compass.h>

using namespace std;

Log logger(256);
ActorMsgBus eb;

#define BZERO(x) ::memset(&x, 0, sizeof(x))

extern void XdrTester(uint32_t max);

extern "C" void app_main() {
	INFO(" ActorCell : %d , ActorRef: %d , Uid : %d Mailbox: %d ",sizeof(ActorCell),sizeof(ActorRef),sizeof(Uid),sizeof(Mailbox));
	Sys::init();
	nvs_flash_init();
	INFO("Starting Akka ");
	Sys::delay(3000); // let wifi start
	std::string output;
	config.load();
	config.printPretty(output);
	printf(" config : \n %s \n",output.c_str());

	printf("Starting Akka on %s heap : %d ", Sys::getProcessor(), Sys::getFreeHeap());
	static Mailbox defaultMailbox("default", 100);
	static MessageDispatcher defaultDispatcher(3,3000,tskIDLE_PRIORITY+1);
	defaultDispatcher.attach(defaultMailbox);
	static ActorSystem actorSystem(Sys::hostname(), defaultDispatcher, defaultMailbox);

	actorSystem.actorOf<Sender>("sender");
	ActorRef wifi = actorSystem.actorOf<Wifi>("wifi");
	ActorRef mqtt = actorSystem.actorOf<Mqtt>("mqtt", wifi,"tcp://limero.ddns.net:1883");
	ActorRef bridge = actorSystem.actorOf<Bridge>("bridge",mqtt);
	defaultDispatcher.unhandled(bridge.cell());

	actorSystem.actorOf<System>("system",mqtt);
	actorSystem.actorOf<ConfigActor>("config");
	ActorRef publisher = actorSystem.actorOf<Publisher>("publisher",mqtt);
	ActorRef us = actorSystem.actorOf<UltraSonic>("ultraSonic",publisher);
//	ActorRef compass = actorSystem.actorOf<Compass>("compass",publisher);
	config.save();
	defaultDispatcher.start();
}
