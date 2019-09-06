#include "Wifi.h"
#include <Config.h>


#define SSID_DEFAULT "Merckx"

#define STRINGIFY(X) #X
#define S(X) STRINGIFY(X)

#define CHECK(x)                                                               \
    do {                                                                       \
        esp_err_t __err_rc = (x);                                              \
        if (__err_rc != ESP_OK) {                                              \
            WARN("%s = %d ", #x, __err_rc);        \
        }                                                                      \
    } while (0);

MsgClass Wifi::Connected("Connected");
MsgClass Wifi::Disconnected("Disconnected");

Wifi::Wifi()
{
    _rssi = 0;
    JsonObject myConfig = config.root()["wifi"];
#ifdef WIFI_SSID
    _prefix = S(WIFI_SSID);
    myConfig["prefix"]=S(WIFI_SSID);
#else
    _prefix = myConfig["prefix"] | SSID_DEFAULT;
#endif
#ifdef WIFI_PASS
    _pswd = S(WIFI_PASS);
    myConfig["password"]=S(WIFI_PASS);
#else
    _pswd = myConfig["password"] | "";
#endif
}

Wifi::~Wifi()
{
}

void Wifi::preStart()
{
    wifiInit();
    esp_base_mac_addr_get(_mac);
}

Receive& Wifi::createReceive()
{
    return receiveBuilder()

    .match(MsgClass::Properties(), [this](Msg& msg) {
        std::string macAddress;

        string_format(macAddress,"%02X:%02X:%02X:%02X:%02X:%02X",_mac[0],_mac[1],_mac[2],_mac[3],_mac[4],_mac[5]);
        sender().tell(replyBuilder(msg)
                      ("rssi",_rssi)
                      ("ssid",_ssid)
                      ("prefix",_prefix)
                      ("ip",_ipAddress)
                      ("mac",macAddress)
                      ,self());
    })

    .build();
}

//#define BZERO(x) ::memset(&x, sizeof(x), 0)
static const char* TAG = "MQTT_EXAMPLE";

static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;

//const char* getIpAddress() { return my_ip_address; }

esp_err_t Wifi::wifi_event_handler(void* ctx, system_event_t* event)
{
    Wifi& wifi = *(Wifi*) ctx;
    switch (event->event_id) {

    case SYSTEM_EVENT_SCAN_DONE: {
        INFO("SYSTEM_EVENT_SCAN_DONE");
        wifi.scanDoneHandler();
        break;
    }
    case SYSTEM_EVENT_STA_STOP: {
        INFO("SYSTEM_EVENT_STA_STOP");
        esp_wifi_start();
//				wifi.wifiInit();
        break;
    }
    case SYSTEM_EVENT_STA_START: {
        INFO("SYSTEM_EVENT_STA_START");
        wifi.startScan();
        break;
    }
    case SYSTEM_EVENT_STA_GOT_IP: {
        INFO("SYSTEM_EVENT_STA_GOT_IP");
        system_event_sta_got_ip_t* got_ip = &event->event_info.got_ip;
        char my_ip_address[20];
        ip4addr_ntoa_r(&got_ip->ip_info.ip, my_ip_address, 20);
        wifi._ipAddress = my_ip_address;

        eb.publish(Msg(Wifi::Connected).src(wifi.self().id()));
        break;
    }
    case SYSTEM_EVENT_STA_CONNECTED: {
        INFO("SYSTEM_EVENT_STA_CONNECTED");
        break;
    }
    case SYSTEM_EVENT_STA_DISCONNECTED: {
        INFO("SYSTEM_EVENT_STA_DISCONNECTED");
        eb.publish(Msg(Wifi::Disconnected).src(wifi.self().id()));
        esp_wifi_connect();
        break;
    }

    default:
        INFO("unknown wifi event %d ", event->event_id);
        break;
    }

    return ESP_OK;
}

void Wifi::connectToAP(const char* ssid)
{
    INFO(" connecting to SSID : %s", ssid);
    wifi_config_t wifi_config;
    _ssid = ssid;
    memset(&wifi_config, 0, sizeof(wifi_config)); // needed !!
    strncpy((char*) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid)
            - 1);
    strncpy((char*) wifi_config.sta.password, S(WIFI_PASS), sizeof(wifi_config.sta
            .password) - 1);
    CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    esp_wifi_connect();
}

void Wifi::scanDoneHandler()
{
    uint16_t sta_number;
    esp_wifi_scan_get_ap_num(&sta_number);
    INFO(" found %d AP's ", sta_number);
    if (sta_number == 0) {
        WARN(" no AP found , restarting scan.");
        startScan();
        return;
    }
    wifi_ap_record_t apRecords[sta_number];
    esp_wifi_scan_get_ap_records(&sta_number, apRecords);
    int strongestAP = -1;
    _rssi = -200;
    for (uint32_t i = 0; i < sta_number; i++) {
        INFO(" %s : %d ", apRecords[i].ssid, apRecords[i].rssi);
        std::basic_string<char> ssid = (const char*) apRecords[i].ssid;
        if ((apRecords[i].rssi > _rssi) && (ssid.find(_prefix) == 0)) {
            strongestAP = i;
            _rssi = apRecords[i].rssi;
        }
    }
    if (strongestAP == -1) {
        WARN(" no AP found matching pattern '%s', restarting scan.", _prefix.c_str());
        startScan();
        return;
    }
    connectToAP((const char*) apRecords[strongestAP].ssid);
}

void Wifi::startScan()
{
    INFO(" starting WiFi scan.");
    wifi_scan_config_t scanConfig = {
        NULL, NULL, 0, false, WIFI_SCAN_TYPE_ACTIVE, { 0, 0 }
    };
    CHECK(esp_wifi_scan_start(&scanConfig, false));
}

void Wifi::wifiInit()
{
    tcpip_adapter_init();
    CHECK(esp_event_loop_init(wifi_event_handler, this));
    wifi_init_config_t wifiInitializationConfig = WIFI_INIT_CONFIG_DEFAULT()
            ;
    CHECK(esp_wifi_init(&wifiInitializationConfig));
    CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    CHECK(esp_wifi_set_promiscuous(false));
    CHECK(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B));
    CHECK(esp_wifi_start());
}
