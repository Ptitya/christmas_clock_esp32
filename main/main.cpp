#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "lcd_basic.h"
#include "wifi_time.h"
#include "button_safe.h"
#include "sensor_dht.h"
#include "alarm_buzzer.h"
#include "led_control.h"
#include "web_server.h"

static const char *TAG = "SMART_CLOCK";
bool is_alarm_active = false; 

// กำหนดพินปุ่ม GPIO 13 (Snooze) และ GPIO 27 (Stop)
#define PIN_BTN_SNOOZE  ((gpio_num_t)13)
#define PIN_BTN_STOP    ((gpio_num_t)27)

// ฟังก์ชันเลื่อนปลุก
void trigger_snooze() {
    if (is_alarm_active) {
        a_minute += 5;
        if (a_minute >= 60) {
            a_minute -= 60;
            a_hour = (a_hour + 1) % 24;
        }
        is_alarm_active = false;
        set_led_state(0); 
        ESP_LOGI(TAG, "Alarm Snoozed for 5 minutes. Next alarm: %02d:%02d", a_hour, a_minute);
    }
}

// ฟังก์ชันหยุดเสียงปลุกทันที
void trigger_stop_alarm() {
    if (is_alarm_active) {
        is_alarm_active = false;
        set_led_state(0); 
        ESP_LOGI(TAG, "Alarm stopped immediately via GPIO 27.");
    }
}

void draw_screen() {
    char buf1[32], buf2[32]; 
    static int last_logged_second = -1;
    static int last_logged_mode = -1;
    static int last_d = -1, last_m = -1, last_y = -1;
    static int last_h = -1, last_min = -1, last_s = -1;
    static int last_alarm_h = -1, last_alarm_m = -1;

    // พิมพ์ Log เฉพาะเมื่อมีการเปลี่ยนโหมด หรือข้อมูลในหน้านั้นๆ เปลี่ยนแปลง
    if (is_alarm_active) {
        ESP_LOGI(TAG, "Status: ALARM TRIGGERED! 🔔");
    } else {
        if (current_mode != last_logged_mode || 
            (current_mode == 0 && s_second != last_logged_second) ||
            (current_mode == 1 && s_second != last_logged_second) || 
            (current_mode == 2 && (s_day != last_d || s_month != last_m || s_year != last_y)) ||
            (current_mode == 3 && (s_hour != last_h || s_minute != last_min || s_second != last_s)) ||
            (current_mode == 4 && (a_hour != last_alarm_h || a_minute != last_alarm_m))) {
            
            last_logged_mode = current_mode;
            last_logged_second = s_second;
            last_d = s_day; last_m = s_month; last_y = s_year;
            last_h = s_hour; last_min = s_minute; last_s = s_second;
            last_alarm_h = a_hour; last_alarm_m = a_minute;

            switch (current_mode) {
                case 0:
                    if (!ntp_synced) {
                        ESP_LOGI(TAG, "LCD Mode 0 -> Syncing NTP Time...");
                    } else {
                        ESP_LOGI(TAG, "LCD Mode 0 (Time) -> Date: %02d/%02d/%04d | Time: %02d:%02d:%02d", s_day, s_month, s_year, s_hour, s_minute, s_second);
                    }
                    break;
                case 1:
                    ESP_LOGI(TAG, "LCD Mode 1 (Sensor) -> Temp: %.1f C | Hum: %.1f %%", current_temp, current_hum);
                    break;
                case 2:
                    ESP_LOGI(TAG, "LCD Mode 2 (Set Date) -> Date: %02d/%02d/%04d", s_day, s_month, s_year);
                    break;
                case 3:
                    ESP_LOGI(TAG, "LCD Mode 3 (Set Time) -> Time: %02d:%02d:%02d", s_hour, s_minute, s_second);
                    break;
                case 4:
                    ESP_LOGI(TAG, "LCD Mode 4 (Set Alarm) -> Alarm: %02d:%02d", a_hour, a_minute);
                    break;
            }
        }
    }

    // วาดข้อความออกจอ LCD 16x2
    if (is_alarm_active) {
        sprintf(buf1, "   * ALARM! *   ");
        sprintf(buf2, "  Time is up!   ");
    }
    else if (current_mode == 0) {
        if (!ntp_synced) {
            sprintf(buf1, "   Syncing...   ");
            sprintf(buf2, "  Please Wait.. ");
        } else {
            sprintf(buf1, "   %02d/%02d/%04d   ", s_day, s_month, s_year);
            sprintf(buf2, "    %02d:%02d:%02d    ", s_hour, s_minute, s_second);
        }
    } 
    else if (current_mode == 1) {
        sprintf(buf1, " Temp: %.1f C   ", current_temp);
        sprintf(buf2, " Hum:  %.1f %%   ", current_hum);
    } 
    else if (current_mode == 2) {
        sprintf(buf1, "    SET DATE    ");
        sprintf(buf2, "   %02d/%02d/%04d   ", s_day, s_month, s_year);
    }
    else if (current_mode == 3) {
        sprintf(buf1, "    SET TIME    ");
        sprintf(buf2, "    %02d:%02d:%02d    ", s_hour, s_minute, s_second);
    }
    else if (current_mode == 4) {
        sprintf(buf1, "   SET ALARM    ");
        sprintf(buf2, "   Time %02d:%02d   ", a_hour, a_minute);
    }

    lcd_print_xy(0, 0, buf1);
    lcd_print_xy(0, 1, buf2);
}

extern "C" void app_main(void) {
    init_lcd();
    
    lcd_print_xy(0, 0, " Connecting to  ");
    lcd_print_xy(0, 1, " WiFi & NTP...  ");
    
    init_wifi_and_sync_time();

    // เริ่มต้นระบบฮาร์ดแวร์และเซ็นเซอร์
    init_buttons();
    init_buzzer();
    init_led(); 

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_BTN_SNOOZE) | (1ULL << PIN_BTN_STOP);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; 
    gpio_config(&io_conf);

    xTaskCreate(dht_sensor_task, "dht_task", 2048, NULL, 5, NULL);
    start_web_server();
    
    ESP_LOGI(TAG, "Smart Clock System Fully Initialized!");
    draw_screen();

    static int last_second = -1;
    
    while (1) {
        if (is_alarm_active) {
            set_led_state(1); 

            if (gpio_get_level(PIN_BTN_SNOOZE) == 0) {
                ESP_LOGI(TAG, "GPIO 13 (Snooze) Pressed!");
                trigger_snooze();
                vTaskDelay(pdMS_TO_TICKS(300));
            } 
            else if (gpio_get_level(PIN_BTN_STOP) == 0) {
                ESP_LOGI(TAG, "GPIO 27 (Stop) Pressed!");
                trigger_stop_alarm();
                vTaskDelay(pdMS_TO_TICKS(300));
            }

            if (!is_alarm_active) {
                set_led_state(0);
            } else {
                play_alarm_sound(); 
            }
        } 
        else {
            set_led_state(0); 
            
            if (check_buttons()) {
                vTaskDelay(pdMS_TO_TICKS(100));
            }

            if (ntp_synced && s_hour == a_hour && s_minute == a_minute && s_second == 0) {
                is_alarm_active = true;
                ESP_LOGI(TAG, "Alarm Triggered!");
            }
        }

        // เรียกฟังก์ชันวาดจอ (ซึ่งมีตัวดักพิมพ์ Log แยก Mode อยู่ข้างในแล้ว)
        draw_screen();

        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}