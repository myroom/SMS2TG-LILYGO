// Пример для LilyGO T-Call SIM800: подключение к сети
#include <Arduino.h>

// Определяем пины для LilyGO T-Call
#define MODEM_PWRKEY    4
#define MODEM_POWER_ON  23
#define MODEM_RST       5
#define LED_GPIO        12
#define LED_OFF         1

#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER 1024
#include <TinyGsmClient.h>
#include "utilities.h"

#define SerialMon Serial
#define SerialAT Serial1
#define MODEM_RX 26
#define MODEM_TX 27

const char apn[] = "internet"; // Замените на APN вашего оператора
const char gprsUser[] = "";
const char gprsPass[] = "";
const char simPIN[] = "";

TinyGsm modem(SerialAT);

void setup() {
  SerialMon.begin(9600);
  delay(10);

  setupModem(); // Важно для питания модема

  SerialAT.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  SerialMon.println("Инициализация модема...");
  modem.restart();

  if (strlen(simPIN) > 0) {
    if (!modem.simUnlock(simPIN)) {
      SerialMon.println("Ошибка PIN SIM-карты");
      while (true);
    }
  }

  SerialMon.print("Подключение к сети...");
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
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" Ошибка GPRS");
    while (true);
  }
  SerialMon.println(" GPRS подключен");
}

void loop() {
  // ничего не делаем
}
