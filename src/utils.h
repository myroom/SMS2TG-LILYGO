#ifndef UCS2_UTILS_H
#define UCS2_UTILS_H
#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <TinyGsmClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// Объявляем глобальные переменные, которые определены в main.cpp
extern TinyGsm modem;
extern HttpClient http;
extern HttpClient httpsClient;
extern WiFiClientSecure wifiClientSecure;
extern String imei;
extern const char* ssid;
extern const char* password;
#define SerialMon Serial

// Прототипы функций
String decodeUCS2(const String& ucs2);
String formatSmsDatetime(const String& raw);
void modemPowerOn();
void sendDataOverHttp(const String& phone, const String& message, const String& datetime);
void sendDataOverHttpWifi(const String& phone, const String& message, const String& datetime);
void sendSMS(const String& phone, const String& message);
void readAllUnreadSMS();
bool sendPing();
void setup_wifi();

// Корректная конвертация UCS2 (HEX) в String (UTF-8)
String decodeUCS2(const String& ucs2) {
  String result;
  for (int i = 0; i < ucs2.length(); i += 4) {
    String hexChar = ucs2.substring(i, i + 4);
    uint16_t unicode = (uint16_t)strtol(hexChar.c_str(), NULL, 16);
    char buf[5] = {0};
    if (unicode < 0x80) {
      buf[0] = (char)unicode;
    } else if (unicode < 0x800) {
      buf[0] = 0xC0 | (unicode >> 6);
      buf[1] = 0x80 | (unicode & 0x3F);
    } else {
      buf[0] = 0xE0 | (unicode >> 12);
      buf[1] = 0x80 | ((unicode >> 6) & 0x3F);
      buf[2] = 0x80 | (unicode & 0x3F);
    }
    result += String(buf);
  }
  return result;
}

// Форматирование даты/времени из SMS в читаемый вид
String formatSmsDatetime(const String& raw) {
  // Пример входа: 25/06/18,02:59:35+12
  if (raw.length() < 17) return raw;
  String day = raw.substring(0, 2);
  String month = raw.substring(3, 5);
  String year = raw.substring(6, 8);
  String hour = raw.substring(9, 11);
  String min = raw.substring(12, 14);
  String sec = raw.substring(15, 17);
  String tz = raw.substring(17); // +12
  String formatted = day + "." + month + ".20" + year + " " + hour + ":" + min + ":" + sec + " " + tz;
  return formatted;
}

// Включение модема
void modemPowerOn() {
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);

  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_POWER_ON, HIGH);
  digitalWrite(MODEM_RST, HIGH);
  delay(1000);

  digitalWrite(MODEM_PWKEY, HIGH);
  delay(1000);
  digitalWrite(MODEM_PWKEY, LOW);
  delay(1000);
}

// Отправка данных на сервер по HTTP
void sendDataOverHttp(const String& phone, const String& message, const String& datetime) {
  if (imei.length() == 0) {
    SerialMon.println("IMEI is not available, sending is not possible.");
    return;
  }
  
  ArduinoJson::DynamicJsonDocument jsonDoc(256);
  jsonDoc["imei"] = imei;
  jsonDoc["date"] = datetime;
  jsonDoc["phone"] = phone;
  jsonDoc["message"] = message;

  String postData;
  serializeJson(jsonDoc, postData);

  SerialMon.println("Sending data to server...");
  SerialMon.println(postData);

  String url = "/api/messages"; // Путь на вашем сервере, куда отправлять данные
  String contentType = "application/json";

  http.post(url, contentType, postData);

  int statusCode = http.responseStatusCode();
  String response = http.responseBody();
  SerialMon.print("HTTP status: ");
  SerialMon.println(statusCode);
  SerialMon.print("Server response: ");
  SerialMon.println(response);
  
  http.stop();
}

// Отправка данных на сервер по WiFi (HTTPS)
void sendDataOverHttpWifi(const String& phone, const String& message, const String& datetime) {
  if (WiFi.status() != WL_CONNECTED) {
    SerialMon.println("WiFi not connected, cannot send data over WiFi.");
    return;
  }
  
  ArduinoJson::DynamicJsonDocument jsonDoc(256);
  jsonDoc["imei"] = imei;
  jsonDoc["date"] = datetime;
  jsonDoc["phone"] = phone;
  jsonDoc["message"] = message;

  String postData;
  serializeJson(jsonDoc, postData);

  SerialMon.println("Sending data to server over WiFi...");
  SerialMon.println(postData);

  String url = "/api/confirm"; // Путь на вашем сервере, куда отправлять данные
  String contentType = "application/json";

  httpsClient.post(url, contentType, postData);

  int statusCode = httpsClient.responseStatusCode();
  String response = httpsClient.responseBody();
  SerialMon.print("HTTPS status: ");
  SerialMon.println(statusCode);
  SerialMon.print("Server response: ");
  SerialMon.println(response);
  
  httpsClient.stop();
}

// Отправка SMS
void sendSMS(const String& phone, const String& message) {
  SerialMon.print("Sending SMS to number " + phone + "...");
  if (modem.sendSMS(phone, message)) {
    SerialMon.println(" SMS sent successfully!");
  } else {
    SerialMon.println(" SMS sending failed");
  }
}

// Чтение всех непрочитанных SMS
void readAllUnreadSMS() {
  SerialAT.println("AT+CMGF=1");
  delay(200);
  // Опросим первые 20 ячеек SIM-карты (можно увеличить при необходимости)
  for (int i = 1; i <= 20; ++i) {
    SerialAT.print("AT+CMGR=");
    SerialAT.println(i);
    delay(300);
    // После чтения удаляем SMS
    SerialAT.print("AT+CMGD=");
    SerialAT.println(i);
    delay(100);
  }
}

// Отправка ping на сервер
bool sendPing() {
  // Формируем JSON для ping
  ArduinoJson::DynamicJsonDocument jsonDoc(128);
  jsonDoc["imei"] = imei;
  jsonDoc["uptime"] = millis();

  String postData;
  serializeJson(jsonDoc, postData);

  SerialMon.println("Sending ping to server...");
  SerialMon.println(postData);

  String url = "/api/ping"; // Путь для ping
  String contentType = "application/json";

  http.post(url, contentType, postData);

  int statusCode = http.responseStatusCode();
  String response = http.responseBody();
  SerialMon.print("Ping HTTP status: ");
  SerialMon.println(statusCode);
  SerialMon.print("Ping server response: ");
  SerialMon.println(response);

  // Закрываем соединение перед обработкой ответа
  http.stop();

  if (statusCode == 200) {
    // Успешный HTTP запрос, парсим JSON
    ArduinoJson::DynamicJsonDocument responseDoc(128);
    deserializeJson(responseDoc, response);

    if (!responseDoc["status"].isNull() && responseDoc["status"] == "error") {
      SerialMon.println("Ping error detected in response.");
      return false; // Ошибка, если сервер вернул status: "error"
    }
    
    SerialMon.println("Ping successful.");
    return true; // Успешный ping
  } else {
    // Ошибка HTTP
    SerialMon.println("Ping HTTP error.");
    return false;
  }
}

void setup_wifi() {
  SerialMon.print("Connecting to WiFi: ");
  SerialMon.println(ssid);
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    SerialMon.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    SerialMon.println("\nWiFi connected");
    SerialMon.print("IP address: ");
    SerialMon.println(WiFi.localIP());
    // Insecure, but required for this example
    wifiClientSecure.setInsecure();
  } else {
    SerialMon.println("\nWiFi connection failed");
  }
}

#endif // UCS2_UTILS_H 