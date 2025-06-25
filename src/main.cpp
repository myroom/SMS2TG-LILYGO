// GPRS настройки
const char apn[]  = "internet"; // Замените на APN вашего оператора
const char user[] = "";
const char pass[] = "";

// Фиктивные значения для server и port, чтобы не было ошибки компиляции
const char server[] = "192.168.4.1";
const int port = 80;

#include <Arduino.h>
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


TinyGsm modem(SerialAT);

// Инициализация GSM и HTTP клиентов
TinyGsmClient client(modem);
HttpClient http(client, server, port);

#include "utils.h"

Preferences preferences;
WebServer serverWeb(80);

String imei = "";
WiFiClientSecure wifiClientSecure;

// --- Асинхронная state machine для инициализации модема и GSM ---
enum ModemInitState {
  INIT_IDLE,
  INIT_POWER_ON,
  INIT_SERIAL,
  INIT_AT_CMGF,
  INIT_AT_CNMI,
  INIT_RESTART,
  INIT_WAIT_NET,
  INIT_CHECK_NET,
  INIT_READ_SMS,
  INIT_DONE,
  INIT_FAIL
};
ModemInitState modemState = INIT_IDLE;
unsigned long modemTimer = 0;
bool modemInitOk = false;

// Счётчик неудачных попыток инициализации модема
int modemFailCount = 0;
const int MODEM_MAX_FAILS = 5;

bool isSetupMode = false;

// --- Получение текущей даты и времени (простая версия через millis, если нет RTC) ---
String getTimeStamp() {
  time_t now = time(nullptr);
  struct tm *tm_info = localtime(&now);
  char buf[24];
  if (now > 100000) {
    strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M:%S ", tm_info);
    return String(buf);
  } else {
    // Если время не установлено, используем дату 01.01.1970 и текущее время по millis
    unsigned long ms = millis() / 1000;
    unsigned long h = ms / 3600;
    unsigned long m = (ms % 3600) / 60;
    unsigned long s = ms % 60;
    snprintf(buf, sizeof(buf), "01.01.1970 %02lu:%02lu:%02lu ", h, m, s);
    return String(buf);
  }
}

// --- Буфер логов для web-интерфейса ---
#define LOG_BUFFER_SIZE 20
String logBuffer[LOG_BUFFER_SIZE];
int logBufferPos = 0;

void logPrint(const String& msg) {
  String line = getTimeStamp() + msg + "\n";
  SerialMon.print(line);
  logBuffer[logBufferPos] = line;
  logBufferPos = (logBufferPos + 1) % LOG_BUFFER_SIZE;
}
void logPrintln(const String& msg) {
  String line = getTimeStamp() + msg + "\n";
  SerialMon.print(line);
  logBuffer[logBufferPos] = line;
  logBufferPos = (logBufferPos + 1) % LOG_BUFFER_SIZE;
}

// --- Функция для получения состояния модема (state machine) ---
const char* getModemStateName(ModemInitState state) {
  switch(state) {
    case INIT_IDLE: return "Ожидание";
    case INIT_POWER_ON: return "Включение модема";
    case INIT_SERIAL: return "Инициализация Serial";
    case INIT_AT_CMGF: return "AT+CMGF=1";
    case INIT_AT_CNMI: return "AT+CNMI=2,1,0,0,0";
    case INIT_RESTART: return "Перезапуск модема";
    case INIT_WAIT_NET: return "Ожидание GSM-сети";
    case INIT_CHECK_NET: return "Проверка сети";
    case INIT_READ_SMS: return "Чтение SMS";
    case INIT_DONE: return "Готово";
    case INIT_FAIL: return "Ошибка";
    default: return "?";
  }
}

// --- Делаем функции видимыми для других файлов ---
String getModemStatusJson() {
  String json = "{\"state\":\"";
  json += getModemStateName(modemState);
  json += "\",\"ok\":";
  json += modemInitOk ? "true" : "false";
  json += "}";
  return json;
}

String getLogBufferText() {
  String out;
  int pos = logBufferPos;
  for (int i = 0; i < LOG_BUFFER_SIZE; ++i) {
    int idx = (pos + i) % LOG_BUFFER_SIZE;
    if (logBuffer[idx].length() > 0) out += logBuffer[idx];
  }
  return out;
}

