#include <Arduino.h>
#include "base64.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include <ArduinoJson.h>
#include <I2S.h>

const char* ssid = "J09 502";
const char* password = "qwertyuiop111";

const char* BAIDU_API_KEY = "your_api_key";
const char* BAIDU_SECRET_KEY = "your_secret_key";
char baidu_token[256] = {0};

#define SAMPLE_RATE 16000
#define RECORD_TIME 2
#define DATA_LEN (SAMPLE_RATE * RECORD_TIME)

uint16_t adc_data[DATA_LEN];
uint8_t adc_start_flag = 0;
uint8_t adc_complete_flag = 0;

hw_timer_t* timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer() {
    portENTER_CRITICAL_ISR(&timerMux);
    static uint32_t num = 0;
    
    if (adc_start_flag == 1) {
        uint32_t raw_data = I2S.read();
        adc_data[num] = (uint16_t)(raw_data & 0xFFFF);
        num++;
        
        if (num >= DATA_LEN) {
            adc_complete_flag = 1;
            adc_start_flag = 0;
            num = 0;
        }
    }
    portEXIT_CRITICAL_ISR(&timerMux);
}

bool baidu_get_token() {
    HTTPClient http;
    String url = "https://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=";
    url += BAIDU_API_KEY;
    url += "&client_secret=";
    url += BAIDU_SECRET_KEY;
    
    Serial.println("Getting token from: " + url);
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Token response: " + payload);
        
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);
        
        if (doc.containsKey("access_token")) {
            strncpy(baidu_token, doc["access_token"].as<const char*>(), sizeof(baidu_token) - 1);
            Serial.println("Token obtained: " + String(baidu_token));
            http.end();
            return true;
        } else {
            Serial.println("Error: access_token not found in response");
            if (doc.containsKey("error")) {
                Serial.println("Error message: " + doc["error"].as<String>());
            }
        }
    } else {
        Serial.println("HTTP Error: " + String(httpCode));
        Serial.println("Error message: " + http.errorToString(httpCode));
    }
    
    http.end();
    return false;
}

String baidu_speech_recognition(uint16_t* data, int data_len) {
    if (strlen(baidu_token) == 0) {
        Serial.println("Error: Token is empty, please get token first");
        return "";
    }
    
    String jsonStr = String("{") +
        "\"format\":\"pcm\"," +
        "\"rate\":" + String(SAMPLE_RATE) + "," +
        "\"dev_pid\":1537," +
        "\"channel\":1," +
        "\"cuid\":\"esp32-sense-001\"," +
        "\"token\":\"" + String(baidu_token) + "\"," +
        "\"len\":" + String(data_len * 2) + "," +
        "\"speech\":\"" + base64::encode((uint8_t*)data, data_len * 2).c_str() + "\"" +
        "}";
    
    Serial.println("JSON length: " + String(jsonStr.length()));
    
    HTTPClient http;
    http.begin("http://vop.baidu.com/server_api");
    http.addHeader("Content-Type", "application/json");
    
    Serial.println("Sending POST request...");
    int httpCode = http.POST(jsonStr);
    
    if (httpCode == HTTP_CODE_OK) {
        String response = http.getString();
        Serial.println("Response: " + response);
        
        DynamicJsonDocument doc(2048);
        deserializeJson(doc, response);
        
        if (doc.containsKey("result")) {
            String result = doc["result"][0].as<String>();
            Serial.println("Recognition result: " + result);
            http.end();
            return result;
        } else {
            Serial.println("Recognition failed");
            if (doc.containsKey("err_no")) {
                Serial.println("Error code: " + String(doc["err_no"].as<int>()));
                Serial.println("Error message: " + doc["err_msg"].as<String>());
            }
        }
    } else {
        Serial.println("HTTP Error: " + String(httpCode));
        Serial.println("Error message: " + http.errorToString(httpCode));
    }
    
    http.end();
    return "";
}

void setup() {
    Serial.begin(115200);
    Serial.println("========================================");
    Serial.println("ESP32S3 Sense Audio Repair");
    Serial.println("========================================");
    
    I2S.setAllPins(-1, 42, 41, -1, -1);
    if (!I2S.begin(PDM_MONO_MODE, SAMPLE_RATE, 16)) {
        Serial.println("Failed to initialize I2S!");
        while (1);
    }
    Serial.println("I2S initialized successfully");
    
    Serial.println("Connecting to WiFi: " + String(ssid));
    WiFi.begin(ssid, password);
    
    int count = 0;
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        count++;
        if (count >= 50) {
            Serial.println("\nWiFi connect failed!");
            break;
        }
        delay(200);
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
        
        Serial.println("\nGetting Baidu token...");
        if (baidu_get_token()) {
            Serial.println("Token obtained successfully!");
        } else {
            Serial.println("Failed to get token!");
        }
    }
    
    timer = timerBegin(0, 80, true);
    timerAlarmWrite(timer, 125, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmEnable(timer);
    timerStop(timer);
    
    Serial.println("\n========================================");
    Serial.println("Ready! Send '1' to start recognition");
    Serial.println("========================================");
}

void loop() {
    if (Serial.available() > 0) {
        char cmd = Serial.read();
        
        if (cmd == '1') {
            Serial.println("\n----------- Start Recording -----------");
            
            adc_start_flag = 1;
            adc_complete_flag = 0;
            timerStart(timer);
            
            while (!adc_complete_flag) {
                delay(1);
            }
            
            timerStop(timer);
            Serial.println("Recording complete!");
            
            Serial.println("----------- Sending to Baidu -----------");
            String result = baidu_speech_recognition(adc_data, DATA_LEN);
            
            if (result.length() > 0) {
                Serial.println("Success! Result: " + result);
            } else {
                Serial.println("Recognition failed, check error above");
            }
            
            Serial.println("----------- Recognition Done -----------");
            
        } else if (cmd == 't') {
            Serial.println("\n----------- Getting Token -----------");
            baidu_get_token();
            Serial.println("----------- Token Done -----------");
        }
    }
    
    delay(10);
}