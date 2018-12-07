#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include <stdio.h>

#include <Log.h>

// esp_err_t event_handler(void *ctx, system_event_t *event) { return ESP_OK; }

#include "driver/spi_master.h"
#include "esp_system.h"

EventBus eb(1024);
Log logger(256);

extern "C" void app_main()
{
    //       config.clear();
}
