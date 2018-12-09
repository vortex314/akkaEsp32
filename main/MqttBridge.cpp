
#include <MqttBridge.h>
#include <sys/types.h>
#include <unistd.h>
// volatile MQTTAsync_token deliveredtoken;
#define BZERO(x) ::memset(&x, sizeof(x), 0)
#define CONFIG_BROKER_URL "test.mosquitto.org"

static const char* TAG = "MQTT_EXAMPLE";

MqttBridge::MqttBridge(va_list args) { _address = va_arg(args, const char*); };
MqttBridge::~MqttBridge() {}

MsgClass MqttBridge::MQTT_PUBLISH_RCVD() {
    static MsgClass MQTT_PUBLISH_RCVD("MQTT_PUBLISH_RCVD");
    return MQTT_PUBLISH_RCVD;
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event) {
    ESP_LOGI(TAG, "[MQTT] Event..");

    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id =
            esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id =
            esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    }
    return ESP_OK;
}

static void mqtt_app_start(void) {
    ESP_LOGI(TAG, "[MQTT] Startup..");

    esp_mqtt_client_config_t mqtt_cfg;
    BZERO(mqtt_cfg);
    mqtt_cfg.uri = CONFIG_BROKER_URL;
    mqtt_cfg.event_handle = mqtt_event_handler;
    mqtt_cfg.user_context = (void*)0;

#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}

void MqttBridge::preStart() {
    //   context().mailbox(remoteMailbox);

    _clientId = self().path();
    _clientId += "#";
    char str[10];
    int pid = 12345;
    snprintf(str, sizeof(str), "%d", pid);
    _clientId += str;
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    mqtt_app_start();
    mqttConnect();
}

void MqttBridge::mqttConnect() {
    int rc;
    INFO(" connecting to %s", _address.c_str());
}

void MqttBridge::mqttDisconnect() { int rc; }

bool payloadToJsonArray(JsonArray& array, Cbor& payload) {
    Cbor::PackType pt;
    payload.offset(0);
    Erc rc;
    Str str(100);
    while (payload.hasData()) {
        rc = payload.peekToken(pt);
        if (rc != E_OK)
            break;
        switch (pt) {
        case Cbor::P_STRING: {
            payload.get(str);
            array.add(
                (char*)str.c_str()); // const char* is not copied into buffer
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
        .match(MsgClass::AnyClass(),
               [this](Envelope& msg) {
                   if (!(*msg.receiver == self())) {
                       INFO(" message received %s:%s:%s [%d] in %s",
                            msg.sender->path(), msg.receiver->path(),
                            msg.msgClass.label(), msg.message.length(),
                            context().self().path());
                       _jsonBuffer.clear();
                       JsonArray& array = _jsonBuffer.createArray();
                       array.add(msg.receiver->path());
                       array.add(msg.sender->path());
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
        .match(MQTT_PUBLISH_RCVD(),
               [this](Envelope& msg) {
                   Str topic(100);
                   Str message(1024);

                   if (msg.scanf("SS", &topic, &message) &&
                       handleMqttMessage(message.c_str())) {
                       INFO(" processed message %s", message.c_str());
                   } else {
                       WARN(" processing failed : %s ", message.c_str());
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

    ActorRef* rcv = ActorRef::lookup(Uid::hash(array.get<const char*>(0)));
    if (rcv == 0) {
        rcv = &ActorRef::NoSender();
        WARN(" local Actor : %s not found ", array.get<const char*>(0));
    }
    ActorRef* snd = ActorRef::lookup(Uid::hash(array.get<const char*>(1)));
    if (snd == 0) {
        snd = new ActorRef(Uid::hash(array.get<const char*>(1)), 0);
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
}

void MqttBridge::mqttSubscribe(const char* topic) {
    INFO("Subscribing to topic %s ", topic);
}
