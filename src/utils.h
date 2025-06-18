#ifndef UCS2_UTILS_H
#define UCS2_UTILS_H
#include <Arduino.h>

// Корректная конвертация UCS2 (HEX) в String (UTF-8)
String decodeUCS2(const String& ucs2) {
  String result;
  for (int i = 0; i < ucs2.length(); i += 4) {
    String hexChar = ucs2.substring(i, i + 4);
    uint16_t unicode = (uint16_t)strtol(hexChar.c_str(), NULL, 16);
    char buf[5] = {0};
    if (unicode < 0x80) {
      buf[0] = (char)unicode;
    } else if (unicode < 0x800) {
      buf[0] = 0xC0 | (unicode >> 6);
      buf[1] = 0x80 | (unicode & 0x3F);
    } else {
      buf[0] = 0xE0 | (unicode >> 12);
      buf[1] = 0x80 | ((unicode >> 6) & 0x3F);
      buf[2] = 0x80 | (unicode & 0x3F);
    }
    result += String(buf);
  }
  return result;
}

// Форматирование даты/времени из SMS в читаемый вид
String formatSmsDatetime(const String& raw) {
  // Пример входа: 25/06/18,02:59:35+12
  if (raw.length() < 17) return raw;
  String day = raw.substring(0, 2);
  String month = raw.substring(3, 5);
  String year = raw.substring(6, 8);
  String hour = raw.substring(9, 11);
  String min = raw.substring(12, 14);
  String sec = raw.substring(15, 17);
  String tz = raw.substring(17); // +12
  String formatted = day + "." + month + ".20" + year + " " + hour + ":" + min + ":" + sec + " " + tz;
  return formatted;
}

#endif // UCS2_UTILS_H 