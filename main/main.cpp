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
#include <MqttSerial.h>
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

#ifndef WIFI_SSID
#error "WIFI_SSID not found "
#endif

#ifndef WIFI_PASS
#error "WIFI_PASS not found "
#endif


#define CONTROLLER  "{\"uext\":[\"controller\"], \
		\"controller\":{\"class\":\"Controller\"}, \
		\"system\":{\"hostname\":\"remote\"}, \
		\"mqtt\":{\"host\":\"limero.local\",\"port\":1883}, \
\"wifi\":{\"ssid\":\""##WIFI_SSID##"\",\"password\":\""##WIFI_PASS##"\"}}"

#define MOTOR "{\"uext\":[\"motor\"],\"motor\":{\"class\":\"MotorSpeed\"}, \
		\"system\":{\"hostname\":\"drive\"}, \
		\"mqtt\":{\"host\":\"limero.local\",\"port\":1883}, \
\"wifi\":{\"ssid\":\""## WIFI_SSID ## "\",\"password\":\"" ## WIFI_PASS ## "\"}}"

#define SERVO "{\"uext\":[\"steer\"],\"steer\":{\"class\":\"MotorServo\"},\"system\":{\"hostname\":\"drive\"},\"mqtt\":{\"host\":\"limero.local\",\"port\":1883},\"wifi\":{\"ssid\":\"Merckx\",\"password\":\"LievenMarletteEwoutRonald\"}}"

#define SERVO_MOTOR "{\"uext\":[\"steer\",\"motor\"], \
        \"motor\":{\"class\":\"MotorSpeed\"}, \
        \"steer\":{\"class\":\"MotorServo\"}, \
        \"system\":{\"hostname\":\"drive\"}, \
        \"mqtt\":{\"host\":\"pi2.local\"}, \
\"wifi\":{\"ssid\":\"" ## WIFI_SSID ## "\",\"password\":\"" ## WIFI_PASS ## "\"}}"


#define GENERIC         "{\"uext\":[],\"mqtt\":{\"host\":\"limero.local\",\"port\":1883}, \
\"wifi\":{\"ssid\":\"" ## WIFI_SSID ## "\",\"password\":\"" ## WIFI_PASS ## "\"}}"

#define DWM1000_TAG "{\"uext\":[\"dwm1000Tag\"],\"dwm1000Tag\":{\"class\":\"DWM1000_Tag\"},\"system\":{\"hostname\":\"tag\"},\"mqtt\":{\"host\":\"limero.local\",\"port\":1883}, \
\"wifi\":{\"ssid\":\"" ## WIFI_SSID ## "\",\"password\":\"" ## WIFI_PASS ## "\"}}"
#define GPS_US     "{\"uext\":[\"gps\",\"us\"],\"gps\":{\"class\":\"NEO6M\"},\"us\":{\"class\":\"UltraSonic\"},\"mqtt\":{\"host\":\"limero.local\",\"port\":1883},\"wifi\":{\"ssid\":\"Merckx\",\"password\":\"LievenMarletteEwoutRonald\"}}"
#define STM32_PROGRAMMER  "{\"uext\":[\"programmer\"],\"programmer\":{\"class\":\"Programmer\"},\"mqtt\":{\"host\":\"limero.local\",\"port\":1883},\"wifi\":{\"ssid\":\"Merckx\",\"password\":\"LievenMarletteEwoutRonald\"}}"
#define COMPASS_US 	    "{\"uext\":[\"compass\",\"us\"],\"compass\":{\"class\":\"DigitalCompass\"},\"us\":{\"class\":\"UltraSonic\"},\"mqtt\":{\"host\":\"limero.local\",\"port\":1883},\"wifi\":{\"ssid\":\"Merckx\",\"password\":\"LievenMarletteEwoutRonald\"}}"
#define TRIAC 	    "{\"uext\":[\"triac\"],\"triac\":{\"class\":\"Triac\"},\"us\":{\"class\":\"UltraSonic\"},\"mqtt\":{\"host\":\"limero.local\",\"port\":1883},\"wifi\":{\"ssid\":\"Merckx\",\"password\":\"LievenMarletteEwoutRonald\"}}"


#define CONFIGURATION  CONTROLLER
#define MQTT_TCP


Log logger(256);
ActorMsgBus eb;

#define BZERO(x) ::memset(&x, 0, sizeof(x))

extern "C" void app_main() {
	esp_log_level_set("*", ESP_LOG_WARN);
	Sys::init();
	nvs_flash_init();
	INFO("Starting Akka on %s heap : %d ", Sys::getProcessor(),Sys::getFreeHeap());
#ifdef CONFIGURATION
	config.load(CONFIGURATION);
#else
	config.load();
#endif
	std::string hostname;
	config.setNameSpace("system");
	config.get("hostname",hostname,Sys::hostname());
	Sys::setHostname(hostname.c_str());

	static MessageDispatcher defaultDispatcher(4, 6000, tskIDLE_PRIORITY + 1);
	static ActorSystem actorSystem(Sys::hostname(), defaultDispatcher);

#ifdef MQTT_SERIAL
	ActorRef& mqtt = actorSystem.actorOf<MqttSerial>("mqttSerial");
#endif
#ifdef MQTT_TCP
	ActorRef& wifi = actorSystem.actorOf<Wifi>("wifi");
	ActorRef& mqtt = actorSystem.actorOf<Mqtt>("mqtt", wifi,cfg["mqtt"]);
#endif
	ActorRef& bridge = actorSystem.actorOf<Bridge>("bridge", mqtt);
	actorSystem.actorOf<System>("system", mqtt);
	actorSystem.actorOf<ConfigActor>("config");

	JsonObject cfg = config.root();
	JsonArray uexts = cfg["uext"].as<JsonArray>();
	int idx = 1; // starts at 1
	for (const char* name : uexts) {
		const char* peripheral = cfg[name]["class"] | "";
		if (strlen(peripheral) > 0) {
			switch (H(peripheral)) {
				case H("Controller"): {
						actorSystem.actorOf<Controller>(name,bridge);
						break;
					}
				case H("Programmer"): {
						actorSystem.actorOf<Programmer>(name, new Connector(idx),
						                                bridge);
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
