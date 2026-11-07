#ifndef SENSOR_DHT_H
#define SENSOR_DHT_H

#include "dht.h"
#include "esp_log.h"

#define DHT_PIN GPIO_NUM_4

static float current_temp = 0.0f;
static float current_hum = 0.0f;
static const char *TAG_DHT = "DHT_SENSOR";

void dht_sensor_task(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(2000)); // รอระบบพร้อม
    
    while (1) {
        float t = 0, h = 0;
        // อ่านค่าจากเซนเซอร์ DHT11 ที่ขา GPIO 4
        esp_err_t result = dht_read_float_data(DHT11, DHT_PIN, &h, &t);
        
        if (result == ESP_OK) {
            current_temp = t;
            current_hum = h;
            // ESP_LOGI(TAG_DHT, "Sensor OK -> Temp: %.1f C, Hum: %.1f %%", current_temp, current_hum);
        } else {
            // ESP_LOGW(TAG_DHT, "Sensor Read Failed! Check wiring on GPIO 4");
        }
        
        vTaskDelay(pdMS_TO_TICKS(3000)); // อ่านค่าทุกๆ 3 วินาที
    }
}
#endif