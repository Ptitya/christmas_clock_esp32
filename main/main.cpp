#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lcd_basic.h"

static const char *TAG = "STEP_1.5";

// ฟังก์ชันช่วยแปลงชื่อเดือนภาษาอังกฤษเป็นตัวเลข
static int parse_month(const char* m) { 
    const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec"; 
    char* pos = strstr(months, m); 
    return pos ? (pos - months) / 3 + 1 : 11; 
}

// ตัวแปรเก็บเวลา โดยดึงจากคอมพิวเตอร์อัตโนมัติ ณ ตอนที่กด Build
static int s_hour = [](){ int h,m,s; sscanf(__TIME__,"%d:%d:%d",&h,&m,&s); return h; }();
static int s_minute = [](){ int h,m,s; sscanf(__TIME__,"%d:%d:%d",&h,&m,&s); return m; }();
static int s_second = [](){ int h,m,s; sscanf(__TIME__,"%d:%d:%d",&h,&m,&s); return s; }();

static int s_day = [](){ char sm[4]; int d,y; sscanf(__DATE__,"%s %d %d",sm,&d,&y); return d; }();
static int s_month = [](){ char sm[4]; int d,y; sscanf(__DATE__,"%s %d %d",sm,&d,&y); return parse_month(sm); }();
static int s_year = [](){ char sm[4]; int d,y; sscanf(__DATE__,"%s %d %d",sm,&d,&y); return y; }();

static float mock_temp = 25.5; 
static float mock_hum = 60.0;

void tick_clock() {
    if (++s_second >= 60) {
        s_second = 0;
        if (++s_minute >= 60) {
            s_minute = 0;
            s_hour = (s_hour + 1) % 24;
        }
    }
}

extern "C" void app_main(void) {
    init_lcd();
    ESP_LOGI(TAG, "LCD Initialized. Current Time Synced!");
    
    int toggle_timer = 0;

    while (1) {
        char buf1[32], buf2[32];

        // 0-4 วินาที โชว์เวลา, 5-9 วินาที โชว์อุณหภูมิ
        if (toggle_timer < 5) {
            sprintf(buf1, "   %02d/%02d/%04d   ", s_day, s_month, s_year);
            sprintf(buf2, "    %02d:%02d:%02d    ", s_hour, s_minute, s_second);
        } else {
            sprintf(buf1, " Temp: %.1f C   ", mock_temp);
            sprintf(buf2, " Hum:  %.1f %%   ", mock_hum);
        }

        lcd_print_xy(0, 0, buf1);
        lcd_print_xy(0, 1, buf2);

        vTaskDelay(pdMS_TO_TICKS(1000));
        tick_clock();
        
        toggle_timer++;
        if (toggle_timer >= 10) toggle_timer = 0;
    }
}