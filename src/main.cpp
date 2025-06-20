// GPRS настройки
const char apn[]  = "internet"; // Замените на APN вашего оператора
const char user[] = "";
const char pass[] = "";

// Адрес вашего сервера
const char server[] = "zerosim.ru"; // <-- Укажите здесь адрес вашего сервера
const int  port = 80;

#include <Arduino.h>
#include <StreamDebugger.h>

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

// Инициализация GSM и HTTP клиентов
TinyGsmClient client(modem);
HttpClient http(client, server, port);

String imei = "";
int pingErrorCount = 0; // Счетчик ошибок ping
const int maxPingErrors = 10; // Максимальное количество ошибок ping до перезагрузки
unsigned long lastTestSend = 0; // Время последней отправки тестового сообщения
const unsigned long testInterval = 60000; // Интервал между тестовыми сообщениями (1 минута)
unsigned long lastPingSend = 0; // Время последней отправки ping
const unsigned long pingInterval = 30000; // Интервал между ping (30 секунд)

#include "utils.h"

void setup() {
  SerialMon.begin(115200);
  delay(10);

  modemPowerOn();

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  // Получаем IMEI модема до инициализации сети
  imei = modem.getIMEI();
  SerialMon.print("Device IMEI: ");
  SerialMon.println(imei);

  // Пример формата сообщения
  //SerialMon.println("Example JSON message:");
  //SerialMon.println("{\n  \"imei\": \"" + imei + "\",\n  \"date\": \"24/01/01,12:34:56+00\",\n  \"phone\": \"+79991234567\",\n  \"message\": \"SMS text\"\n}");

  SerialAT.println("AT+CMGF=1"); // Текстовый режим SMS
  delay(500);
  SerialAT.println("AT+CNMI=2,1,0,0,0"); // Уведомления о новых SMS (CMTI)
  delay(500);

  SerialMon.println("Modem is initializing...");
  modem.restart();

  SerialMon.print("Waiting for GSM network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" No network");
    while (true);
  }
  SerialMon.println(" Network found");

  if (modem.isNetworkConnected()) {
    SerialMon.println("Successfully connected to network!");
  } else {
    SerialMon.println("Network connection error");
    while (true);
  }

  SerialMon.print("Connecting to GPRS...");
  if (!modem.gprsConnect(apn, user, pass)) {
    SerialMon.println(" GPRS error");
    while (true);
  }
  SerialMon.println(" GPRS connected");

  // Получаем IMEI модема
  imei = modem.getIMEI();
  SerialMon.println("IMEI: " + imei);

  readAllUnreadSMS();

  SerialMon.println("Waiting for incoming SMS...");
}

void loop() {
  // Отправка ping раз в 30 секунд
  if (millis() - lastPingSend > pingInterval) {
    lastPingSend = millis();
    sendPing();
  }

  // Отправка тестового сообщения раз в минуту
  if (millis() - lastTestSend > testInterval) {
    lastTestSend = millis();
    String testPhone = "+79991234567";
    String testMessage = "Test message from device";
    String testDatetime = "24/01/01,12:34:56+00";
    sendDataOverHttp(testPhone, testMessage, testDatetime);
  }

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
      SerialMon.println("\nNew SMS!");
      SerialMon.println("Sender: " + sender);
      SerialMon.println("Date/time: " + formattedDatetime);
      SerialMon.println("Text: " + outText);
      
      // Отправляем данные на сервер
      sendDataOverHttp(sender, outText, formattedDatetime);
    }
  }
}