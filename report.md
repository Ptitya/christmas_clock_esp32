# รายงาน โครงการนาฬิกาดิจิทัลอัจฉริยะด้วย ESP32 และฮาร์ดแวร์จริง (Report: ESP32 Real Hardware Digital Clock)
**แพลตฟอร์ม:** ESP32 DevKit (ESP-IDF Framework & FreeRTOS Architecture)

---

## 1. บทนำและสถาปัตยกรรมภาพรวมของระบบ (System Architecture)
รายงานฉบับนี้อธิบายกระบวนการออกแบบ การเขียนโค้ด การเชื่อมต่อฮาร์ดแวร์จริง ตลอดจนการวิเคราะห์ปัญหาและวิธีแก้ปัญหาในเชิงวิศวกรรมสำหรับโครงการ **"นาฬิกาดิจิทัลอัจฉริยะ" (Smart Digital Clock)** ที่พัฒนาขึ้นด้วยไมโครคอนโทรลเลอร์ ESP32 โดยใช้ระบบปฏิบัติการแบบเรียลไทม์ (**FreeRTOS Multitasking**) เพื่อแยกการทำงานของแต่ละโมดูลออกจากกันอย่างอิสระ ป้องกันไม่ให้ระบบเกิดอาการบล็อก (Blocking) และทำงานได้อย่างเสถียร

---

## 2. รายละเอียดฟังก์ชันหลัก (Core Functions)

### 2.1 ระบบแสดงผลเวลาและหน้าจอ (LCD 16x2 I2C Display)
* **หลักการทำงาน:** แสดงผลข้อมูลเวลา (ชั่วโมง:นาที:วินาที) วันที่ และโหมดการทำงานต่างๆ ผ่านหน้าจอ LCD 16x2 โดยใช้โมดูลแปลงสัญญาณ I2C (PCF8574) เพื่อลดการใช้ขาพินของ ESP32 ให้เหลือเพียง 2 ขาหลัก
* **การเชื่อมต่อฮาร์ดแวร์จริง:**
  * `SCL` $\rightarrow$ GPIO 22 (ESP32 Clock Line)
  * `SDA` $\rightarrow$ GPIO 21 (ESP32 Data Line)
  * `VCC / GND` $\rightarrow$ 5V / GND (ใช้ไฟ 5V เพื่อให้แสง Backlight สว่างคมชัด)
* **ตัวอย่างโค้ดการทำงานหลัก (ESP-IDF + I2C):**
  ```c
  #include <stdio.h>
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
  #include "driver/i2c.h"

  #define I2C_MASTER_SCL_IO 22
  #define I2C_MASTER_SDA_IO 21
  #define I2C_MASTER_NUM I2C_NUM_0
  #define I2C_MASTER_FREQ_HZ 100000

  // ฟังก์ชันตั้งค่าบัส I2C สำหรับติดต่อจอ LCD
  esp_err_t i2c_master_init(void) {
      i2c_config_t conf = {
          .mode = I2C_MODE_MASTER,
          .sda_io_num = I2C_MASTER_SDA_IO,
          .sda_pullup_en = GPIO_PULLUP_ENABLE,
          .scl_io_num = I2C_MASTER_SCL_IO,
          .scl_pullup_en = GPIO_PULLUP_ENABLE,
          .master.clk_speed = I2C_MASTER_FREQ_HZ,
      };
      i2c_param_config(I2C_MASTER_NUM, &conf);
      return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
  }
  ```
* **ปัญหาที่พบในฮาร์ดแวร์จริง:** 
  * หน้าจอติดไฟพื้นหลัง (Backlight) เป็นสีฟ้า แต่ไม่มีตัวอักษรหรือข้อความปรากฏขึ้นมาบนจอ
* **วิธีการแก้ไข:** 
  * ใช้ไขควงปากแบนขนาดเล็กหมุนปรับตัวต้านทานปรับค่าได้ (Contrast Trimpot / Potentiometer) ที่อยู่บริเวณด้านหลังของโมดูล I2C เพื่อปรับระดับความเข้มของตัวอักษรจนกว่าจะมองเห็นข้อความได้อย่างชัดเจน และตรวจสอบหมายเลข I2C Address (มักเป็น `0x27` หรือ `0x3F`) ให้ตรงกับฮาร์ดแวร์จริง

