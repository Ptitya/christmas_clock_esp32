#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_http_server.h"
#include "esp_log.h"
#include "wifi_time.h"
#include "led_control.h"

static const char *TAG_WEB = "WEB_SERVER";

extern int current_mode;
extern int edit_sub_mode;
extern bool is_alarm_active;
extern int s_hour, s_minute, s_second;
extern int s_day, s_month, s_year;
extern int a_hour, a_minute;
extern float current_temp;
extern float current_hum;

// ฟังก์ชันสั่งงานภายนอกที่ต้องประกาศจาก main.cpp
extern void trigger_snooze();
extern void trigger_stop_alarm();

static const char* web_page_html = R"rawliteral(
<!DOCTYPE html>
<html lang="th">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Christmas Smart Clock</title>
    <link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@500;700&family=Prompt:wght@400;600&display=swap" rel="stylesheet">
    <style>
        :root {
            --xmas-red: #c41e3a;
            --xmas-green: #0b6623;
            --xmas-gold: #ffd700;
            --lcd-bg: #112211;
            --lcd-text: #33ff33;
        }
        body {
            background: linear-gradient(135deg, #0f2027, #203a43, #2c5364);
            font-family: 'Prompt', sans-serif;
            color: #fff;
            margin: 0;
            padding: 15px;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
            text-align: center;
        }
        .container {
            background: rgba(20, 30, 25, 0.95);
            border: 4px solid var(--xmas-gold);
            border-radius: 20px;
            padding: 25px;
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.6), 0 0 20px rgba(255, 215, 0, 0.4);
            text-align: center;
            max-width: 450px;
            width: 100%;
            display: flex;
            flex-direction: column;
            align-items: center;
        }
        h1 { color: var(--xmas-gold); font-size: 22px; margin-bottom: 5px; text-align: center; }
        .subtitle { font-size: 12px; color: #a8dadc; margin-bottom: 15px; text-align: center; }
        
        .lcd-screen {
            background-color: var(--lcd-bg);
            border: 6px solid #224422;
            border-radius: 10px;
            padding: 15px;
            font-family: 'Orbitron', monospace;
            color: var(--lcd-text);
            font-size: 20px;
            letter-spacing: 2px;
            text-shadow: 0 0 8px rgba(51, 255, 51, 0.7);
            margin-bottom: 15px;
            box-shadow: inset 0 0 15px rgba(0,0,0,0.8);
            min-height: 60px;
            width: 85%;
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
            text-align: center;
        }
        .lcd-line { margin: 4px 0; width: 100%; text-align: center; }
        .mode-indicator { font-size: 12px; color: var(--xmas-gold); margin-bottom: 12px; text-transform: uppercase; text-align: center; width: 100%; }
        
        .section-box {
            background: rgba(255, 255, 255, 0.05);
            border: 1px dashed rgba(255, 215, 0, 0.4);
            border-radius: 10px;
            padding: 10px;
            margin-bottom: 10px;
            width: 90%;
            display: flex;
            flex-direction: column;
            align-items: center;
            text-align: center;
        }
        .section-title { font-size: 12px; color: var(--xmas-gold); margin-bottom: 6px; font-weight: 600; text-align: center; width: 100%; }
        
        .btn-grid { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 6px; width: 100%; justify-items: center; }
        .btn-grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; width: 100%; justify-items: center; }
        
        .btn {
            background: linear-gradient(to bottom, var(--xmas-red), #900c1e);
            color: white;
            border: 2px solid var(--xmas-gold);
            padding: 8px 5px;
            border-radius: 8px;
            font-family: 'Prompt', sans-serif;
            font-size: 12px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s ease;
            box-shadow: 0 3px 5px rgba(0,0,0,0.3);
            width: 100%;
            text-align: center;
        }
        .btn:hover { transform: translateY(-2px); filter: brightness(1.1); }
        .btn-green { background: linear-gradient(to bottom, var(--xmas-green), #043912); }
        
        .status-badge { margin-top: 10px; padding: 6px 12px; border-radius: 20px; font-size: 12px; display: inline-block; text-align: center; }
        .alarm-active { background-color: var(--xmas-red); animation: pulse 1s infinite; }
        .alarm-standby { background-color: #333; color: #888; }
        @keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.5; } 100% { opacity: 1; } }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎄 Christmas Smart Clock 🎅</h1>
        <div class="subtitle">ESP32 Control Dashboard</div>

        <div class="mode-indicator" id="modeIndicator">Loading...</div>

        <div class="lcd-screen">
            <div class="lcd-line" id="lcdLine1">Syncing...</div>
            <div class="lcd-line" id="lcdLine2">Please Wait</div>
        </div>

        <div class="section-box">
            <div class="section-title">📺 โหมดแสดงผล (Mode 0 & 1)</div>
            <div class="btn-grid-2">
                <button class="btn btn-green" onclick="sendAction('mode_0')">Mode 0: เวลา</button>
                <button class="btn btn-green" onclick="sendAction('mode_1')">Mode 1: เซ็นเซอร์</button>
            </div>
        </div>

        <div class="section-box">
            <div class="section-title">📅 Mode 2: ตั้งวันที่ (Day / Month / Year)</div>
            <div class="btn-grid">
                <button class="btn" onclick="sendAction('inc_day')">➕ วัน</button>
                <button class="btn" onclick="sendAction('inc_month')">➕ เดือน</button>
                <button class="btn" onclick="sendAction('inc_year')">➕ ปี</button>
            </div>
        </div>

        <div class="section-box">
            <div class="section-title">⏰ Mode 3: ตั้งเวลา (Hour / Minute / Second)</div>
            <div class="btn-grid">
                <button class="btn" onclick="sendAction('inc_hour')">➕ ชั่วโมง</button>
                <button class="btn" onclick="sendAction('inc_min')">➕ นาที</button>
                <button class="btn" onclick="sendAction('inc_sec')">➕ วินาที</button>
            </div>
        </div>

        <div class="section-box">
            <div class="section-title">🚨 Mode 4: ตั้งนาฬิกาปลุก (Alarm)</div>
            <div class="btn-grid-2">
                <button class="btn" onclick="sendAction('inc_alarm_h')">➕ ชม. ปลุก</button>
                <button class="btn" onclick="sendAction('inc_alarm_m')">➕ นาที ปลุก</button>
            </div>
        </div>

        <!-- แยกปุ่มควบคุมเสียงปลุกอย่างชัดเจน -->
        <div class="section-box">
            <div class="section-title">⚡ ควบคุมการปลุก (Alarm Control)</div>
            <div class="btn-grid-2">
                <button class="btn btn-green" onclick="sendAction('snooze')">💤 เลื่อนปลุก (+5 นาที)</button>
                <button class="btn" onclick="sendAction('stop_alarm')">⏹️ หยุดปลุกทันที</button>
            </div>
        </div>

        <div>
            <div id="statusBadge" class="status-badge alarm-standby">สถานะ: กำลังเชื่อมต่อ...</div>
        </div>
    </div>

    <script>
        function fetchStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('lcdLine1').innerText = data.line1;
                    document.getElementById('lcdLine2').innerText = data.line2;
                    document.getElementById('modeIndicator').innerText = data.indicator;
                    
                    const badge = document.getElementById('statusBadge');
                    if(data.is_alarm) {
                        badge.className = "status-badge alarm-active";
                        badge.innerText = "🚨 นาฬิกาปลุกกำลังดัง!";
                    } else {
                        badge.className = "status-badge alarm-standby";
                        badge.innerText = `ตั้งปลุกเวลา: ${data.alarm_time}`;
                    }
                })
                .catch(err => console.log('Error fetching status:', err));
        }

        function sendAction(cmd) {
            fetch('/cmd?type=' + cmd)
                .then(response => response.text())
                .then(() => fetchStatus());
        }

        setInterval(fetchStatus, 1000);
        fetchStatus();
    </script>
