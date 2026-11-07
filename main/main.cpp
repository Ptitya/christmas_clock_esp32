#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lcd_basic.h"
#include "wifi_time.h"
#include "button_safe.h"
#include "sensor_dht.h"
#include "alarm_buzzer.h"

static const char *TAG = "SMART_CLOCK";
bool is_alarm_active = false; // ตัวแปรสถานะกำลังปลุก
static int last_logged_mode = -1; // เก็บสถานะโหมดล่าสุดเพื่อพริ้นต์ Log เฉพาะตอนเปลี่ยน

void draw_screen() {
    char buf1[32], buf2[32];

    // พิมพ์ Log บน Serial Monitor ให้สอดคล้องและแสดงค่าเดียวกับหน้าจอ LCD
    if (is_alarm_active) {
        ESP_LOGI(TAG, "Monitor Status: ALARM TRIGGERED! 🔔 (Time is up!)");
    } else {
        switch (current_mode) {
            case 0:
                ESP_LOGI(TAG, "LCD Mode 0 (Time): Date: %02d/%02d/%04d | Time: %02d:%02d:%02d", 
                         s_day, s_month, s_year, s_hour, s_minute, s_second);
                break;
            case 1:
                ESP_LOGI(TAG, "LCD Mode 1 (Sensor): Temp: %.1f C | Hum: %.1f %%", current_temp, current_hum);
                break;
            case 2:
                if (edit_sub_mode == 0)
                    ESP_LOGI(TAG, "LCD Mode 2 (Set Date): Editing Day -> Day: %02d", s_day);
                else if (edit_sub_mode == 1)
                    ESP_LOGI(TAG, "LCD Mode 2 (Set Date): Editing Month -> Month: %02d", s_month);
                else
                    ESP_LOGI(TAG, "LCD Mode 2 (Set Date): Editing Year -> Year: %04d", s_year);
                break;
            case 3:
                if (edit_sub_mode == 0)
                    ESP_LOGI(TAG, "LCD Mode 3 (Set Time): Editing Hour -> Hour: %02d", s_hour);
                else if (edit_sub_mode == 1)
                    ESP_LOGI(TAG, "LCD Mode 3 (Set Time): Editing Minute -> Minute: %02d", s_minute);
                else
                    ESP_LOGI(TAG, "LCD Mode 3 (Set Time): Editing Second -> Second: %02d", s_second);
                break;
            case 4:
                if (edit_sub_mode == 0)
                    ESP_LOGI(TAG, "LCD Mode 4 (Set Alarm): Editing Hour -> Alarm Hour: %02d", a_hour);
                else
                    ESP_LOGI(TAG, "LCD Mode 4 (Set Alarm): Editing Minute -> Alarm Minute: %02d", a_minute);
                break;
        }
    }

    // แสดงหน้าจอ LCD ตามโหมดและสถานะ
    if (is_alarm_active) {
        sprintf(buf1, "  * ALARM! *  ");
        sprintf(buf2, " Time is up!    ");
    }
    else if (current_mode == 0) {
        // หน้า 1: วันที่ / เวลา
        sprintf(buf1, "   %02d/%02d/%04d   ", s_day, s_month, s_year);
        sprintf(buf2, "    %02d:%02d:%02d    ", s_hour, s_minute, s_second);
    } 
    else if (current_mode == 1) {
        // หน้า 2: อุณหภูมิและความชื้นจากเซนเซอร์จริง
        sprintf(buf1, " Temp: %.1f C   ", current_temp);
        sprintf(buf2, " Hum:  %.1f %%   ", current_hum);
    } 
    else if (current_mode == 2) {
        // หน้า 3: ตั้งค่าวันที่
        sprintf(buf1, "SET DATE        ");
        if (edit_sub_mode == 0) sprintf(buf2, "Day:   %02d       ", s_day);
        else if (edit_sub_mode == 1) sprintf(buf2, "Month: %02d       ", s_month);
        else sprintf(buf2, "Year:  %04d     ", s_year);
    }
    else if (current_mode == 3) {
        // หน้า 4: ตั้งค่าเวลา
        sprintf(buf1, "SET TIME        ");
        if (edit_sub_mode == 0) sprintf(buf2, "Hour:   %02d      ", s_hour);
        else if (edit_sub_mode == 1) sprintf(buf2, "Minute: %02d      ", s_minute);
        else sprintf(buf2, "Second: %02d      ", s_second);
    }
    else if (current_mode == 4) {
        // หน้า 5: ตั้งปลุก
        sprintf(buf1, "SET ALARM       ");
        if (edit_sub_mode == 0) sprintf(buf2, "Hour:   %02d      ", a_hour);
        else if (edit_sub_mode == 1) sprintf(buf2, "Minute: %02d      ", a_minute);
    }

    lcd_print_xy(0, 0, buf1);
    lcd_print_xy(0, 1, buf2);
}

extern "C" void app_main(void) {
    init_lcd();
    
    // 1. เชื่อมต่อ Wi-Fi และดึงเวลา NTP
    lcd_print_xy(0, 0, " Connecting to  ");
    lcd_print_xy(0, 1, " WiFi & NTP...  ");
    init_wifi_and_sync_time();
    get_current_time(s_hour, s_minute, s_second, s_day, s_month, s_year);
    
    // 2. เริ่มต้นระบบฮาร์ดแวร์ ปุ่มกด, Buzzer และ Task อ่านค่าเซนเซอร์ DHT11
    init_buttons();
    init_buzzer();
    xTaskCreate(dht_sensor_task, "dht_task", 2048, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Smart Clock System Fully Initialized!");
    draw_screen();
    
    int tick_cnt = 0;
    while (1) {
        // เช็กปุ่มกดตลอดเวลา (รองรับการกดปุ่มเพื่อเปลี่ยนโหมด ตั้งค่า และปิดเสียงปลุก)
        if (check_buttons()) {
            if (is_alarm_active) {
                is_alarm_active = false;
                ESP_LOGI(TAG, "Alarm dismissed by user button press.");
            }
            draw_screen();
        }

        // ถ้าระบบอยู่ในโหมดปลุก ให้เล่นเพลง Jingle Bells วนไปพร้อมรีเฟรชจอ
        if (is_alarm_active) {
            play_alarm_sound();
            draw_screen();
        }

        // นับเวลาเดินทุกๆ 1 วินาที (20 รอบ x 50ms)
        if (!is_alarm_active && ++tick_cnt >= 20) {
            tick_cnt = 0;
            tick_clock();
            
            // เช็กเวลาปลุก เทียบชั่วโมงและนาทีปัจจุบันกับเวลาตั้งปลุก
            if (s_hour == a_hour && s_minute == a_minute && s_second == 0) {
                is_alarm_active = true;
            }
            
            // รีเฟรชหน้าจออัตโนมัติทุกวินาทีเฉพาะ "หน้าจอแสดงเวลา (Mode 0)" เท่านั้น
            // เพื่อให้หน้าจออื่นๆ (เช่น อุณหภูมิหรือหน้าตั้งค่า) ค้างอยู่ได้ตามที่ผู้ใช้เลือก
            if (current_mode == 0) {
                draw_screen();
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}