void asyncModemInit() {
  switch (modemState) {
    case INIT_IDLE:
      modemPowerOn();
      modemTimer = millis();
      modemState = INIT_POWER_ON;
      logPrint("Модем: Power ON...");
      break;
    case INIT_POWER_ON:
      if (millis() - modemTimer > 500) {
        SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
        modemTimer = millis();
        modemState = INIT_SERIAL;
        logPrint("SerialAT запущен...");
      }
      break;
    case INIT_SERIAL:
      if (millis() - modemTimer > 3000) {
        SerialAT.println("AT+CMGF=1");
        modemTimer = millis();
        modemState = INIT_AT_CMGF;
        logPrint("AT+CMGF=1...");
      }
      break;
    case INIT_AT_CMGF:
      if (millis() - modemTimer > 500) {
        SerialAT.println("AT+CNMI=2,1,0,0,0");
        modemTimer = millis();
        modemState = INIT_AT_CNMI;
        logPrint("AT+CNMI=2,1,0,0,0...");
      }
      break;
    case INIT_AT_CNMI:
      if (millis() - modemTimer > 500) {
        logPrint("Инициализация модема...");
        modem.restart();
        modemTimer = millis();
        modemState = INIT_RESTART;
      }
      break;
    case INIT_RESTART:
      if (millis() - modemTimer > 1000) {
        logPrint("Ожидание GSM-сети (до 2 минут)...");
        modemTimer = millis();
        modemState = INIT_WAIT_NET;
      }
      break;
    case INIT_WAIT_NET: {
      unsigned long waitStart = millis();
      bool netOk = modem.waitForNetwork(); // Ждём столько, сколько нужно (или внутренний таймаут TinyGSM)
      if (netOk) {
        logPrintln("Сеть найдена");
        modemState = INIT_CHECK_NET;
      } else if (millis() - modemTimer > 120000) { // 120 сек таймаут
        logPrintln("Не удалось подключиться к сети GSM");
        modemState = INIT_FAIL;
      } else {
        delay(500);
      }
      break;
    }
    case INIT_CHECK_NET:
      if (modem.isNetworkConnected()) {
        logPrintln("Успешное подключение к сети GSM");
        modemState = INIT_READ_SMS;
      } else {
        logPrintln("Ошибка подключения к сети GSM");
        modemState = INIT_FAIL;
      }
      break;
    case INIT_READ_SMS:
      readAllUnreadSMS();
      logPrintln("Ожидание входящих SMS...");
      modemInitOk = true;
      modemState = INIT_DONE;
      break;
    case INIT_DONE:
      modemFailCount = 0; // сбрасываем счётчик при успешной инициализации
      break;
    case INIT_FAIL:
      modemFailCount++;
      logPrintln("Ошибка инициализации модема. Перезапуск модема...");
      delay(1000);
      modemPowerOn();
      if (modemFailCount >= MODEM_MAX_FAILS) {
        logPrintln("Модем не восстановлен после " + String(MODEM_MAX_FAILS) + " попыток. Перезагрузка устройства...");
        delay(2000);
        ESP.restart();
      } else {
        modemState = INIT_IDLE;
      }
      break;
  }
}

void setup() {
  SerialMon.begin(115200);
  delay(10);

  // Загружаем настройки WiFi и Telegram
  String savedSsid, savedPass;
  String token, chat_id;
  if (!loadWiFiSettings(savedSsid, savedPass) || !loadTelegramSettings(token, chat_id)) {
    logPrintln("WiFi или Telegram не настроены.");
    logPrintln("Запуск точки доступа и web-интерфейса...");
    startAPMode();  // Запуск точки доступа и web-интерфейса
    isSetupMode = true;
    return;
  }

  // Если настройки есть — подключаемся к WiFi
  logPrint("Подключение к WiFi: ");
  logPrintln(savedSsid);
  WiFi.begin(savedSsid.c_str(), savedPass.c_str());
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    logPrintln("WiFi подключён, IP-адрес: " + WiFi.localIP().toString());
    wifiClientSecure.setInsecure();

    // --- NTP инициализация времени ---
    configTzTime("MSK-3", "pool.ntp.org", "time.nist.gov"); // Europe/Moscow
    logPrintln("Ожидание синхронизации времени через NTP...");
    time_t now = time(nullptr);
    int ntpWait = 0;
    while (now < 1672531200 && ntpWait < 20) { // 1672531200 = 01.01.2023
      delay(500);
      now = time(nullptr);
      ntpWait++;
    }
    if (now >= 1672531200) {
      char buf[32];
      struct tm *tm_info = localtime(&now);
      strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M:%S", tm_info);
      logPrintln("Время синхронизировано: " + String(buf));
    } else {
      logPrintln("Не удалось синхронизировать время через NTP");
    }
    // --- конец NTP ---

    // Запуск web-интерфейса рабочего режима
    startWorkModeWeb();
  } else {
    logPrintln("\nОшибка подключения к WiFi, перезагрузка...");
    delay(2000);
    ESP.restart();
  }

  // Запускаем асинхронную инициализацию модема
  modemState = INIT_IDLE;
  modemInitOk = false;
}

void loop() {
  serverWeb.handleClient(); // web-интерфейс всегда доступен

  if (isSetupMode) {
    // В режиме настроек не работаем с модемом
    return;
  }

  if (!modemInitOk) {
    asyncModemInit();
    return; // пока модем не инициализирован, не обрабатываем SMS
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
      logPrintln("\nНовое SMS!");
      logPrintln("Отправитель: " + sender);
      logPrintln("Дата/время: " + formattedDatetime);
      logPrintln("Текст: " + outText);
      
      // Отправляем SMS в Telegram через WiFi
      sendToTelegram("SMS от: " + sender + "\nВремя: " + formattedDatetime + "\n" + outText);
    }
  }
}