// GPRS настройки
const char apn[]  = "internet"; // Замените на APN вашего оператора
const char user[] = "";
const char pass[] = "";

#include <Arduino.h>
#include <StreamDebugger.h>
#include "utils.h"

// Устанавливаем порт для отладки (к Serial Monitor, по умолчанию скорость 115200)
#define SerialMon Serial

// Устанавливаем порт для AT команд (к модулю SIM800)
#define SerialAT Serial1

// TTGO T-Call пины
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26

// Настройка TinyGSM библиотеки
#define TINY_GSM_MODEM_SIM800      // Модем SIM800
#define TINY_GSM_RX_BUFFER   1024  // Устанавливаем буфер RX на 1Кб
//#define DUMP_AT_COMMANDS // для отладки AT команд

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, SerialMon);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif

// Инициализация GSM клиента
TinyGsmClient client(modem);
HttpClient http(client, "api.telegram.org", 80);

// Параметры Telegram (имя бота @sms2tg_direct_bot)
const char botToken[] = "7527301010:AAE3p_jGIoisPx3h4v1txYbXJDVmwGR1NF0"; // <-- Вставьте сюда токен вашего бота
const char chatId[] = "-4676572107";     // <-- Вставьте сюда ваш chat_id

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
  SerialMon.print("Telegram HTTP статус: ");
  SerialMon.println(statusCode);
  SerialMon.print("Ответ Telegram: ");
  SerialMon.println(response);
}

void sendSMS(const String& phone, const String& message) {
  SerialMon.print("Отправка SMS на номер " + phone + "...");
  if (modem.sendSMS(phone, message)) {
    SerialMon.println(" SMS отправлено успешно!");
  } else {
    SerialMon.println(" Ошибка отправки SMS");
  }
}

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

void setup() {
  SerialMon.begin(115200);
  delay(10);

  modemPowerOn();

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  SerialAT.println("AT+CMGF=1"); // Текстовый режим SMS
  delay(500);
  SerialAT.println("AT+CNMI=2,1,0,0,0"); // Уведомления о новых SMS (CMTI)
  delay(500);

  SerialMon.println("Модем инициализируется...");
  modem.restart();

  SerialMon.print("Ожидание сети...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" Нет сети");
    while (true);
  }
  SerialMon.println(" Сеть найдена");

  if (modem.isNetworkConnected()) {
    SerialMon.println("Успешно подключено к сети!");
  } else {
    SerialMon.println("Ошибка подключения к сети");
    while (true);
  }

  SerialMon.print("Подключение к GPRS...");
  if (!modem.gprsConnect(apn, user, pass)) {
    SerialMon.println(" Ошибка GPRS");
    while (true);
  }
  SerialMon.println(" GPRS подключен");

  readAllUnreadSMS();

   // Отправка тестового SMS
  //sendSMS("+359899041106", "Устройство TTGO T-Call готово к работе!");

  SerialMon.println("Ожидаем входящий SMS...");

 

  // Отправка тестового сообщения в Telegram
  //SerialMon.print("Отправка сообщения в Telegram...");
  //sendToTelegram("Test from TTGO T-Call via GPRS"); // Вызов закомментирован
}

void loop() {
  if (SerialAT.available()) {
    String line = SerialAT.readStringUntil('\n');
    line.trim();

    // Обработка входящего звонка (больше не выводим)
    // if (line == "RING") {
    //   SerialMon.println("Входящий звонок!");
    // } else if (line.startsWith("+CLIP:")) {
    //   int firstQuote = line.indexOf('"');
    //   int secondQuote = line.indexOf('"', firstQuote + 1);
    //   String caller = (firstQuote != -1 && secondQuote != -1) ? line.substring(firstQuote + 1, secondQuote) : "";
    //   SerialMon.println("Номер звонящего: " + caller);
    // }

    // Если пришло уведомление о новом SMS
    if (line.startsWith("+CMTI:")) {
      int idx1 = line.indexOf(',');
      if (idx1 != -1) {
        int smsIndex = line.substring(idx1 + 1).toInt();
        SerialAT.print("AT+CMGR=");
        SerialAT.println(smsIndex);
        delay(300);
      }
    }
    // Если пришёл ответ на AT+CMGR (содержимое SMS)
    else if (line.startsWith("+CMGR:")) {
      String smsHeader = line;
      String smsText = SerialAT.readStringUntil('\n');
      smsText.trim();

      int q[10], qn = 0, pos = -1;
      while ((pos = smsHeader.indexOf('"', pos + 1)) != -1 && qn < 10) {
        q[qn++] = pos;
      }
      String sender = (qn >= 4) ? smsHeader.substring(q[2] + 1, q[3]) : "";
      String datetime = (qn >= 8) ? smsHeader.substring(q[6] + 1, q[7]) : "";

      // Декодируем UCS2, если в тексте только HEX-цифры
      bool isUCS2 = true;
      if (smsText.length() > 0 && smsText.length() % 4 == 0) {
        for (int i = 0; i < smsText.length(); ++i) {
          char c = smsText.charAt(i);
          if (!isxdigit(c)) { isUCS2 = false; break; }
        }
      } else {
        isUCS2 = false;
      }
      String outText;
      if (isUCS2) {
        outText = decodeUCS2(smsText);
      } else {
        if (smsText.startsWith("\"") && smsText.endsWith("\"") && smsText.length() > 1) {
          smsText = smsText.substring(1, smsText.length() - 1);
        }
        outText = smsText;
      }
      SerialMon.println("\nНовое SMS!");
      SerialMon.println("Отправитель: " + sender);
      SerialMon.println("Дата/время: " + formatSmsDatetime(datetime));
      SerialMon.println("Текст: " + outText);
    }
  }
}