---

### 2.2 ระบบตั้งเวลาและควบคุมโหมด (Tactile Push Buttons)
* **หลักการทำงาน:** ใช้ปุ่มกดแบบ Tactile Switch สำหรับเปลี่ยนหน้าจอโหมดการทำงาน หรือกดเข้าสู่โหมดตั้งค่าเวลา โดยใช้โหมด Internal Pull-up ของ ESP32 เพื่อลดการใช้อุปกรณ์ภายนอก
* **การเชื่อมต่อฮาร์ดแวร์จริง:**
  * `Button 1 (Mode Switch)` $\rightarrow$ GPIO 18 (ต่อลง GND เมื่อกด)
  * `Button 2 (Time Adjust)` $\rightarrow$ GPIO 19 (ต่อลง GND เมื่อกด)
* **ตัวอย่างโค้ดจัดการปุ่มกดพร้อมระบบ Debounce (FreeRTOS Task):**
  ```c
  #include "driver/gpio.h"

  #define BUTTON_MODE_PIN GPIO_NUM_18

  void button_init() {
      gpio_config_t io_conf = {
          .intr_type = GPIO_INTR_DISABLE,
          .mode = GPIO_MODE_INPUT,
          .pin_bit_mask = (1ULL << BUTTON_MODE_PIN),
          .pull_up_en = GPIO_PULLUP_ENABLE,     // เปิด Pull-up ภายใน
          .pull_down_en = GPIO_PULLDOWN_DISABLE,
      };
      gpio_config(&io_conf);
  }

  void button_task(void *pvParameter) {
      button_init();
      while (1) {
          if (gpio_get_level(BUTTON_MODE_PIN) == 0) { // Active Low (เมื่อกดปุ่ม)
              vTaskDelay(pdMS_TO_TICK(50));          // Software Debounce หน่วงเวลา 50ms
              if (gpio_get_level(BUTTON_MODE_PIN) == 0) {
                  printf("Button Pressed: Mode Switched Successfully!\n");
                  
                  // รอจนกว่าจะปล่อยปุ่ม เพื่อป้องกันการรันซ้ำรัวๆ
                  while (gpio_get_level(BUTTON_MODE_PIN) == 0) {
                      vTaskDelay(pdMS_TO_TICK(10));
                  }
              }
          }
          vTaskDelay(pdMS_TO_TICK(50));
      }
  }
  ```
* **ปัญหาที่พบในฮาร์ดแวร์จริง:** 
  * เมื่อกดปุ่มเพียง 1 ครั้ง แต่ระบบกลับเปลี่ยนโหมดหรือนับค่าเบิ้ล 2-3 ครั้งทันที (Contact Bounce)
* **วิธีการแก้ไข:** 
  * เพิ่มคำสั่งหน่วงเวลาซอฟต์แวร์ (Software Debounce) ด้วย `vTaskDelay(pdMS_TO_TICK(50))` หลังจากการตรวจจับสถานะกดครั้งแรก เพื่อให้หน้าสัมผัสโลหะนิ่งสนิทก่อนอ่านค่าซ้ำ

---

## 3. รายละเอียดฟังก์ชันเสริม (Optional Functions)

### 3.1 ระบบวัดอุณหภูมิและความชื้น (DHT11 Sensor)
* **หลักการทำงาน:** อ่านค่าสภาพอากาศภายในห้อง (อุณหภูมิและความชื้นสัมพัทธ์) เพื่อนำมาแสดงสลับบนหน้าจอ LCD เป็นฟังก์ชันเสริม
* **การเชื่อมต่อฮาร์ดแวร์จริง:**
  * `VCC` $\rightarrow$ 3.3V
  * `DATA` $\rightarrow$ GPIO 4 (ต้องมีตัวต้านทาน Pull-up ขนาด 4.7kΩ ค่อมระหว่าง VCC กับ DATA)
  * `GND` $\rightarrow$ GND
