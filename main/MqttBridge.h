extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
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

class MqttBridge : public Actor
{

    bool _connected;
    StaticJsonBuffer<2000> _jsonBuffer;
    string _clientId;
    string _address;


public:
    static MsgClass MQTT_PUBLISH_RCVD();
    MqttBridge(va_list args);
    ~MqttBridge();
    void preStart();
    Receive& createReceive();


    void mqttPublish(const char* topic, const char* message);
    void mqttSubscribe(const char* topic);
    void mqttConnect();
    void mqttDisconnect();

    bool handleMqttMessage(const char* message);
};
