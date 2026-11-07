#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lcd_basic.h"
#include "wifi_time.h"
#include "button_safe.h"
#include "sensor_dht.h"

static const char *TAG = "SMART_CLOCK";

void draw_screen() {
    char buf1[32], buf2[32];

    if (current_mode == 0) {
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
        else sprintf(buf2, "Minute: %02d      ", a_minute);
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
    
    // 2. เริ่มต้นระบบปุ่มกดและสร้าง Task อ่านค่าเซนเซอร์ DHT11 เบื้องหลัง
    init_buttons();
    xTaskCreate(dht_sensor_task, "dht_task", 2048, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Smart Clock System Fully Initialized!");
    draw_screen();
    
    int tick_cnt = 0;
    while (1) {
        // เช็กปุ่มกดเปลี่ยนโหมดและตั้งค่าตลอดเวลา
        if (check_buttons()) {
            draw_screen();
        }

        // นับเวลาเดินทุกๆ 1 วินาที (20 รอบ x 50ms)
        if (++tick_cnt >= 20) {
            tick_cnt = 0;
            tick_clock();
            
            // รีเฟรชหน้าจออัตโนมัติทุกวินาทีเฉพาะหน้าจอเวลา (0) หรือหน้าจออุณหภูมิ (1)
            if (current_mode == 0 || current_mode == 1) {
                draw_screen();
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}