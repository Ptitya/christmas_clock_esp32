#ifndef ALARM_BUZZER_H
#define ALARM_BUZZER_H

#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUZZER_PIN GPIO_NUM_23

// กำหนดค่าความถี่โน้ตดนตรีมาตรฐาน (Hz)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

void init_buzzer() {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 2000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .gpio_num = BUZZER_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);
}

// ฟังก์ชันเล่นโน้ตพร้อมเว้นช่องว่างเล็กน้อยเพื่อให้เสียงแยกออกจากกันชัดเจน (Articulation)
void play_tone(int freq, int duration_ms) {
    if (freq > 0) {
        ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4096); // Duty 50%
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    } else {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    }
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    
    // ปิดเสียงช่วงสั้นๆ ระหว่างโน้ตเพื่อให้เพลงไม่ติดกันเป็นพืด
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    vTaskDelay(pdMS_TO_TICKS(35));
}

// เมโลดี้เพลง Jingle Bells ฉบับปรับปรุงจังหวะให้ไพเราะและสมูทขึ้น
void play_alarm_sound() {
    // โน้ตเพลงท่อนฮุกที่จัดสรรจังหวะสั้น-ยาวอย่างลงตัว
    int notes[] = {
        NOTE_E4, NOTE_E4, NOTE_E4,  // Jin-gle bells
        NOTE_E4, NOTE_E4, NOTE_E4,  // Jin-gle bells
        NOTE_E4, NOTE_G4, NOTE_C4, NOTE_D4, NOTE_E4, // Jin-gle all the way
        NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_E4, NOTE_D4, NOTE_G4
    };
    
    int durations[] = {
        200, 200, 400,  // Jin-gle bells (ตัวสุดท้ายลากยาวขึ้น)
        200, 200, 400,  // Jin-gle bells
        200, 200, 300, 100, 600, // Jin-gle all the way
        200, 200, 200, 200,  200, 200, 200, 200,  100, 100, 200, 200, 400, 400 // Oh what fun...
    };
    
    int size = sizeof(notes) / sizeof(notes[0]);
    for (int i = 0; i < size; i++) {
        play_tone(notes[i], durations[i]);
    }
}

#endif