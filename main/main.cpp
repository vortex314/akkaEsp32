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
#include <Sender.cpp>
#include <Uid.cpp>
#include <mqtt_client.h>

void Semaphore::wait(){};
void Semaphore::release(){};
Semaphore& Semaphore::create()
{
    return *new Semaphore();
}

#define CONFIG_BROKER_URL "test.mosquitto.org"

using namespace std;

Log logger(256);
/*
string string_format(string& str, const char* fmt, ...)
{
    int size = strlen(fmt) * 2 + 50; // Use a rubric appropriate for your code
    va_list ap;
    while(1) { // Maximum two passes on a POSIX system...
        str.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf((char*)str.data(), size, fmt, ap);
        va_end(ap);
        if(n > -1 && n < size) { // Everything worked
            str.resize(n);
            return str;
        }
        if(n > -1)        // Needed size returned
            size = n + 1; // For null char
        else
            size *= 2; // Guess at a larger size (OS specific)
    }
    return str;
}
*/
void setHostname()
{
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

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    ESP_LOGI(TAG, "[MQTT] Event..");

    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch(event->event_id) {
    case MQTT_EVENT_CONNECTED:
	ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
	msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
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
	msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
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

static esp_err_t wifi_event_handler(void* ctx, system_event_t* event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
	esp_wifi_connect();
	break;
    case SYSTEM_EVENT_STA_GOT_IP:
	xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);

	break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
	esp_wifi_connect();
	xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
	break;
    default:
	break;
    }
    return ESP_OK;
}

#define BZERO(x) ::memset(&x, sizeof(x), 0)

static void wifi_init(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config;
    BZERO(wifi_config);
    strcpy((char*)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, WIFI_PASS);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_LOGI(TAG, "start the WIFI SSID:[%s][%s]", WIFI_SSID, WIFI_PASS);
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Waiting for wifi");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}

static void mqtt_app_start(void)
{
    ESP_LOGI(TAG, "[MQTT] Startup..");

    esp_mqtt_client_config_t mqtt_cfg;
    BZERO(mqtt_cfg);
    mqtt_cfg.uri = CONFIG_BROKER_URL;
    mqtt_cfg.event_handle = mqtt_event_handler;
    mqtt_cfg.user_context = (void*)0;

#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if(strcmp(mqtt_cfg.uri, "FROM_STDIN") == 0) {
	int count = 0;
	printf("Please enter url of mqtt broker\n");
	while(count < 128) {
	    int c = fgetc(stdin);
	    if(c == '\n') {
		line[count] = '\0';
		break;
	    } else if(c > 0 && c < 127) {
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

extern "C" void app_main()
{
    Sys::init();
    setHostname();
    Mailbox defaultMailbox = *new Mailbox("default", 20000, 1000);
    Mailbox remoteMailbox = *new Mailbox("remote", 20000, 1000);
    MessageDispatcher& defaultDispatcher = *new MessageDispatcher();
    ActorSystem actorSystem(Sys::hostname(), defaultDispatcher, defaultMailbox);

    ActorRef sender = actorSystem.actorOf<Sender>("Sender");
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    nvs_flash_init();
    wifi_init();
    mqtt_app_start();
}

extern "C" void app_main2()
{
    setHostname();
    INFO(" starting ...");

    esp_mqtt_client_config_t mqtt_cfg;
    mqtt_cfg.uri = "mqtt://iot.eclipse.org";
    mqtt_cfg.event_handle = mqtt_event_handler;
    mqtt_cfg.user_context = (void*)0;
};
