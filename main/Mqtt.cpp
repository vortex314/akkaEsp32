
#include <Mqtt.h>
#include <Wifi.h>
#include <sys/types.h>
#include <unistd.h>
#include <Config.h>
// volatile MQTTAsync_token deliveredtoken;
#define BZERO(x) ::memset(&x, 0, sizeof(x))
#define CONFIG_BROKER_URL "mqtt://limero.ddns.net"

Mqtt::Mqtt(va_list args) {
	_wifi = va_arg(args,ActorRef);
	_address = va_arg(args, const char*);
	_connected = false;
	config.setNameSpace("mqtt");
	config.get("url",_address,"tcp://limero.ddns.net:1883");
};
Mqtt::~Mqtt() {}

MsgClass Mqtt::PublishRcvd("Mqtt/Publish");
MsgClass Mqtt::Connected("Mqtt/Connected");
MsgClass Mqtt::Disconnected("Mqtt/Disconnected");
MsgClass Mqtt::Publish("Mqtt/Publish");
MsgClass Mqtt::Subscribe("Mqtt/Subscribe");

void Mqtt::preStart() {
	INFO(" MQTT preStart");
	timers().startPeriodicTimer("PUB_TIMER", Msg("aliveTimer"), 1000);
	eb.subscribe(self(), MessageClassifier(_wifi, Wifi::Disconnected));
	eb.subscribe(self(), MessageClassifier(_wifi, Wifi::Connected));
	_clientId = self().path();
	_clientId += "#";
	char str[10];
	int pid = 12345;
	snprintf(str, sizeof(str), "%d", pid);
	_clientId += str;

	esp_log_level_set("*", ESP_LOG_VERBOSE);

	esp_mqtt_client_config_t mqtt_cfg;
	BZERO(mqtt_cfg);
	mqtt_cfg.uri = CONFIG_BROKER_URL;
	mqtt_cfg.event_handle = mqtt_event_handler;
	mqtt_cfg.client_id = Sys::hostname();
	mqtt_cfg.user_context = this;
	_mqttClient = esp_mqtt_client_init(&mqtt_cfg);

	//   esp_mqtt_client_start(_mqttClient);
}


Receive& Mqtt::createReceive() {
	return receiveBuilder()
	.match(MsgClass("aliveTimer"),	[this](Envelope& msg) {
		string topic = "src/";
		topic += context().system().label();
		topic += "/system/alive";
		if (_connected) {
			mqttPublish(topic.c_str(), "true");
		}
	}).match(Mqtt::Publish,	[this](Envelope& msg) {
		std::string topic;
		std::string message;
		if ( msg.get("topic",topic)==0 && msg.get("message",message)==0 ) {
			mqttPublish(topic.c_str(),message.c_str());
		}
	})
	.match(Properties(),[this](Envelope& msg) {
		sender().tell(Msg(PropertiesReply())
		              ("clientId",_clientId)
		              ,self());
	})
	.match(Wifi::Connected,	[this](Envelope& msg) {esp_mqtt_client_start(_mqttClient); INFO(" WIFI CONNECTED !!");	})
	.match(Wifi::Disconnected,[this](Envelope& msg) {INFO(" WIFI DISCONNECTED !!"); esp_mqtt_client_stop(_mqttClient);	})
	.build();
}

esp_err_t Mqtt::mqtt_event_handler(esp_mqtt_event_handle_t event) {
	Mqtt& me = *(Mqtt*)event->user_context;
	string topics;
	esp_mqtt_client_handle_t client = event->client;
	int msg_id;

	switch (event->event_id) {
		case MQTT_EVENT_CONNECTED: {
				me._connected = true;
				INFO("MQTT_EVENT_CONNECTED");
				msg_id =
				    esp_mqtt_client_publish(me._mqttClient, "src/limero/systems",
				                            me.context().system().label(), 0, 1, 0);
				topics = "dst/";
				topics += me.context().system().label();
				me.mqttSubscribe(topics.c_str());
				topics += "/#";
				me.mqttSubscribe(topics.c_str());
				eb.publish(Msg(Mqtt::Connected).src(me.self().id()));
				break;
			}
		case MQTT_EVENT_DISCONNECTED: {
				me._connected = false;
				eb.publish(Msg(Mqtt::Disconnected).src(me.self().id()));
				INFO("MQTT_EVENT_DISCONNECTED");
				break;
			}
		case MQTT_EVENT_SUBSCRIBED:
			INFO("MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_UNSUBSCRIBED:
			INFO("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_PUBLISHED:
			INFO("MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_DATA: {
				bool busy=false;
				if ( !busy ) {
					busy=true;
					INFO("MQTT_EVENT_DATA");
					std::string topic(event->topic, event->topic_len);
					std::string data(event->data, event->data_len);
					Msg  pub(PublishRcvd);
					pub("topic", topic)("message", data).src(me.self().id());
					INFO("%s",pub.toString().c_str());
					eb.publish(pub);
					busy=false;
				} else {
					WARN(" sorry ! MQTT reception busy ");
				}
				break;

			}
		case MQTT_EVENT_ERROR:
			INFO("MQTT_EVENT_ERROR");
			break;
	}
	return ESP_OK;
}

void Mqtt::mqttPublish(const char* topic, const char* message) {
	INFO(" MQTT TXD : %s = %s", topic, message);
	int id = esp_mqtt_client_publish(_mqttClient, topic, message, 0, 0, 0);
	if (id < 0)
		WARN("esp_mqtt_client_publish() failed.");
}

void Mqtt::mqttSubscribe(const char* topic) {
	INFO("Subscribing to topic %s ", topic);
	int id = esp_mqtt_client_subscribe(_mqttClient, topic, 0);
	if (id < 0)
		WARN("esp_mqtt_client_publish() failed.");
}
