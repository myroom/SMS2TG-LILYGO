#include <Arduino.h>
#define SerialMon Serial
#define SerialAT Serial1

// Пины для TTGO T-Call
#define MODEM_RST      5
#define MODEM_PWKEY    4
#define MODEM_POWER_ON 23
#define MODEM_TX       27
#define MODEM_RX       26

#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>

const char apn[]  = "internet"; // APN вашего оператора
const char user[] = "";
const char pass[] = "";

TinyGsm modem(SerialAT);

void modemPowerOn() {
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);

  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);

  delay(1000);
  digitalWrite(MODEM_PWKEY, HIGH);
  delay(2000);
  digitalWrite(MODEM_PWKEY, LOW);
}

void setup() {
  SerialMon.begin(115200);
  delay(10);

  modemPowerOn();

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  SerialMon.println("Инициализация модема...");
  modem.restart();

  SerialMon.print("Ожидание сети...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" Нет сети!");
    while (true);
  }
  SerialMon.println(" Сеть найдена!");

  if (modem.isNetworkConnected()) {
    SerialMon.println("Успешно подключено к сети GSM!");
  } else {
    SerialMon.println("Ошибка подключения к сети!");
    while (true);
  }

  SerialMon.print("Подключение к GPRS...");
  if (!modem.gprsConnect(apn, user, pass)) {
    SerialMon.println(" Ошибка GPRS!");
    while (true);
  }
  SerialMon.println(" GPRS подключён!");
}

void loop() {
  // Здесь можно добавить отправку/приём данных через GSM
}
