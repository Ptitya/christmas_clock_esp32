#ifndef BUTTON_SAFE_H
#define BUTTON_SAFE_H

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

// กำหนดพินปุ่มกดตามที่คุณต้องการ (โหมด 13, เพิ่มค่า 14)
#define BTN_MODE GPIO_NUM_13   
#define BTN_INC  GPIO_NUM_14   

// ตัวแปรสถานะเมนู (0=เวลา, 1=อุณหภูมิ, 2=ตั้งวัน, 3=ตั้งเวลา, 4=ตั้งปลุก)
static int current_mode = 0; 
static int edit_sub_mode = 0; // ย่อยในแต่ละโหมดการตั้งค่า

// ตัวแปรเก็บค่าเวลา วันที่ และเวลาปลุก
static int s_hour = 0, s_minute = 0, s_second = 0;
static int s_day = 1, s_month = 1, s_year = 2026;
static int a_hour = 7, a_minute = 0; // ตั้งเวลาปลุกเริ่มต้น 07:00

// ตัวแปรจำสถานะปุ่มก่อนหน้า (Active Low เพราะใช้ Pull-up ภายใน)
static bool last_mode = 1;
static bool last_inc = 1;

// ฟังก์ชันตั้งค่า GPIO สำหรับปุ่มกด
void init_buttons() {
    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << BTN_MODE) | (1ULL << BTN_INC);
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // เปิดตัวต้านทานดึงไฟขึ้น เพื่อให้ปุ่มอ่านค่าเสถียร
    gpio_config(&io_conf);
}

// ฟังก์ชันเดินนาฬิกาทุกๆ 1 วินาที
void tick_clock() {
    if (++s_second >= 60) { 
        s_second = 0; 
        if (++s_minute >= 60) { 
            s_minute = 0; 
            s_hour = (s_hour + 1) % 24; 
        } 
    }
}

// ฟังก์ชันตรวจสอบการกดปุ่ม (คืนค่า true เมื่อมีการกดเปลี่ยนแปลง)
bool check_buttons() {
    bool changed = false;
    bool curr_mode = gpio_get_level(BTN_MODE);
    bool curr_inc = gpio_get_level(BTN_INC);

    // 1. ตรวจสอบปุ่ม MODE (เปลี่ยนหน้าเมนู / สลับตำแหน่งตั้งค่า)
    if (curr_mode == 0 && last_mode == 1) { 
        vTaskDelay(pdMS_TO_TICKS(50)); // Software Debounce กันปุ่มเด้งรัว
        
        if (current_mode == 0) { 
            current_mode = 1; // จากหน้าเวลา -> ไปหน้าอุณหภูมิ
        }
        else if (current_mode == 1) { 
            current_mode = 2; edit_sub_mode = 0; // ไปหน้าตั้งวันที่ (เริ่มที่ Day)
        }
        else if (current_mode == 2) { 
            if (++edit_sub_mode > 2) { edit_sub_mode = 0; current_mode = 3; } // วน Day -> Month -> Year แล้วไปตั้งเวลา
        }
        else if (current_mode == 3) { 
            if (++edit_sub_mode > 2) { edit_sub_mode = 0; current_mode = 4; } // วน Hour -> Minute -> Second แล้วไปตั้งนาฬิกาปลุก
        }
        else if (current_mode == 4) { 
            if (++edit_sub_mode > 1) { edit_sub_mode = 0; current_mode = 0; } // วน Hour -> Minute แล้วกลับมาหน้าหลัก
        }
        changed = true;
    } 
    last_mode = curr_mode;

    // 2. ตรวจสอบปุ่ม INC (เพิ่มค่าตัวเลขในโหมดตั้งค่า)
    if (curr_inc == 0 && last_inc == 1) {
        vTaskDelay(pdMS_TO_TICKS(50)); // Software Debounce
        
        if (current_mode == 2) { // โหมดตั้งวันที่
            if (edit_sub_mode == 0) s_day = (s_day % 31) + 1;
            else if (edit_sub_mode == 1) s_month = (s_month % 12) + 1;
            else if (edit_sub_mode == 2) { s_year++; if(s_year > 2035) s_year = 2026; }
        } 
        else if (current_mode == 3) { // โหมดตั้งเวลา
            if (edit_sub_mode == 0) s_hour = (s_hour + 1) % 24;
            else if (edit_sub_mode == 1) s_minute = (s_minute + 1) % 60;
            else if (edit_sub_mode == 2) s_second = (s_second + 1) % 60;
        } 
        else if (current_mode == 4) { // โหมดตั้งปลุก
            if (edit_sub_mode == 0) a_hour = (a_hour + 1) % 24;
            else a_minute = (a_minute + 1) % 60;
        }
        changed = true;
    }
    last_inc = curr_inc;
    
    return changed;
}

#endif