* **ตัวอย่างโค้ดอ่านค่าเซ็นเซอร์:**
  ```c
  #include "dht.h"

  #define DHT_GPIO GPIO_NUM_4

  void dht_sensor_task(void *pvParameter) {
      int16_t temperature = 0;
      int16_t humidity = 0;

      while (1) {
          if (dht_read_data(DHT_TYPE_DHT11, DHT_GPIO, &humidity, &temperature) == ESP_OK) {
              printf("Temperature: %d.%d C | Humidity: %d.%d %%\n", 
                     temperature / 10, temperature % 10, 
                     humidity / 10, humidity % 10);
          } else {
              printf("Error: Failed to read data from DHT11 sensor\n");
          }
          // ข้อกำหนด: DHT11 ต้องการเวลาพักอย่างน้อย 2 วินาทีต่อการอ่าน 1 ครั้ง
          vTaskDelay(pdMS_TO_TICK(2000));
      }
  }
  ```
* **ปัญหาที่พบในฮาร์ดแวร์จริง:** 
  * อ่านค่าไม่ได้เลย ได้ค่าเป็น 0 ตลอด หรือขึ้น Error "Checksum Error" บ่อยครั้ง
* **วิธีการแก้ไข:** 
  * ตรวจสอบว่าได้ติดตั้งตัวต้านทาน Pull-up ขนาด 4.7kΩ ที่ขา Data หรือไม่ และตรวจสอบการสลับสาย VCC กับ GND ให้ถูกต้องเพื่อไม่ให้เซ็นเซอร์เกิดความเสียหาย พร้อมทั้งกำหนดเวลาหน่วงระหว่างการอ่านค่าไม่ให้เร็วกว่า 2 วินาที

---

### 3.2 ระบบเสียงแจ้งเตือน (Passive Buzzer)
* **หลักการทำงาน:** ส่งเสียงเพลง Jingle Bell แจ้งเตือนเมื่อถึงเวลาตั้งปลุก
* **การเชื่อมต่อฮาร์ดแวร์จริง:**
  * ต่อผ่านวงจรขยายกระแสด้วยทรานซิสเตอร์ (เช่น 2N2222) โดยขา Base ต่อผ่าน Resistor 1kΩ เข้ากับ GPIO 23 และขา Emitter (E) ของทรานซิสเตอร์: ต่อลง GND
* **ตัวอย่างโค้ดควบคุมเสียงแจ้งเตือน:**
  ```c
  #include "driver/ledc.h"
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"

  #define BUZZER_PIN      GPIO_NUM_23
  #define LEDC_TIMER      LEDC_TIMER_1
  #define LEDC_MODE       LEDC_LOW_SPEED_MODE
  #define LEDC_CHANNEL    LEDC_CHANNEL_1
  #define LEDC_DUTY_RES   LEDC_TIMER_10_BIT // ความละเอียด 10-bit (0-1023)

  void buzzer_init(void) {
    // 1. ตั้งค่าความถี่ PWM (Timer)
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = 2000, // ความถี่เริ่มต้น 2kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // 2. ผูก Channel กับ GPIO พิน
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = BUZZER_PIN,
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .duty           = 0, // เริ่มต้นปิดเสียง (Duty = 0)
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
  }

  // ฟังก์ชันเล่นโน้ตเพลง (ระบุความถี่ Hz และระยะเวลา ms)
  void play_tone(uint32_t frequency, int duration_ms) {
    if (frequency == 0) {
        // ถ้าความถี่เป็น 0 ให้หยุดเสียง
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    } else {
        // เปลี่ยนความถี่ตามโน้ตเพลง
        ledc_set_freq(LEDC_MODE, LEDC_TIMER, frequency);
        // เปิดความดัง (Duty 50% คือ 512 จาก 1023)
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 512);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    }
    
    vTaskDelay(pdMS_TO_TICK(duration_ms));

    // ปิดเสียงหลังเล่นจบโน้ต
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
  }
  ```

---

## 4. สรุปผลการดำเนินงาน
การพัฒนานาฬิกาดิจิทัลบนฮาร์ดแวร์จริงด้วย ESP32 และ FreeRTOS Task Architecture ช่วยให้ระบบสามารถประมวลผลการแสดงผล อ่านเซ็นเซอร์ และรับค่าปุ่มกดไปพร้อมกันได้อย่างราบรื่น การแก้ปัญหาทางฮาร์ดแวร์และซอฟต์แวร์ตามที่ระบุไว้ข้างต้นทำให้ชิ้นงานมีความสมบูรณ์พร้อมสำหรับการนำเสนอและใช้งานจริง