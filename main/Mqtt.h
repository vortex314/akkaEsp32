#ifndef _MQTT_H_
#define _MQTT_H_
extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "mqtt_client.h"
}
#include <Akka.h>
#include <ArduinoJson.h>

// #define ADDRESS "tcp://test.mosquitto.org:1883"
//#define CLIENTID "microAkka"
//#define TOPIC "dst/steer/system"
//#define PAYLOAD "[\"pclat/aliveChecker\",1234,23,\"hello\"]"
#define QOS 0
#define TIMEOUT 10000L

class Mqtt : public Actor {

		bool _connected;
		StaticJsonDocument<3000> _jsonBuffer;
		std::string _clientId;
		std::string _address;
		esp_mqtt_client_handle_t _mqttClient;
		ActorRef& _wifi;
		std::string _lwt_topic;
		std::string _lwt_message;

	public:
		static MsgClass PublishRcvd;
		static MsgClass Connected;
		static MsgClass Disconnected;
		static MsgClass Publish;
		static MsgClass Subscribe;
		Mqtt(ActorRef&,const char*);
		~Mqtt();
		void preStart();
		Receive& createReceive();

		void mqttPublish(const char* topic, const char* message);
		void mqttSubscribe(const char* topic);
		void mqttConnect();
		void mqttDisconnect();

		bool handleMqttMessage(const char* message);

		static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);
};

#endif
