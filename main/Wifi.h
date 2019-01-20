#ifndef WIFI_H
#define WIFI_H

#include <Akka.h>

#include "esp_wifi.h"
#include "esp_wifi_types.h"

#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "freertos/event_groups.h"


class Wifi : public Actor {
		std::string _ssid;
		std::string _pswd;
		std::string _prefix;
		std::string _ipAddress;
		int _rssi;

	public:
		static MsgClass Connected;
		static MsgClass Disconnected;
		Wifi( );
		~Wifi();
		void preStart();
		Receive& createReceive();

		static esp_err_t wifi_event_handler(void* ctx, system_event_t* event);
		void init();
		void scanDoneHandler();
		void connectToAP(const char* AP);
		void startScan();
		void wifiInit();
		const char* getSSID();
};

#endif // WIFI_H
