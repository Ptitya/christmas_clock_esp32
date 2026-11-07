#ifndef BUTTON_SAFE_H
#define BUTTON_SAFE_H

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#define BTN_MODE GPIO_NUM_13   
#define BTN_INC  GPIO_NUM_27   // ย้ายมาใช้ขา 27 ปลอดภัยและเสถียรที่สุด

static int current_mode = 0; // 0=เวลา, 1=อุณหภูมิ, 2=ตั้งวัน, 3=ตั้งเวลา, 4=ตั้งปลุก
static int edit_sub_mode = 0; 

static int s_hour = 0, s_minute = 0, s_second = 0;
static int s_day = 1, s_month = 1, s_year = 2026;
static int a_hour = 7, a_minute = 0; 

static bool last_mode = 1;
static bool last_inc = 1;

void init_buttons() {
    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_INPUT;
    
    // กำหนดพินและเปิดใช้ Pull-up ทั้งสองปุ่ม
    io_conf.pin_bit_mask = (1ULL << BTN_MODE) | (1ULL << BTN_INC);
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; 
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);
}

void tick_clock() {
    if (++s_second >= 60) { 
        s_second = 0; 
        if (++s_minute >= 60) { 
            s_minute = 0; 
            s_hour = (s_hour + 1) % 24; 
        } 
    }
}

bool check_buttons() {
    bool changed = false;
    bool curr_mode = gpio_get_level(BTN_MODE);
    bool curr_inc = gpio_get_level(BTN_INC);

    // 1. ตรวจสอบปุ่ม MODE (ขา 13)
    if (curr_mode == 0 && last_mode == 1) { 
        vTaskDelay(pdMS_TO_TICKS(50)); 
        
        if (current_mode == 0) { 
            current_mode = 1; 
        }
        else if (current_mode == 1) { 
            current_mode = 2; edit_sub_mode = 0; 
        }
        else if (current_mode == 2) { 
            if (++edit_sub_mode > 2) { edit_sub_mode = 0; current_mode = 3; } 
        }
        else if (current_mode == 3) { 
            if (++edit_sub_mode > 2) { edit_sub_mode = 0; current_mode = 4; } 
        }
        else if (current_mode == 4) { 
            if (++edit_sub_mode > 1) { edit_sub_mode = 0; current_mode = 0; } 
        }
        changed = true;
    } 
    last_mode = curr_mode;

    // 2. ตรวจสอบปุ่ม INC (ขา 27)
    if (curr_inc == 0 && last_inc == 1) {
        vTaskDelay(pdMS_TO_TICKS(50)); 
        
        if (current_mode == 2) { 
            if (edit_sub_mode == 0) { s_day++; if (s_day > 31) s_day = 1; }
            else if (edit_sub_mode == 1) { s_month++; if (s_month > 12) s_month = 1; }
            else if (edit_sub_mode == 2) { s_year++; if (s_year > 2035) s_year = 2026; }
        } 
        else if (current_mode == 3) { 
            if (edit_sub_mode == 0) s_hour = (s_hour + 1) % 24;
            else if (edit_sub_mode == 1) s_minute = (s_minute + 1) % 60;
            else if (edit_sub_mode == 2) s_second = (s_second + 1) % 60;
        } 
        else if (current_mode == 4) { 
            if (edit_sub_mode == 0) a_hour = (a_hour + 1) % 24;
            else a_minute = (a_minute + 1) % 60;
        }
        changed = true;
    }
    last_inc = curr_inc;
    
    return changed;
}
#endif