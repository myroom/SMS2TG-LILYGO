// GPRS настройки
const char apn[]  = "internet"; // Замените на APN вашего оператора
const char user[] = "";
const char pass[] = "";

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

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, SerialMon);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif

// Инициализация GSM клиента
TinyGsmClient client(modem);

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

void setup() {
  SerialMon.begin(115200);
  delay(10);

  modemPowerOn();

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

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

  // Отправка тестового SMS
  SerialMon.print("Отправка SMS...");
  if (modem.sendSMS("+34628485623", "Test SMS from TTGO T-Call!")) {
    SerialMon.println(" SMS отправлено успешно!");
  } else {
    SerialMon.println(" Ошибка отправки SMS");
  }
}

void loop() {
  // ничего не делаем
}