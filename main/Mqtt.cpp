#include <Mqtt.h>
#include <Wifi.h>
#include <sys/types.h>
#include <unistd.h>
#include <Config.h>
// volatile MQTTAsync_token deliveredtoken;
#define BZERO(x) ::memset(&x, 0, sizeof(x))
#define CONFIG_BROKER_URL "mqtt://limero.ddns.net"

Mqtt::Mqtt(ActorRef& wifi, const char* address)
	: _wifi(wifi), _address(address) {
	_connected = false;
	config.setNameSpace("mqtt");
	config.get("url", _address, "tcp://limero.ddns.net:1883");
	_mqttClient = 0;
	_lwt_topic="src/";
	_lwt_topic.append(Sys::hostname()).append("/system/alive");
	_lwt_message="false";
}

Mqtt::~Mqtt() {
}

MsgClass Mqtt::PublishRcvd("mqttPublishRcvd");
MsgClass Mqtt::Connected("mqttConnected");
MsgClass Mqtt::Disconnected("mqttDisconnected");
MsgClass Mqtt::Publish("mqttPublish");
MsgClass Mqtt::Subscribe("mqttSubscribe");

void Mqtt::preStart() {
	DEBUG(" MQTT preStart");
	eb.subscribe(self(), MessageClassifier(_wifi, Wifi::Disconnected));
	eb.subscribe(self(), MessageClassifier(_wifi, Wifi::Connected));

	_clientId = self().path();
	char str[10];
	int pid = 12345;
	snprintf(str, sizeof(str), "#%d", pid);
	_clientId += str;

//	esp_log_level_set("*", ESP_LOG_VERBOSE);

	esp_mqtt_client_config_t mqtt_cfg;
	BZERO(mqtt_cfg);
	mqtt_cfg.uri = CONFIG_BROKER_URL;
	mqtt_cfg.event_handle = mqtt_event_handler;
	mqtt_cfg.client_id = Sys::hostname();
	mqtt_cfg.user_context = this;
	mqtt_cfg.buffer_size = 10240;
	mqtt_cfg.lwt_topic = _lwt_topic.c_str();
	mqtt_cfg.lwt_msg = _lwt_message.c_str();
	mqtt_cfg.lwt_qos=1;
	mqtt_cfg.lwt_msg_len=5;
	mqtt_cfg.keepalive=5;
	_mqttClient = esp_mqtt_client_init(&mqtt_cfg);

	timers().startPeriodicTimer("T1", Msg("reportTimer"), 1000);

}

Receive& Mqtt::createReceive() {
	return receiveBuilder()

	.match(Mqtt::Publish, [this](Msg& msg) {
		std::string topic;
		std::string message;
		while ( msg.getNext("topic",topic) && msg.getNext("message",message) ) {
			mqttPublish(topic.c_str(),message.c_str());
		}
	})

	.match(MsgClass::Properties(), [this](Msg& msg) {
		sender().tell(Msg(MsgClass::PropertiesReply())
		              ("clientId",_clientId)
		              ,self());
	})

	.match(MsgClass(H("reportTimer")), [this](Msg& msg) {
		mqttPublish(_lwt_topic.c_str(),"true");
	})

	.match(Wifi::Connected, [this](Msg& msg) {
		esp_mqtt_client_start(_mqttClient);
		INFO(" WIFI CONNECTED !!");
	})

	.match(Wifi::Disconnected, [this](Msg& msg) {
		INFO(" WIFI DISCONNECTED !!");
		esp_mqtt_client_stop(_mqttClient);
	})

	.build();
}

esp_err_t Mqtt::mqtt_event_handler(esp_mqtt_event_handle_t event) {
	Mqtt& me = *(Mqtt*) event->user_context;
	std::string topics;
	esp_mqtt_client_handle_t client = event->client;
	int msg_id;

	switch (event->event_id) {
		case MQTT_EVENT_CONNECTED: {
				me._connected = true;
				INFO("MQTT_EVENT_CONNECTED");
				msg_id =
				    esp_mqtt_client_publish(me._mqttClient, "src/limero/systems", me
				                            .context().system().label(), 0, 1, 0);
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
//			INFO("MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_DATA: {
				bool busy = false;
				if (!busy) {
					busy = true;
					static std::string topic;
					static std::string data;
					bool ready = true;
					if (event->data_len != event->total_data_len) {
						if (event->current_data_offset == 0) {
							topic = std::string(event->topic, event->topic_len);
							data = std::string(event->data, event->data_len);
							ready = false;
						} else {
							data.append(event->data, event->data_len);
							if (data.length() != event->total_data_len) {
								ready = false;
							}
						}
					} else {
						topic = std::string(event->topic, event->topic_len);
						data = std::string(event->data, event->data_len);
					}
					if (ready) {
						INFO("RXD : %s = %s", topic.c_str(), data.c_str());
						Msg pub(PublishRcvd);
						pub("topic", topic)("message", data).src(me.self().id());
						eb.publish(pub);
					}
					busy = false;
				} else {
					WARN(" sorry ! MQTT reception busy ");
				}
				break;

			}
		case MQTT_EVENT_ERROR:
			WARN("MQTT_EVENT_ERROR");
			break;
	}
	return ESP_OK;
}

typedef enum {
	PING=0,PUBLISH,PUBACK,SUBSCRIBE,SUBACK
} CMD;

void Mqtt::mqttPublish(const char* topic, const char* message) {
	if (_connected == false) return;
	INFO("PUB : %s = %s", topic, message);
	DynamicJsonDocument doc(2038);

	doc.add("0000");
	doc.add(PUBLISH);
	doc.add(topic);
	doc.add(message);
	doc.add(0);
	doc.add(0);
	std::string line;
	serializeJson(doc,line);
	printf("%s\r\n",line.c_str());
	int id = esp_mqtt_client_publish(_mqttClient, topic, message, 0, 1, 0);
	if (id < 0)
		WARN("esp_mqtt_client_publish() failed.");
}

void Mqtt::mqttSubscribe(const char* topic) {
	if (_connected == false) return;
	INFO("Subscribing to topic %s ", topic);
	int id = esp_mqtt_client_subscribe(_mqttClient, topic, 0);
	if (id < 0)
		WARN("esp_mqtt_client_subscribe() failed.");
	DynamicJsonDocument doc(2038);

	doc.add("0000");
	doc.add(SUBSCRIBE);
	doc.add(topic);
	std::string line;
	serializeJson(doc,line);
	printf("%s\r\n",line.c_str());
}
