#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lcd_basic.h" // ดึงไฟล์จอที่เพิ่งสร้างมาใช้

static const char *TAG = "STEP_1";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Starting Step 1: Monitor and LCD Test...");

    init_lcd(); // เปิดการเชื่อมต่อจอ
    ESP_LOGI(TAG, "LCD Initialized.");

    lcd_print_xy(0, 0, "  HELLO ESP32   ");
    lcd_print_xy(0, 1, " LCD IS WORKING ");

    int counter = 0;

    // ลูปหลักเพื่อให้บอร์ดไม่ค้าง
    while (1) {
        ESP_LOGI(TAG, "System is running perfectly... %d", counter++);
        
        // ให้มันหลับยาวๆ 1 วินาที เพื่อไม่ให้ I2C ทำงานหนักเกินไป
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}