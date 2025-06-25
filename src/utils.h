#ifndef UCS2_UTILS_H
#define UCS2_UTILS_H
#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include <TinyGsmClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <Preferences.h>

// Объявляем глобальные переменные, которые определены в main.cpp
extern TinyGsm modem;
extern HttpClient http;
extern HttpClient httpsClient;
extern WiFiClientSecure wifiClientSecure;
extern String imei;
// extern const char* ssid;
// extern const char* password;
#define SerialMon Serial

extern Preferences preferences;
extern WebServer serverWeb;

// Прототипы функций
String decodeUCS2(const String& ucs2);
String formatSmsDatetime(const String& raw);
void modemPowerOn();
void sendDataOverHttp(const String& phone, const String& message, const String& datetime);
void sendDataOverHttpWifi(const String& phone, const String& message, const String& datetime);
void sendSMS(const String& phone, const String& message);
void readAllUnreadSMS();
bool sendPing();
void setup_wifi();
void sendToTelegram(const String& text);
void startAPMode();
bool loadWiFiSettings(String &ssid, String &pass);
bool loadTelegramSettings(String &token, String &chat_id);
void saveTelegramSettings(const String &token, const String &chat_id);
void startWorkModeWeb();
extern String getModemStatusJson();
extern String getLogBufferText();

