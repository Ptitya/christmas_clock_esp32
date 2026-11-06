#ifndef LCD_BASIC_H
#define LCD_BASIC_H

#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2C_NUM I2C_NUM_0
static uint8_t LCD_ADDR = 0x27; 
static const char *LCD_TAG = "LCD";

void lcd_write_byte(uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (LCD_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM, cmd, pdMS_TO_TICKS(50)); // เพิ่มเวลาเผื่อจออ่านช้า
    i2c_cmd_link_delete(cmd);
}

void lcd_send_4bit(uint8_t data) {
    lcd_write_byte(data | 0x08);       
    lcd_write_byte(data | 0x04 | 0x08); // EN=1
    vTaskDelay(pdMS_TO_TICKS(2));       // หน่วงเวลาให้ชิปจออ่านทัน 100% (ชัวร์กว่า us)
    lcd_write_byte((data & ~0x04) | 0x08); // EN=0
    vTaskDelay(pdMS_TO_TICKS(2));
}

void lcd_cmd(uint8_t cmd) {
    lcd_send_4bit(cmd & 0xF0);
    lcd_send_4bit((cmd << 4) & 0xF0);
    vTaskDelay(pdMS_TO_TICKS(2)); 
}

void lcd_data(uint8_t data) {
    lcd_send_4bit((data & 0xF0) | 0x01); 
    lcd_send_4bit(((data << 4) & 0xF0) | 0x01);
    vTaskDelay(pdMS_TO_TICKS(1)); 
}

void init_lcd() {
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = GPIO_NUM_21;
    conf.scl_io_num = GPIO_NUM_22;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 100000;
    
    i2c_driver_delete(I2C_NUM); // เคลียร์ไดรเวอร์เก่าทิ้งก่อน ป้องกันบั๊กค้าง
    i2c_param_config(I2C_NUM, &conf);
    i2c_driver_install(I2C_NUM, conf.mode, 0, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(200));

    // ค้นหา Address
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (0x3F << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    if (i2c_master_cmd_begin(I2C_NUM, cmd, pdMS_TO_TICKS(50)) == ESP_OK) { 
        LCD_ADDR = 0x3F; 
    } else {
        LCD_ADDR = 0x27;
    }
    i2c_cmd_link_delete(cmd);

    vTaskDelay(pdMS_TO_TICKS(100)); // รอจอรับไฟเต็มที่
    lcd_send_4bit(0x30); vTaskDelay(pdMS_TO_TICKS(10));
    lcd_send_4bit(0x30); vTaskDelay(pdMS_TO_TICKS(2));
    lcd_send_4bit(0x30); vTaskDelay(pdMS_TO_TICKS(2));
    lcd_send_4bit(0x20); vTaskDelay(pdMS_TO_TICKS(2));
    
    lcd_cmd(0x28); 
    lcd_cmd(0x08); 
    lcd_cmd(0x01); vTaskDelay(pdMS_TO_TICKS(10)); // clear screen ใช้เวลานาน
    lcd_cmd(0x06); 
    lcd_cmd(0x0C); 
}

void lcd_print_xy(uint8_t col, uint8_t row, const char *str) {
    uint8_t row_offsets[] = {0x00, 0x40};
    lcd_cmd(0x80 | (col + row_offsets[row]));
    while (*str) {
        lcd_data(*str++);
    }
}
#endif