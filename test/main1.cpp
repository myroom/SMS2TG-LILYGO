// Ссылка на проект
// https://randomnerdtutorials.com/esp32-sim800l-publish-data-to-cloud/

#include <Arduino.h>

// Настройка TinyGSM библиотеки
#define TINY_GSM_MODEM_SIM800      // Modem is SIM800
#define TINY_GSM_RX_BUFFER   1024  // Set RX buffer to 1Kb
//#define TINY_GSM_DEBUG DEBUG_NONE
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
//#include "utilities.h" // Раскомментируйте, если добавите файл utilities.h для управления питанием

// Пины для SIM800
#define MODEM_RST 5
#define MODEM_PWKEY 4
#define MODEM_POWER_ON 23
#define MODEM_RX 26
#define MODEM_TX 27

// Устанавливаем порт для общения с монитором
#define SerialMon Serial

// Устанавливаем порт для общения с SIM800
#define SerialAT Serial1



// Определяем порт для отладки, если нужно
//#define DUMP_AT_COMMANDS

// Настройки GPRS
const char apn[]  = "internet"; // Замените на APN вашего оператора
const char gprsUser[] = "";
const char gprsPass[] = "";
const char simPIN[] = ""; // Добавлено для PIN-кода SIM-карты

// Telegram
const char botToken[] = "YOUR_BOT_TOKEN"; // Токен вашего бота
const char chatId[] = "YOUR_CHAT_ID";     // Ваш chat_id

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, SerialMon);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif

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
  SerialMon.begin(9600); // Скорость монитора порта и Serial Monitor должна быть 9600!
  delay(10);

   // Set modem reset, enable, power pins
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);

  SerialAT.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX); // Скорость общения с SIM800
  delay(3000);

  // Если у вас версия с IP5306, раскомментируйте следующую строку и добавьте файл utilities.h:
  // setupModem();

  // Инициализация модема
  SerialMon.println("Initializing modem...");
  modem.restart();

  // Вводим PIN, если требуется
  if (strlen(simPIN) > 0) {
    if (!modem.simUnlock(simPIN)) {
      SerialMon.println("SIM PIN unlock failed");
      while (true);
    }
  }

  // Подключение к GPRS
  SerialMon.print("Connecting to network...");
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
    while (true);
  }
  SerialMon.println(" success");

  // Выводим уровень сигнала
  int16_t rssi = modem.getSignalQuality();
  SerialMon.print("Уровень сигнала (RSSI): ");
  SerialMon.println(rssi);

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
      // String text = "SMS от: " + phoneNumber + "\n" + message;
      // sendToTelegram(text);
    }
  }
}

