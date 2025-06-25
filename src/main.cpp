// GPRS настройки
const char apn[]  = "internet"; // Замените на APN вашего оператора
const char user[] = "";
const char pass[] = "";

// Настройки Telegram
const char TELEGRAM_BOT_TOKEN[] = "7527301010:AAE3p_jGIoisPx3h4v1txYbXJDVmwGR1NF0"; // <-- токен
const char TELEGRAM_CHAT_ID[] = "-4676572107";      // <-- chat_id 

// Фиктивные значения для server и port, чтобы не было ошибки компиляции
const char server[] = "192.168.4.1";
const int port = 80;

#include <Arduino.h>
#include <StreamDebugger.h>
#include <WebServer.h>
#include <Preferences.h>
#include <FS.h>
#include <SPIFFS.h>

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
#include <WiFi.h>
#include <WiFiClientSecure.h>

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, SerialMon);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif

// Инициализация GSM и HTTP клиентов
TinyGsmClient client(modem);
HttpClient http(client, server, port);

#include "utils.h"

Preferences preferences;
WebServer serverWeb(80);

String imei = "";
WiFiClientSecure wifiClientSecure;

void setup() {
  SerialMon.begin(115200);
  delay(10);

  // Загружаем настройки WiFi и Telegram
  String savedSsid, savedPass;
  String token, chat_id;
  if (!loadWiFiSettings(savedSsid, savedPass) || !loadTelegramSettings(token, chat_id)) {
    SerialMon.println("WiFi или Telegram не настроены.");
    SerialMon.println("Запуск точки доступа и web-интерфейса...");
    startAPMode();  // Запуск точки доступа и web-интерфейса
    while (true) {
      serverWeb.handleClient();
      delay(10);
    }
  }

  // Если настройки есть — подключаемся к WiFi
  SerialMon.print("Подключение к WiFi: ");
  SerialMon.println(savedSsid);
  WiFi.begin(savedSsid.c_str(), savedPass.c_str());
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    SerialMon.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    SerialMon.println("\nWiFi подключён");
    SerialMon.print("IP-адрес: ");
    SerialMon.println(WiFi.localIP());
    // Insecure, but required for this example
    wifiClientSecure.setInsecure();
  } else {
    SerialMon.println("\nОшибка подключения к WiFi, перезагрузка...");
    delay(2000);
    ESP.restart();
  }

  // Дальнейшая инициализация модема и SMS
  modemPowerOn();
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);
  SerialAT.println("AT+CMGF=1");
  delay(500);
  SerialAT.println("AT+CNMI=2,1,0,0,0");
  delay(500);
  SerialMon.println("Инициализация модема...");
  modem.restart();
  SerialMon.print("Ожидание GSM-сети...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" Нет сети");
    while (true);
  }
  SerialMon.println(" Сеть найдена");
  if (modem.isNetworkConnected()) {
    SerialMon.println("Успешное подключение к сети!");
  } else {
    SerialMon.println("Ошибка подключения к сети");
    while (true);
  }
  readAllUnreadSMS();
  SerialMon.println("Ожидание входящих SMS...");
}

void loop() {
  if (SerialAT.available()) {
    String line = SerialAT.readStringUntil('\n');
    line.trim();

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
      String formattedDatetime = formatSmsDatetime(datetime);

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
      SerialMon.println("Дата/время: " + formattedDatetime);
      SerialMon.println("Текст: " + outText);
      
      // Отправляем SMS в Telegram через WiFi
      sendToTelegram("SMS от: " + sender + "\nВремя: " + formattedDatetime + "\n" + outText);
    }
  }
}