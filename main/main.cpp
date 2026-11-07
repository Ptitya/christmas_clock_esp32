#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lcd_basic.h"
#include "wifi_time.h"  // นำเข้าไฟล์ Wi-Fi
#include "button_safe.h"

static const char *TAG = "STEP_2_WIFI";
static float mock_temp = 25.5; 
static float mock_hum = 60.0;

void draw_screen() {
    char buf1[32], buf2[32];

    if (current_mode == 0) {
        sprintf(buf1, "   %02d/%02d/%04d   ", s_day, s_month, s_year);
        sprintf(buf2, "    %02d:%02d:%02d    ", s_hour, s_minute, s_second);
    } 
    else if (current_mode == 1) {
        sprintf(buf1, " Temp: %.1f C   ", mock_temp);
        sprintf(buf2, " Hum:  %.1f %%   ", mock_hum);
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
        else sprintf(buf2, "Minute: %02d      ", a_minute);
    }

    lcd_print_xy(0, 0, buf1);
    lcd_print_xy(0, 1, buf2);
}

extern "C" void app_main(void) {
    init_lcd();
    
    // แสดงข้อความระหว่างรอต่อเน็ต
    lcd_print_xy(0, 0, " Connecting to  ");
    lcd_print_xy(0, 1, " WiFi for Time  ");
    
    // เชื่อมต่อ Wi-Fi และดึงเวลามาตรฐานโลก
    init_wifi_and_sync_time();
    
    // อัปเดตเวลาลงในตัวแปรของเรา
    get_current_time(s_hour, s_minute, s_second, s_day, s_month, s_year);
    
    init_buttons();
    draw_screen();
    
    int tick_cnt = 0;
    while (1) {
        if (check_buttons()) {
            draw_screen();
        }

        if (++tick_cnt >= 20) {
            tick_cnt = 0;
            tick_clock();
            if (current_mode == 0) draw_screen();
        }
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}