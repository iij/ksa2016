#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include "INA226.h"
#include "ST7032.h"

extern "C" {
#include "user_interface.h"
}

// ----- ここをチームごとに書き換えてください -----
#define HOST "xxx.xxx.xx"
#define TEAM "team_X"
#define PASS "xxxxxxxxxx"
#define WIFI_SSID "KSA_team_X"
#define WIFI_PASS "xxxxxxxx"
#define CALIBRATION_A 1.0
#define CALIBRATION_B 0.0
// -------------------------------------------

#define LED 13
#define SDA 4
#define SCL 5
#define SAMPLING_INTERVAL 100 // 100ミリ秒間隔で計測
#define SAMPLING_COUNT    50  // 50回計測したら平均値を計算してデータを送信

INA226 ina226;
ST7032 lcd;

// 1. 初回起動時の処理
void setup() {
    Serial.begin(115200);
    delay(20);
    Serial.println("Start");
    pinMode(LED, OUTPUT);
    digitalWrite(13, HIGH); 

    lcd.begin(16,2);
    lcd.setContrast(35);
    lcd.clear();

    ina226.begin(SDA, SCL);
    Serial.print("INA226 Manufacture ID: ");
    Serial.println(ina226.readId(), HEX);

    connect_to_wifi();
}

// 2. 繰り返し実行する処理
void loop() {
    float average_current;
    float average_voltage;
    float average_power;
    float average_wind_speed;

    static int index = 0;
    static float currents[SAMPLING_COUNT];
    static float voltages[SAMPLING_COUNT];
    static float wind_speeds[SAMPLING_COUNT];

    delay(SAMPLING_INTERVAL);

    // 2-1. データを測定
    currents[index]    = ina226.readCurrent();
    voltages[index]    = ina226.readVoltage();
    wind_speeds[index] = get_wind_speed();

    // 2-2. データが一定数溜まるまで、ここまでの処理を繰り返す
    if (index < SAMPLING_COUNT - 1) {
        index++;
        return;
    }

    // 2-3. 平均を計算
    average_current = mean(currents);
    average_voltage = mean(voltages) * 0.001;
    average_power   = average_current * average_voltage;
    average_wind_speed = mean(wind_speeds);
    print_measured_data(average_voltage, average_power, average_wind_speed);

    // 2-4. データを送信する前に Wi-Fi 接続状況をチェック
    reconnect_wifi_if_disconnected();
    print_wifi_rssi_lcd(0);

    // 2-5. データをクラウドに送信
    post_to_cloud(average_current, average_voltage, average_power, average_wind_speed);

    // 2-6. 次のサンプリングを実施するため、配列のインデックスを初期化
    index = 0;
}

float get_analog_voltage() {
    return system_adc_read() / 1024.0;
}

float get_wind_speed() {
    return (((get_analog_voltage() * 2.0 - 0.4) / 1.6 * 32.4) - CALIBRATION_B) / CALIBRATION_A;
}

void clear_lcd_row(int row) {
    lcd.setCursor(0, row);
    lcd.print("                ");
}

void connect_to_wifi() {
    Serial.print("Wi-Fi Connecting");
    lcd.setCursor(0,0);
    lcd.print("Wi-Fi Connecting");

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
}

void reconnect_wifi_if_disconnected() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wi-Fi diconnected. Reconnecting...");
        connect_to_wifi();
    }
}

void print_wifi_rssi_lcd(int row) {
    char lcd_message[16];

    sprintf(lcd_message, "Wi-Fi %ddBm", wifi_station_get_rssi());
    clear_lcd_row(0);
    lcd.setCursor(0,0);
    lcd.print(lcd_message);
}

float mean(float* array) {
  float sum  = 0.0;

  for (int i = 0; i < SAMPLING_COUNT; i++) {
      sum += abs(array[i]);
  }
  return sum / (float)SAMPLING_COUNT;
}

void post_to_cloud(float current, float voltage, float power, float wind_speed) {
    char current_str[8];
    char voltage_str[8];
    char power_str[8];
    char wind_speed_str[8];
    char request[1024];
    char body[1024];
    HTTPClient http;

    dtostrf(current,   -1, 1, current_str);
    dtostrf(voltage,   -1, 1, voltage_str);
    dtostrf(power,     -1, 1, power_str);
    dtostrf(wind_speed,-1, 1, wind_speed_str);

    sprintf(request, "http://%s:%s@%s/db", TEAM, PASS, HOST);
    sprintf(body, "mA %s\nV %s\nmW %s\nm/s %s\n", current_str, voltage_str, power_str, wind_speed_str);
    Serial.println(request);
    Serial.print(body);

    http.begin(request);
    int httpCode = http.POST(body);
    Serial.printf("[HTTP] code: %d\n\n", httpCode);
}

void print_all(float* array) {
    for (int i = 0; i < SAMPLING_COUNT; i++) {
        Serial.print(array[i]);
        Serial.print(", ");
    }
    Serial.println("");
}

void print_measured_data(float voltage, float power, float wind) {
    char voltage_str[8];
    char power_str[8];
    char wind_str[8];
    char message[24];

    dtostrf(voltage, -1, 1, voltage_str);
    dtostrf(power,   -1, 1, power_str);
    dtostrf(wind,    -1, 1, wind_str);

    sprintf(message, "%sV %smW %sm", voltage_str, power_str, wind_str);
    clear_lcd_row(1);
    lcd.setCursor(0,1);
    lcd.print(message);
}

