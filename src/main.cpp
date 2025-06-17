#include <Arduino.h>
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
//#include "utilities.h" // Раскомментируйте, если добавите файл utilities.h для управления питанием

// Пины для SIM800
#define MODEM_RX 26
#define MODEM_TX 27
#define SerialMon Serial
#define SerialAT Serial1

// Настройки GPRS
const char apn[]  = "your_apn"; // Замените на APN вашего оператора
const char gprsUser[] = "";
const char gprsPass[] = "";

// Telegram
const char botToken[] = "YOUR_BOT_TOKEN"; // Токен вашего бота
const char chatId[] = "YOUR_CHAT_ID";     // Ваш chat_id

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
HttpClient http(client, "api.telegram.org", 80); // Используем HTTP, т.к. SIM800L не поддерживает SSL

void sendToTelegram(const String& text) {
  String url = "/bot" + String(botToken) + "/sendMessage";
  String contentType = "application/x-www-form-urlencoded";
  String postData = "chat_id=" + String(chatId) + "&text=" + text;

  http.beginRequest();
  http.post(url);
  http.sendHeader("Content-Type", contentType);
  http.sendHeader("Content-Length", postData.length());
  http.beginBody();
  http.print(postData);
  http.endRequest();

  int statusCode = http.responseStatusCode();
  String response = http.responseBody();
  SerialMon.print("Telegram response: ");
  SerialMon.println(response);
}

void setup() {
  SerialMon.begin(115200);
  delay(10);

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  // Если у вас версия с IP5306, раскомментируйте следующую строку и добавьте файл utilities.h:
  // setupModem();

  // Инициализация модема
  SerialMon.println("Initializing modem...");
  modem.restart();

  // Подключение к GPRS
  SerialMon.print("Connecting to network...");
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
    while (true);
  }
  SerialMon.println(" success");

  // Настройка SIM800L на прием SMS
  SerialAT.println("AT+CMGF=1"); // Текстовый режим SMS
  delay(500);
  SerialAT.println("AT+CNMI=2,2,0,0,0"); // Уведомления о новых SMS
  delay(500);

  SerialMon.println("Готов к приему SMS");
}

void loop() {
  if (SerialAT.available()) {
    String sms = SerialAT.readString();
    SerialMon.println("RAW: " + sms);

    if (sms.indexOf("+CMT:") != -1) {
      int start = sms.indexOf("+CMT:") + 5;
      int end = sms.indexOf('"', start);
      String phoneNumber = sms.substring(start + 1, end);

      start = sms.indexOf('"', end + 1) + 2;
      String message = sms.substring(start);

      SerialMon.println("\nНовое SMS получено!");
      SerialMon.println("-------------------");
      SerialMon.println("Отправитель: " + phoneNumber);
      SerialMon.println("Сообщение: " + message);
      SerialMon.println("-------------------");

      // Отправка в Telegram
      String text = "SMS от: " + phoneNumber + "\n" + message;
      sendToTelegram(text);
    }
  }
}

