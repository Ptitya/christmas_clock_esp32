#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lcd_basic.h"
#include "wifi_time.h"
#include "button_safe.h"
#include "sensor_dht.h"
#include "alarm_buzzer.h"
#include "led_control.h"

static const char *TAG = "SMART_CLOCK";
bool is_alarm_active = false; 

void draw_screen() {
    char buf1[32], buf2[32];

    if (is_alarm_active) {
        ESP_LOGI(TAG, "Status: ALARM TRIGGERED! 🔔");
    } else {
        switch (current_mode) {
            case 0:
                ESP_LOGI(TAG, "LCD Mode 0 (Time) -> Date: %02d/%02d/%04d | Time: %02d:%02d:%02d", s_day, s_month, s_year, s_hour, s_minute, s_second);
                break;
            case 1:
                ESP_LOGI(TAG, "LCD Mode 1 (Sensor) -> Temp: %.1f C | Hum: %.1f %%", current_temp, current_hum);
                break;
            case 2:
                if (edit_sub_mode == 0)
                    ESP_LOGI(TAG, "Editing Day: %02d", s_day);
                else if (edit_sub_mode == 1)
                    ESP_LOGI(TAG, "Editing Month: %02d", s_month);
                else
                    ESP_LOGI(TAG, "Editing Year: %04d", s_year);
                break;
            case 3:
                if (edit_sub_mode == 0)
                    ESP_LOGI(TAG, "Editing Hour: %02d", s_hour);
                else if (edit_sub_mode == 1)
                    ESP_LOGI(TAG, "Editing Minute: %02d", s_minute);
                else
                    ESP_LOGI(TAG, "Editing Second: %02d", s_second);
                break;
            case 4:
                if (edit_sub_mode == 0)
                    ESP_LOGI(TAG, "Editing Alarm Hour: %02d", a_hour);
                else
                    ESP_LOGI(TAG, "Editing Alarm Minute: %02d", a_minute);
                break;
        }
    }

    if (is_alarm_active) {
        sprintf(buf1, "   * ALARM! *   ");
        sprintf(buf2, "  Time is up!   ");
    }
    else if (current_mode == 0) {
        sprintf(buf1, "   %02d/%02d/%04d   ", s_day, s_month, s_year);
        sprintf(buf2, "    %02d:%02d:%02d    ", s_hour, s_minute, s_second);
    } 
    else if (current_mode == 1) {
        sprintf(buf1, " Temp: %.1f C   ", current_temp);
        sprintf(buf2, " Hum:  %.1f %%   ", current_hum);
    } 
    else if (current_mode == 2) {
        sprintf(buf1, "SET DATE        ");
        if (edit_sub_mode == 0) sprintf(buf2, "Day:   %02d       ", s_day);
        else if (edit_sub_mode == 1) sprintf(buf2, "Month: %02d       ", s_month);
        else sprintf(buf2, "Year:  %04d     ", s_year);
    }
    else if (current_mode == 3) {
        sprintf(buf1, "SET TIME        ");
        if (edit_sub_mode == 0) sprintf(buf2, "Hour:   %02d      ", s_hour);
        else if (edit_sub_mode == 1) sprintf(buf2, "Minute: %02d      ", s_minute);
        else sprintf(buf2, "Second: %02d      ", s_second);
    }
    else if (current_mode == 4) {
        sprintf(buf1, "SET ALARM       ");
        if (edit_sub_mode == 0) sprintf(buf2, "Hour:   %02d      ", a_hour);
        else if (edit_sub_mode == 1) sprintf(buf2, "Minute: %02d      ", a_minute);
    }

    lcd_print_xy(0, 0, buf1);
    lcd_print_xy(0, 1, buf2);
}

extern "C" void app_main(void) {
    init_lcd();
    
    lcd_print_xy(0, 0, " Connecting to  ");
    lcd_print_xy(0, 1, " WiFi & NTP...  ");
    
    // เรียกใช้งานฟังก์ชันเริ่มต้น Wi-Fi และดึงเวลา NTP
    init_wifi_and_sync_time();
    
    init_buttons();
    init_buzzer();
    init_led(); 
    xTaskCreate(dht_sensor_task, "dht_task", 2048, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Smart Clock System Fully Initialized!");
    draw_screen();
    
    int tick_cnt = 0;
    while (1) {
        if (check_buttons()) {
            if (is_alarm_active) {
                is_alarm_active = false;
                set_led_state(0); 
            }
            draw_screen();
        }

        if (is_alarm_active) {
            set_led_state(1); 
            play_alarm_sound();
            draw_screen();
        }

        if (!is_alarm_active && ++tick_cnt >= 20) {
            tick_cnt = 0;
            tick_clock();
            
            if (s_hour == a_hour && s_minute == a_minute && s_second == 0) {
                is_alarm_active = true;
            }
            
            draw_screen();
        }
        
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}