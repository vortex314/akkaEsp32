
#include <MqttBridge.h>
#include <sys/types.h>
#include <unistd.h>
// volatile MQTTAsync_token deliveredtoken;
#define BZERO(x) ::memset(&x, 0, sizeof(x))
#define CONFIG_BROKER_URL "mqtt://limero.ddns.net"

MqttBridge::MqttBridge(va_list args) {
    _address = va_arg(args, const char*);
    _receiving = false;
};
MqttBridge::~MqttBridge() {}

MsgClass MqttBridge::MqttPublishRcvd("MqttPublishRcvd");

esp_err_t MqttBridge::mqtt_event_handler(esp_mqtt_event_handle_t event) {
    MqttBridge& me = *(MqttBridge*)event->user_context;
    string topics;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        me._connected = true;
        INFO("MQTT_EVENT_CONNECTED");
        msg_id =
            esp_mqtt_client_publish(me._mqttClient, "src/limero/systems",
                                    me.context().system().label(), 0, 1, 0);
        topics = "dst/";
        topics += me.context().system().label();
        topics += "/#";
        me.mqttSubscribe(topics.c_str());

        break;
    case MQTT_EVENT_DISCONNECTED:
        me._connected = false;

        INFO("MQTT_EVENT_DISCONNECTED");
        break;

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
        INFO("MQTT_EVENT_DATA");
        std::string topic(event->topic, event->topic_len);
        std::string data(event->data, event->data_len);
        me.self().tell(me.self(), MqttPublishRcvd, "++", topic, data);
        break;
    }
    case MQTT_EVENT_ERROR:
        INFO("MQTT_EVENT_ERROR");
        break;
    }
    return ESP_OK;
}

void MqttBridge::preStart() {
    INFO(" MQTT preStart");
    timers().startPeriodicTimer("PUB_TIMER", TimerExpired, 5000);
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

    esp_mqtt_client_start(_mqttClient);
}

bool payloadToJsonArray(JsonArray& array, Cbor& payload) {
    Cbor::PackType pt;
    payload.offset(0);
    Erc rc;
    string str;
    while (payload.hasData()) {
        rc = payload.peekToken(pt);
        if (rc != E_OK)
            break;
        switch (pt) {
        case Cbor::P_STRING: {
            payload.get(str);
            array.add((char*)str.c_str());
            // const char* is not copied into buffer
            break;
        }
        case Cbor::P_PINT: {
            uint64_t u64;
            payload.get(u64);
            array.add(u64);
            break;
        }
        case Cbor::P_DOUBLE: {
            double d;
            payload.get(d);
            array.add(d);
            break;
        }
        default: { payload.skipToken(); }
        }
    };
    return true;
}

Receive& MqttBridge::createReceive() {
    return receiveBuilder()
        .match(AnyClass,
               [this](Envelope& msg) {
                   if (!(*msg.receiver == self())) {
                       INFO(" message received %s:%s:%s [%d] ",
                            msg.sender->path(), msg.receiver->path(),
                            msg.msgClass.label(), msg.message.length());
                       _jsonBuffer.clear();
                       std::string s;
                       JsonArray& array = _jsonBuffer.createArray();
                       s = context().system().label();
                       s += "/";
                       s += msg.sender->path();
                       array.add(msg.receiver->path());
                       array.add(s.c_str());
                       array.add(msg.msgClass.label());
                       array.add(msg.id);
                       payloadToJsonArray(array, msg.message);

                       std::string topic = "dst/";
                       topic += msg.receiver->path();

                       std::string message;
                       array.printTo(message);

                       mqttPublish(topic.c_str(), message.c_str());
                   }
               })
        .match(MqttPublishRcvd,
               [this](Envelope& msg) {
                   string topic;
                   string message;

                   if (msg.scanf("++", &topic, &message) &&
                       handleMqttMessage(message.c_str())) {
                       INFO(" processed message %s", message.c_str());
                   } else {
                       WARN(" processing failed : %s ", message.c_str());
                   }
               })
        .match(TimerExpired,
               [this](Envelope& msg) {
                   string topic = "src/";
                   topic += context().system().label();
                   topic += "/system/alive";
                   if (_connected) {
                       int id = esp_mqtt_client_publish(
                           _mqttClient, topic.c_str(), "true", 0, 0, 0);
                       if (id < 0)
                           WARN("esp_mqtt_client_publish() failed.");
                   }
               })
        .build();
}

bool MqttBridge::handleMqttMessage(const char* message) {
    //    Envelope envelope(1024);
    _jsonBuffer.clear();
    JsonArray& array = _jsonBuffer.parse(message);
    if (array == JsonArray::invalid())
        return false;
    if (array.size() < 4)
        return false;
    if (!array.is<char*>(0) || !array.is<char*>(1) || !array.is<char*>(2))
        return false;

    string rcvs = array.get<const char*>(0);

    ActorRef* rcv = ActorRef::lookup(Uid(rcvs.c_str()));
    if (rcv == 0) {
        rcv = &context().system().actorFor(rcvs.c_str());
        ASSERT(rcv != 0);
        WARN(" local Actor : %s not found ", rcvs.c_str());
    }
    ActorRef* snd = ActorRef::lookup(Uid::hash(array.get<const char*>(1)));
    if (snd == 0) {
        snd = &context().system().actorFor(array.get<const char*>(1));
        ASSERT(snd != 0);
    }
    MsgClass cls(array.get<const char*>(2));
    uint16_t id = array.get<int>(3);

    for (uint32_t i = 4; i < array.size(); i++) {
        // TODO add to cbor buffer
        if (array.is<char*>(i)) {
            txdEnvelope().message.add(array.get<const char*>(i));
        } else if (array.is<int>(i)) {
            txdEnvelope().message.add(array.get<int>(i));
        } else if (array.is<double>(i)) {
            txdEnvelope().message.add(array.get<double>(i));
        } else if (array.is<bool>(i)) {
            txdEnvelope().message.add(array.get<bool>(i));
        } else {
            WARN(" unhandled data type in MQTT message ");
        }
    }
    txdEnvelope().header(*rcv, *snd, cls, id);
    rcv->tell(*snd, txdEnvelope());
    return true;
}
/*
int MqttBridge::onMessageArrived(void* context, char* topicName, int topicLen)
{
    MqttBridge* me = (MqttBridge*)context;
    Str topic((uint8_t*)topicName, topicLen);
    Str msg((uint8_t*)message->payload, message->payloadlen);
    INFO(" MQTT RXD : %s = %s ", topicName, message->payload);
    //   me->self().tell(me-self(),MQTT_PUBLISH_RCVD,"SS",&topic,&msg);
    Envelope envelope(1024);
    envelope.header(me->self(), me->self(), MQTT_PUBLISH_RCVD());
    envelope.message.addf("SS", &topic, &msg);
    me->self().tell(me->self(), envelope);
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}*/

void MqttBridge::mqttPublish(const char* topic, const char* message) {
    INFO(" MQTT TXD : %s = %s", topic, message);
    int id = esp_mqtt_client_publish(_mqttClient, topic, message, 0, 0, 0);
    if (id < 0)
        WARN("esp_mqtt_client_publish() failed.");
}

void MqttBridge::mqttSubscribe(const char* topic) {
    INFO("Subscribing to topic %s ", topic);
    int id = esp_mqtt_client_subscribe(_mqttClient, topic, 0);
    if (id < 0)
        WARN("esp_mqtt_client_publish() failed.");
}