</body>
</html>
)rawliteral";

static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, web_page_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t status_get_handler(httpd_req_t *req) {
    char buf[512];
    char l1[32], l2[32];
    char ind[128];

    if (is_alarm_active) {
        snprintf(l1, sizeof(l1), "   * ALARM! *   ");
        snprintf(l2, sizeof(l2), "   Time is up!   ");
        snprintf(ind, sizeof(ind), "⚠️ แจ้งเตือนปลุกบนบอร์ด!");
    } else {
        switch(current_mode) {
            case 0:
                snprintf(l1, sizeof(l1), "   %02d/%02d/%04d   ", s_day, s_month, s_year);
                snprintf(l2, sizeof(l2), "    %02d:%02d:%02d    ", s_hour, s_minute, s_second);
                snprintf(ind, sizeof(ind), "⌛ Mode 0: วันและเวลาปัจจุบัน (Day & Time)");
                break;
            case 1:
                snprintf(l1, sizeof(l1), " Temp: %.1f C   ", current_temp);
                snprintf(l2, sizeof(l2), " Hum:  %.1f %%   ", current_hum);
                snprintf(ind, sizeof(ind), "⛄ Mode 1: เซ็นเซอร์ DHT11");
                break;
            case 2:
                snprintf(l1, sizeof(l1), "    SET DATE    ");
                snprintf(l2, sizeof(l2), "   %02d/%02d/%04d   ", s_day, s_month, s_year);
                snprintf(ind, sizeof(ind), "📅 Mode 2: ตั้งวันที่ (Day / Month / Year)");
                break;
            case 3:
                snprintf(l1, sizeof(l1), "    SET TIME    ");
                snprintf(l2, sizeof(l2), "    %02d:%02d:%02d    ", s_hour, s_minute, s_second);
                snprintf(ind, sizeof(ind), "⏰ Mode 3: ตั้งเวลา (Hour / Minute / Second)");
                break;
            case 4:
                snprintf(l1, sizeof(l1), "   SET ALARM    ");
                snprintf(l2, sizeof(l2), "    Time %02d:%02d   ", a_hour, a_minute);
                snprintf(ind, sizeof(ind), "🚨 Mode 4: ตั้งนาฬิกาปลุก (Alarm)");
                break;
        }
    }

    snprintf(buf, sizeof(buf), 
        "{\"line1\":\"%s\",\"line2\":\"%s\",\"indicator\":\"%s\",\"is_alarm\":%s,\"alarm_time\":\"%02d:%02d\"}",
        l1, l2, ind, is_alarm_active ? "true" : "false", a_hour, a_minute);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t cmd_get_handler(httpd_req_t *req) {
    char query[64];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char val[16];
        if (httpd_query_key_value(query, "type", val, sizeof(val)) == ESP_OK) {
            if (strcmp(val, "mode_0") == 0) {
                current_mode = 0;
            } else if (strcmp(val, "mode_1") == 0) {
                current_mode = 1;
            } else if (strcmp(val, "inc_day") == 0) {
                current_mode = 2;
                s_day = (s_day % 31) + 1;
            } else if (strcmp(val, "inc_month") == 0) {
                current_mode = 2;
                s_month = (s_month % 12) + 1;
            } else if (strcmp(val, "inc_year") == 0) {
                current_mode = 2;
                s_year++;
            } else if (strcmp(val, "inc_hour") == 0) {
                current_mode = 3;
                s_hour = (s_hour + 1) % 24;
            } else if (strcmp(val, "inc_min") == 0) {
                current_mode = 3;
                s_minute = (s_minute + 1) % 60;
            } else if (strcmp(val, "inc_sec") == 0) {
                current_mode = 3;
                s_second = (s_second + 1) % 60;
            } else if (strcmp(val, "inc_alarm_h") == 0) {
                current_mode = 4;
                a_hour = (a_hour + 1) % 24;
            } else if (strcmp(val, "inc_alarm_m") == 0) {
                current_mode = 4;
                a_minute = (a_minute + 1) % 60;
            } else if (strcmp(val, "snooze") == 0) {
                trigger_snooze(); // เรียกฟังก์ชันเลื่อนปลุก
            } else if (strcmp(val, "stop_alarm") == 0) {
                trigger_stop_alarm(); // เรียกฟังก์ชันหยุดปลุก
            }
        }
    }
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

void start_web_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root_uri = { .uri = "/", .method = HTTP_GET, .handler = root_get_handler, .user_ctx = NULL };
        httpd_uri_t status_uri = { .uri = "/status", .method = HTTP_GET, .handler = status_get_handler, .user_ctx = NULL };
        httpd_uri_t cmd_uri = { .uri = "/cmd", .method = HTTP_GET, .handler = cmd_get_handler, .user_ctx = NULL };

        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &status_uri);
        httpd_register_uri_handler(server, &cmd_uri);
        ESP_LOGI(TAG_WEB, "Christmas Web Server Started Successfully!");
    }
}

#endif