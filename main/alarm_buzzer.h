#ifndef ALARM_BUZZER_H
#define ALARM_BUZZER_H

#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUZZER_PIN GPIO_NUM_23

#define NOTE_E4  330
#define NOTE_G4  392
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_F4  349

void init_buzzer() {
    ledc_timer_config_t ledc_timer = {};
    ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_timer.duty_resolution = LEDC_TIMER_13_BIT;
    ledc_timer.timer_num = LEDC_TIMER_0;
    ledc_timer.freq_hz = 2000;
    ledc_timer.clk_cfg = LEDC_AUTO_CLK;
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {};
    ledc_channel.gpio_num = BUZZER_PIN;
    ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_channel.channel = LEDC_CHANNEL_0;
    ledc_channel.intr_type = LEDC_INTR_DISABLE;
    ledc_channel.timer_sel = LEDC_TIMER_0;
    ledc_channel.duty = 0;
    ledc_channel.hpoint = 0;
    ledc_channel_config(&ledc_channel);
}

void play_tone(int freq, int duration_ms) {
    if (freq > 0) {
        ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4096);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    } else {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    }
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    vTaskDelay(pdMS_TO_TICKS(35));
}

void play_alarm_sound() {
    int notes[] = {
        NOTE_E4, NOTE_E4, NOTE_E4,  
        NOTE_E4, NOTE_E4, NOTE_E4,  
        NOTE_E4, NOTE_G4, NOTE_C4, NOTE_D4, NOTE_E4, 
        NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_E4, NOTE_D4, NOTE_G4
    };
    
    int durations[] = {
        200, 200, 400,  
        200, 200, 400,  
        200, 200, 300, 100, 600, 
        200, 200, 200, 200,  200, 200, 200, 200,  100, 100, 200, 200, 400, 400 
    };
    
    int size = sizeof(notes) / sizeof(notes[0]);
    for (int i = 0; i < size; i++) {
        play_tone(notes[i], durations[i]);
    }
}

#endif