// HTML-форма теперь шаблон с плейсхолдером %WIFI_OPTIONS%
const char htmlForm[] = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>WiFi и Telegram Настройки</title>
    <style>
        body {
            background: #f7f7f7;
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            font-family: 'Consolas', 'Menlo', 'Monaco', 'Liberation Mono', monospace;
        }
        .container {
            background: #fff;
            padding: 32px 18px 24px 18px;
            border-radius: 12px;
            box-shadow: 0 2px 16px rgba(0,0,0,0.07);
            width: 100%;
            text-align: center;
        }
        h2 { text-align: center; margin-bottom: 16px; font-size: 1.5em; font-weight: 600; }
        .desc { font-size: 0.75em; color: #444; margin-bottom: 18px; text-align: center; }
        .desc-warning { font-size: 0.75em; color: rgb(255, 0, 0); margin-bottom: 18px; text-align: center; }
        .link { font-size: 0.75em; margin-top: 10px; margin-bottom: 0; text-align: center; }
        label { display: block; margin-bottom: 6px; font-size: 1.08em; }
        select, input[type="text"], input[type="password"], textarea, .password-row {
            margin-bottom: 18px;
            font-family: 'Consolas', 'Menlo', 'Monaco', 'Liberation Mono', monospace;
        }
        select, input[type="text"], input[type="password"], textarea {
            width: 100%;
            padding: 8px 10px;
            border: 1px solid #ccc;
            border-radius: 7px;
            font-size: 1em;
            box-sizing: border-box;
            font-family: 'Consolas', 'Menlo', 'Monaco', 'Liberation Mono', monospace;
        }
        textarea { resize: none; min-height: 2.7em; max-height: 5em; }
        .password-row { display: flex; align-items: center; }
        .password-row input[type="password"], .password-row input[type="text"] { flex: 1; margin-bottom: 0; }
        .password-row button { margin-left: 8px; padding: 0 10px; border: none; background: none; cursor: pointer; }
        .password-row svg { width: 28px; height: 28px; fill: #888; transition: fill 0.2s; }
        .password-row button:hover svg { fill: #1976d2; }
        input[type="submit"] {
            width: 100%;
            padding: 8px 0;
            background: #1976d2;
            color: #fff;
            border: none;
            border-radius: 7px;
            font-size: 1em;
            font-weight: 600;
            cursor: pointer;
            transition: background 0.2s;
            margin-bottom: 22px;
        }
        input[type="submit"]:hover { background: #1256a3; }
        @media (max-width: 600px) {
            html, body {
                height: 100vh;
                min-height: 100vh;
                margin: 0;
                padding: 0;
            }
            body {
                display: block;
            }
            .container {
                width: 100vw;
                min-width: unset;
                max-width: unset;
                margin: 0;
                border-radius: 0;
                box-sizing: border-box;
                height: 100vh;
                padding: 18px 6vw;
                overflow-y: auto;
                display: flex;
                flex-direction: column;
                justify-content: flex-start;
            }
        }
    </style>
    <script>
    function onSSIDChange(sel) {
        var custom = document.getElementById('custom_ssid');
        if(sel.value === '__custom__') { custom.style.display='block'; custom.required=true; } else { custom.style.display='none'; custom.required=false; }
    }
    function togglePassword() {
        var pass = document.getElementById('password');
        var eye = document.getElementById('eyeIcon');
        if (pass.type === 'password') {
            pass.type = 'text';
            eye.innerHTML = '<svg viewBox="0 0 24 24"><path d="M12 5c-7 0-10 7-10 7s3 7 10 7 10-7 10-7-3-7-10-7zm0 12c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.65 0-3 1.35-3 3s1.35 3 3 3 3-1.35 3-3-1.35-3-3-3z"/></svg>';
        } else {
            pass.type = 'password';
            eye.innerHTML = '<svg viewBox="0 0 24 24"><path d="M12 5c-7 0-10 7-10 7s3 7 10 7 10-7 10-7-3-7-10-7zm0 12c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.65 0-3 1.35-3 3s1.35 3 3 3 3-1.35 3-3-1.35-3-3-3z"/><line x1="4" y1="4" x2="20" y2="20" stroke="#888" stroke-width="2"/></svg>';
        }
    }
    </script>
</head>
<body>
    <div class="container">
        <h2>SMS2TG-LILYGO</h2>
        <div class="desc">
           Привет! Чтобы устройство могло передавать SMS в Telegram, необходимо ввести данные WiFi-сети и Telegram.
        </div>
        <form action="/save" method="POST">
            <label for="ssid">WiFi сеть:</label>
            <select name="ssid_select" id="ssid_select" onchange="onSSIDChange(this)">
                %WIFI_OPTIONS%
            </select>
            <input name="ssid" id="custom_ssid" type="text" style="display:none" placeholder="Введите SSID вручную" minlength="2" maxlength="32" pattern="[A-Za-z0-9_\-]+" />
            <label for="password">Пароль WiFi:</label>
            <div class="password-row">
                <input name="password" id="password" type="password" required minlength="8" maxlength="64" />
                <button type="button" onclick="togglePassword()" tabindex="-1"><span id="eyeIcon"><svg viewBox="0 0 24 24"><path d="M12 5c-7 0-10 7-10 7s3 7 10 7 10-7 10-7-3-7-10-7zm0 12c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.65 0-3 1.35-3 3s1.35 3 3 3 3-1.35 3-3-1.35-3-3-3z"/></svg></span></button>
            </div>
            <label for="token">Telegram Token:</label>
            <input name="token" id="token" type="text" required minlength="30" maxlength="60" pattern="[0-9]+:[A-Za-z0-9_-]+" />
            <label for="chat_id">Chat ID:</label>
            <input name="chat_id" id="chat_id" type="text" required pattern="-?[0-9]+" />
            <div class="desc-warning">
              После сохранения устройство перезагрузится и попробует подключиться к WiFi. Если подключение не удастся, точка доступа появится снова.
            </div>
            <input type="submit" value="Сохранить" />
            <div class="link">
              Ссылка на проект: <a href="https://github.com/myroom/SMS2TG-LILYGO" target="_blank">https://github.com/myroom/SMS2TG-LILYGO</a>
            </div>
            
        </form>
    </div>
</body>
</html>
)rawliteral";

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

// Включение модема
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

// Отправка SMS
void sendSMS(const String& phone, const String& message) {
  SerialMon.print("Sending SMS to number " + phone + "...");
  if (modem.sendSMS(phone, message)) {
    SerialMon.println(" SMS sent successfully!");
  } else {
    SerialMon.println(" SMS sending failed");
  }
}

// Чтение всех непрочитанных SMS
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

void sendToTelegram(const String& text) {
  if (WiFi.status() != WL_CONNECTED) {
    SerialMon.println("WiFi не подключён, отправка в Telegram невозможна.");
    return;
  }
  String token, chat_id;
  if (!loadTelegramSettings(token, chat_id)) {
    SerialMon.println("Не заданы Telegram token или chat_id!");
    return;
  }
  const char telegramServer[] = "api.telegram.org";
  const int telegramPort = 443;
  WiFiClientSecure telegramClient;
  telegramClient.setInsecure();
  HttpClient telegramHttp(telegramClient, telegramServer, telegramPort);

  String url = "/bot" + token + "/sendMessage";
  String postData = "chat_id=" + chat_id + "&text=" + text;
  String contentType = "application/x-www-form-urlencoded";

  telegramHttp.post(url, contentType, postData);
  int statusCode = telegramHttp.responseStatusCode();
  String response = telegramHttp.responseBody();
  SerialMon.print("Статус HTTP Telegram: ");
  SerialMon.println(statusCode);
  SerialMon.print("Ответ Telegram: ");
  SerialMon.println(response);
  telegramHttp.stop();
}

void startAPMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("SMS2TG-SETUP");
  IPAddress IP = WiFi.softAPIP();
  SerialMon.print("IP-адрес точки доступа: ");
  SerialMon.println(IP);

  // Сканируем WiFi-сети
  int n = WiFi.scanNetworks();
  String wifiOptions = "";
  for (int i = 0; i < n; ++i) {
    String ssid = WiFi.SSID(i);
    wifiOptions += "<option value=\"" + ssid + "\">" + ssid + "</option>";
  }
  wifiOptions += "<option value=\"__custom__\">Другая сеть...</option>";

  String page = String(htmlForm);
  page.replace("%WIFI_OPTIONS%", wifiOptions);

  serverWeb.on("/", HTTP_GET, [page]() mutable {
    serverWeb.send(200, "text/html", page);
  });
  serverWeb.on("/save", HTTP_POST, []() {
    String ssid = serverWeb.arg("ssid_select");
    if (ssid == "__custom__") ssid = serverWeb.arg("ssid");
    String pass = serverWeb.arg("password");
    String token = serverWeb.arg("token");
    String chat_id = serverWeb.arg("chat_id");
    if (ssid.length() > 0 && pass.length() > 0 && token.length() > 0 && chat_id.length() > 0) {
        preferences.begin("wifi", false);
        preferences.putString("ssid", ssid);
        preferences.putString("password", pass);
        preferences.end();
        saveTelegramSettings(token, chat_id);
        serverWeb.send(200, "text/html; charset=utf-8",
            "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Сохранено</title></head>"
            "<body style='display:flex;align-items:center;justify-content:center;height:100vh;margin:0;'>"
            "<div style='text-align:center;font-size:1.3em;'>✅ Сохранено!<br>Перезагрузка...</div>"
            "</body></html>");
        delay(1500);
        ESP.restart();
    } else {
        serverWeb.send(200, "text/html; charset=utf-8",
            "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Ошибка</title></head>"
            "<body style='display:flex;align-items:center;justify-content:center;height:100vh;margin:0;'>"
            "<div style='text-align:center;color:red;font-size:1.2em;'>Ошибка: заполните все поля</div>"
            "</body></html>");
    }
  });
  serverWeb.onNotFound([]() {
    serverWeb.send(404, "text/html", "<b>Страница не найдена</b>");
  });
  serverWeb.begin();
  SerialMon.println("Web-интерфейс настроек запущен");
}

bool loadWiFiSettings(String &ssid, String &pass) {
  preferences.begin("wifi", true);
  ssid = preferences.getString("ssid", "");
  pass = preferences.getString("password", "");
  preferences.end();
  return (ssid.length() > 0 && pass.length() > 0);
}

bool loadTelegramSettings(String &token, String &chat_id) {
  preferences.begin("telegram", true);
  token = preferences.getString("token", "");
  chat_id = preferences.getString("chat_id", "");
  preferences.end();
  return (token.length() > 0 && chat_id.length() > 0);
}

void saveTelegramSettings(const String &token, const String &chat_id) {
  preferences.begin("telegram", false);
  preferences.putString("token", token);
  preferences.putString("chat_id", chat_id);
  preferences.end();
}

const char workHtml[] = R"rawliteral(
    <!DOCTYPE html>
    <html lang="ru">
    <head>
        <meta charset="UTF-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1.0" />
        <title>Рабочий режим SMS2TG-LILYGO</title>
        <style>
            body {
                background: #f7f7f7;
                min-height: 100vh;
                display: flex;
                align-items: center;
                justify-content: center;
                font-family: 'Consolas', 'Menlo', 'Monaco', 'Liberation Mono', monospace;
            }
            .container {
                background: #fff;
                padding: 32px 18px 24px 18px;
                border-radius: 12px;
                box-shadow: 0 2px 16px rgba(0,0,0,0.07);
                width: 100%;
                max-width: 420px;
                margin: 40px auto 0 auto;
                text-align: center;
            }
            h2 { text-align: center; margin-bottom: 16px; font-size: 1.5em; font-weight: 600; }
            .desc { font-size: 0.95em; color: #444; margin-bottom: 18px; text-align: center; }
            .status { font-size: 1em; margin-bottom: 12px; color: #1976d2; }
            .logbox { background: #222; color: #eee; font-size: 0.95em; text-align: left; border-radius: 7px; padding: 10px; min-height: 120px; max-height: 220px; overflow-y: auto; margin-bottom: 18px; font-family: 'Consolas', 'Menlo', 'Monaco', 'Liberation Mono', monospace; }
            form { margin-bottom: 0; }
            button {
                width: 100%;
                padding: 8px 0;
                background: #d32f2f;
                color: #fff;
                border: none;
                border-radius: 7px;
                font-size: 1em;
                font-weight: 600;
                cursor: pointer;
                transition: background 0.2s;
                margin-bottom: 0;
            }
            button:hover { background: #b71c1c; }
            @media (max-width: 600px) {
                html, body {
                    height: 100vh;
                    min-height: 100vh;
                    margin: 0;
                    padding: 0;
                }
                body {
                    display: block;
                }
                .container {
                    width: 100vw;
                    min-width: unset;
                    max-width: unset;
                    margin: 0;
                    border-radius: 0;
                    box-sizing: border-box;
                    height: 100vh;
                    padding: 18px 6vw;
                    overflow-y: auto;
                    display: flex;
                    flex-direction: column;
                    justify-content: flex-start;
                }
            }
        </style>
        <script>
        function updateStatus() {
            fetch('/modem_status').then(r => r.json()).then(data => {
                document.getElementById('modem_status').innerText = 'Статус модема: ' + data.state + (data.ok ? ' (готов)' : '');
            });
        }
        function updateLog() {
            fetch('/modem_log').then(r => r.text()).then(txt => {
                let logbox = document.getElementById('logbox');
                logbox.innerText = txt;
                logbox.scrollTop = logbox.scrollHeight;
            });
        }
        setInterval(updateStatus, 1500);
        setInterval(updateLog, 1200);
        window.onload = function() { updateStatus(); updateLog(); };
        </script>
    </head>
    <body>
        <div class="container">
            <h2>Рабочий режим</h2>
            <div class="desc">Устройство работает. Для сброса настроек нажмите кнопку ниже.</div>
            <div id="modem_status" class="status">Статус модема: ...</div>
            <div id="logbox" class="logbox">Логи загружаются...</div>
            <form action="/reset" method="POST">
                <button type="submit">Сбросить все настройки</button>
            </form>
        </div>
    </body>
    </html>
    )rawliteral";

void startWorkModeWeb() {
    serverWeb.on("/", HTTP_GET, []() {
        serverWeb.send(200, "text/html; charset=utf-8", workHtml);
    });

    serverWeb.on("/modem_status", HTTP_GET, []() {
        String json = getModemStatusJson();
        serverWeb.send(200, "application/json; charset=utf-8", json);
    });
    serverWeb.on("/modem_log", HTTP_GET, []() {
        String log = getLogBufferText();
        serverWeb.send(200, "text/plain; charset=utf-8", log);
    });

    serverWeb.on("/reset", HTTP_POST, []() {
        preferences.begin("wifi", false);
        preferences.clear();
        preferences.end();
        preferences.begin("telegram", false);
        preferences.clear();
        preferences.end();
        serverWeb.send(200, "text/html; charset=utf-8",
            "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Сброс</title></head>"
            "<body style='display:flex;align-items:center;justify-content:center;height:100vh;margin:0;'>"
            "<div style='text-align:center;font-size:1.3em;'>✅ Настройки сброшены!<br>Перезагрузка...</div>"
            "</body></html>");
        delay(1500);
        ESP.restart();
    });

    serverWeb.onNotFound([]() {
        serverWeb.send(404, "text/html; charset=utf-8", "<b>Страница не найдена</b>");
    });

    serverWeb.begin();
    SerialMon.println("Web-интерфейс рабочего режима запущен");
}

#endif // UCS2_UTILS_H 