#ifndef DHT_H
#define DHT_H

#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef enum {
    DHT11 = 0,
    DHT22
} dht_sensor_type_t;

esp_err_t dht_read_float_data(dht_sensor_type_t sensor_type, gpio_num_t pin, float *humidity, float *temperature) {
    int dat[5] = {0, };
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    vTaskDelay(pdMS_TO_TICKS(20)); // Start signal > 18ms
    gpio_set_level(pin, 1);
    esp_rom_delay_us(40);
    gpio_set_direction(pin, GPIO_MODE_INPUT);

    // Wait for sensor response
    int timeout = 0;
    while (gpio_get_level(pin) == 0) {
        if (++timeout > 1000) return ESP_FAIL;
    }
    timeout = 0;
    while (gpio_get_level(pin) == 1) {
        if (++timeout > 1000) return ESP_FAIL;
    }

    // Read 40 bits
    for (int i = 0; i < 40; i++) {
        timeout = 0;
        while (gpio_get_level(pin) == 0) {
            if (++timeout > 1000) return ESP_FAIL;
        }
        int64_t t = esp_timer_get_time();
        timeout = 0;
        while (gpio_get_level(pin) == 1) {
            if (++timeout > 1000) return ESP_FAIL;
        }
        if (esp_timer_get_time() - t > 40) {
            dat[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    if (dat[4] == ((dat[0] + dat[1] + dat[2] + dat[3]) & 0xFF)) {
        if (sensor_type == DHT11) {
            *humidity = dat[0];
            *temperature = dat[2];
        }
        return ESP_OK;
    }
    return ESP_FAIL;
}
#endif