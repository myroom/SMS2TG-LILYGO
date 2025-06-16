#include <Arduino.h>
//#include "utilities.h" // Раскомментируйте, если добавите файл utilities.h для управления питанием

// Пины для SIM800
#define MODEM_RX 26
#define MODEM_TX 27

HardwareSerial SerialAT(1);

void setup() {
  Serial.begin(115200);
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  // Если у вас версия с IP5306, раскомментируйте следующую строку и добавьте файл utilities.h:
  // setupModem();

  SerialAT.println("AT+CMGF=1"); // Текстовый режим SMS
  delay(500);
  SerialAT.println("AT+CNMI=2,2,0,0,0"); // Уведомления о новых SMS
  delay(500);

  Serial.println("Готов к приему SMS");
}

void loop() {
  if (SerialAT.available()) {
    String sms = SerialAT.readString();
    Serial.println("RAW: " + sms); // Для отладки

    if (sms.indexOf("+CMT:") != -1) {
      int start = sms.indexOf("+CMT:") + 5;
      int end = sms.indexOf('"', start);
      String phoneNumber = sms.substring(start + 1, end);

      start = sms.indexOf('"', end + 1) + 2;
      String message = sms.substring(start);

      Serial.println("\nНовое SMS получено!");
      Serial.println("-------------------");
      Serial.println("Отправитель: " + phoneNumber);
      Serial.println("Сообщение: " + message);
      Serial.println("-------------------");
    }
  }
}

