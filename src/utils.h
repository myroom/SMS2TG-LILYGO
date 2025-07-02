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

// Declare global variables defined in main.cpp
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

// Declare logging function from main.cpp
void logPrintln(const String& msg);

#define AP_SSID "SMS2TG-SETUP"
#define MDNS_HOSTNAME "SMS2TG-LILYGO"

// Function prototypes
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

// HTML form template with %WIFI_OPTIONS% placeholder
const char htmlForm[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>SMS2TG-LILYGO (Setup Mode)</title>
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
        .desc { font-size: 0.75em; color: #444; margin-bottom: 18px; text-align: center; }
        .desc-warning { font-size: 0.75em; color: rgb(255, 0, 0); margin-bottom: 18px; text-align: center; }
        .link { font-size: 0.75em; margin-top: 10px; margin-bottom: 0; text-align: center; }
        label { display: block; margin-bottom: 6px; font-size: 1.08em; text-align: left; }
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
        .help-button {
            display: inline-block;
            margin-left: 8px;
            padding: 4px 8px;
            background: #e3f2fd;
            color: #1976d2;
            border: 1px solid #bbdefb;
            border-radius: 4px;
            font-size: 0.85em;
            cursor: pointer;
            text-decoration: none;
            transition: all 0.2s;
        }
        .help-button:hover { background: #bbdefb; }
        .modal {
            display: none;
            position: fixed;
            z-index: 1000;
            left: 0;
            top: 0;
            width: 100%;
            height: 100%;
            background-color: rgba(0,0,0,0.5);
        }
        .modal-content {
            background-color: #fefefe;
            margin: 5% auto;
            padding: 20px;
            border-radius: 8px;
            width: 90%;
            max-width: 600px;
            max-height: 80vh;
            overflow-y: auto;
            position: relative;
        }
        .close {
            color: #aaa;
            float: right;
            font-size: 28px;
            font-weight: bold;
            position: absolute;
            right: 15px;
            top: 10px;
            cursor: pointer;
        }
        .close:hover { color: #000; }
        .modal h3 { margin-top: 0; color: #1976d2; }
        .modal ol { padding-left: 20px; }
        .modal li { margin-bottom: 8px; line-height: 1.4; }
        .modal code { background: #f5f5f5; padding: 2px 4px; border-radius: 3px; font-family: monospace; }
        .modal .note { background: #fff3cd; border: 1px solid #ffeaa7; border-radius: 4px; padding: 10px; margin: 10px 0; color: #856404; }
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
    function showHelp() {
        document.getElementById('helpModal').style.display = 'block';
    }
    function closeHelp() {
        document.getElementById('helpModal').style.display = 'none';
    }
    window.onclick = function(event) {
        var modal = document.getElementById('helpModal');
        if (event.target == modal) {
            modal.style.display = 'none';
        }
    }
    </script>
</head>
<body>
    <div class="container">
        <h2>SMS2TG-LILYGO (Setup Mode)</h2>
        <div class="desc">
           Hello! To enable the device to forward received SMS to Telegram, please enter your WiFi network details (only 2.4 GHz networks are supported) and Telegram settings.
        </div>
        <form action="/save" method="POST">
            <label for="ssid">WiFi Network:</label>
            <select name="ssid_select" id="ssid_select" onchange="onSSIDChange(this)">
                %WIFI_OPTIONS%
            </select>
            <input name="ssid" id="custom_ssid" type="text" style="display:none" placeholder="Enter SSID manually" minlength="2" maxlength="32" pattern="[A-Za-z0-9_\-]+" />
            <label for="password">WiFi Password:</label>
            <div class="password-row">
                <input name="password" id="password" type="password" required minlength="8" maxlength="64" />
                <button type="button" onclick="togglePassword()" tabindex="-1"><span id="eyeIcon"><svg viewBox="0 0 24 24"><path d="M12 5c-7 0-10 7-10 7s3 7 10 7 10-7 10-7-3-7-10-7zm0 12c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.65 0-3 1.35-3 3s1.35 3 3 3 3-1.35 3-3-1.35-3-3-3z"/></svg></span></button>
            </div>
            <label for="token">Telegram Token: <span class="help-button" onclick="showHelp()">❓</span></label>
            <input name="token" id="token" type="text" required minlength="30" maxlength="60" pattern="[0-9]+:[A-Za-z0-9_-]+" />
            <label for="chat_id">Chat ID:</label>
            <input name="chat_id" id="chat_id" type="text" required pattern="-?[0-9]+" />
            <div class="desc-warning">
              After saving, the device will reboot and try to connect to WiFi. If connection fails, the access point %AP_SSID% will appear again.
            </div>
            <input type="submit" value="Save" />
            <div class="link">
              Project link: <a href="https://github.com/myroom/SMS2TG-LILYGO" target="_blank">https://github.com/myroom/SMS2TG-LILYGO</a>
            </div>
            
        </form>
    </div>

    <!-- Modal window with instructions -->
    <div id="helpModal" class="modal">
        <div class="modal-content">
            <span class="close" onclick="closeHelp()">&times;</span>
            <h3>📖 Telegram Setup</h3>
            
            <h4>🤖 Getting Token:</h4>
            <ol>
                <li>Find <code>@BotFather</code> in Telegram</li>
                <li>Send <code>/newbot</code> and follow instructions</li>
                <li>Copy the received Token</li>
            </ol>

            <h4>🆔 Getting Chat ID:</h4>
            <ol>
                <li>Find <code>@GetMyID</code> bot in Telegram</li>
                <li>Send <code>/start</code></li>
                <li>Bot will immediately send your Chat ID</li>
            </ol>

            <div class="note">
                <strong>💡 For channels:</strong> Add <code>@GetMyID</code> to the channel and it will show the channel ID (negative number).
            </div>
        </div>
    </div>
</body>
</html>
)rawliteral";

// Settings saved successfully page
const char savedHtml[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Saved</title>
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
        .icon {
            font-size: 2.5em;
            margin-bottom: 12px;
        }
        h2 { margin-bottom: 12px; font-size: 1.5em; font-weight: 600; }
        .desc { font-size: 1.1em; color: #444; margin-bottom: 0; }
        .next-steps {
            margin-top: 18px;
            font-size: 1em;
            color: #222;
            text-align: left;
            background: #f3f6fa;
            border-radius: 8px;
            padding: 14px 12px 10px 12px;
        }
        .next-steps b { color: #1976d2; }
        .next-steps a { color: #1976d2; text-decoration: underline; }
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
                justify-content: center;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <h2>✅ Settings Saved!</h2>
        <div class="desc">Device is rebooting...</div>
        <div class="next-steps">
          <b>What happens next?</b><br>
          After reboot, the device will try to connect to your WiFi network.<br><br>
          <b>How to find the device:</b><br>
          • Try opening <a href="http://SMS2TG-LILYGO.local" target="_blank">http://SMS2TG-LILYGO.local</a> in your browser.<br>
          • If the page doesn't open, check the device's IP address in your router settings and navigate to it.<br><br>
          <b>If the device failed to connect to WiFi</b>, the access point <span style="color:#1976d2;font-weight:600;">SMS2TG-SETUP</span> will appear again — connect to it and go to <a href="http://192.168.4.1" target="_blank">http://192.168.4.1</a>.
        </div>
    </div>
</body>
</html>
)rawliteral";

// Correct UCS2 (HEX) to String (UTF-8) conversion
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

// Format SMS date/time to readable format
String formatSmsDatetime(const String& raw) {
  // Example input: 25/06/18,02:59:35+12
  if (raw.length() < 17) return raw;
  String day = raw.substring(0, 2);
  String month = raw.substring(3, 5);
  String year = raw.substring(6, 8);
  String hour = raw.substring(9, 11);
  String min = raw.substring(12, 14);
  String sec = raw.substring(15, 17);
  //String tz = raw.substring(17); // +12
  String formatted = day + "." + month + ".20" + year + " " + hour + ":" + min + ":" + sec;
  return formatted;
}

// Power on modem
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

// Send SMS
void sendSMS(const String& phone, const String& message) {
  SerialMon.print("Sending SMS to number " + phone + "...");
  if (modem.sendSMS(phone, message)) {
    SerialMon.println(" SMS sent successfully!");
  } else {
    SerialMon.println(" SMS sending failed");
  }
}

// Read all unread SMS
void readAllUnreadSMS() {
  SerialAT.println("AT+CMGF=1");
  delay(200);
  // Query first 20 SIM card slots (can be increased if needed)
  for (int i = 1; i <= 20; ++i) {
    SerialAT.print("AT+CMGR=");
    SerialAT.println(i);
    delay(300);
    // Delete SMS after reading
    SerialAT.print("AT+CMGD=");
    SerialAT.println(i);
    delay(100);
  }
}

void sendToTelegram(const String& text) {
  if (WiFi.status() != WL_CONNECTED) {
    SerialMon.println("WiFi not connected, Telegram sending impossible.");
    return;
  }
  String token, chat_id;
  if (!loadTelegramSettings(token, chat_id)) {
    SerialMon.println("Telegram token or chat_id not set!");
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
  SerialMon.print("Telegram HTTP status: ");
  SerialMon.println(statusCode);
  SerialMon.print("Telegram response: ");
  SerialMon.println(response);
  
  // Add sending result logging
  if (statusCode == 200) {
    logPrintln("Message successfully sent to Telegram");
  } else {
    logPrintln("Telegram sending error. HTTP code: " + String(statusCode));
  }
  
  telegramHttp.stop();
}

void startAPMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);
  WiFi.setHostname(MDNS_HOSTNAME);
  IPAddress IP = WiFi.softAPIP();
  SerialMon.print("Access point IP address: ");
  SerialMon.println(IP);

  // Scan WiFi networks
  int n = WiFi.scanNetworks();
  String wifiOptions = "";
  String seenSSIDs = ",";
  for (int i = 0; i < n; ++i) {
    String ssid = WiFi.SSID(i);
    if (seenSSIDs.indexOf("," + ssid + ",") == -1) {
      wifiOptions += "<option value=\"" + ssid + "\">" + ssid + "</option>";
      seenSSIDs += ssid + ",";
    }
  }
  wifiOptions += "<option value=\"__custom__\">Other network...</option>";

  String page = String(htmlForm);
  page.replace("%WIFI_OPTIONS%", wifiOptions);
  page.replace("%AP_SSID%", AP_SSID);

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
        serverWeb.send(200, "text/html; charset=utf-8", savedHtml);
        delay(1500);
        ESP.restart();
    } else {
        serverWeb.send(200, "text/html; charset=utf-8",
            "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Error</title></head>"
            "<body style='display:flex;align-items:center;justify-content:center;height:100vh;margin:0;'>"
            "<div style='text-align:center;color:red;font-size:1.2em;'>Error: please fill all fields</div>"
            "</body></html>");
    }
  });
  serverWeb.onNotFound([]() {
    serverWeb.send(404, "text/html", "<b>Page not found</b>");
  });
  serverWeb.begin();
  SerialMon.println("Settings web interface started");
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
    <html lang="en">
    <head>
        <meta charset="UTF-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1.0" />
        <title>SMS2TG-LILYGO (Working Mode)</title>
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
            .ipinfo { font-size: 1em; margin-bottom: 12px; color: #1976d2; }
            .logbox { background: #222; color: #eee; font-size: 0.95em; text-align: left; border-radius: 7px; padding: 10px; min-height: 200px; max-height: 400px; overflow-y: auto; margin-bottom: 18px; font-family: 'Consolas', 'Menlo', 'Monaco', 'Liberation Mono', monospace; }
            form { margin-bottom: 0; }
            button {
                width: 100%;
                padding: 8px 0;
                color: #fff;
                border: none;
                border-radius: 7px;
                font-size: 1em;
                font-weight: 600;
                cursor: pointer;
                transition: background 0.2s;
                margin-bottom: 12px;
            }
            .btn-reboot {
                background: #1976d2;
            }
            .btn-reboot:hover { background: #1256a3; }
            .btn-reset {
                background: #d32f2f;
                margin-bottom: 0;
            }
            .btn-reset:hover { background: #b71c1c; }
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
        function updateLog() {
            fetch('/modem_log').then(r => r.text()).then(txt => {
                let logbox = document.getElementById('logbox');
                logbox.innerText = txt;
                logbox.scrollTop = logbox.scrollHeight;
            });
        }
        function updateIP() {
            fetch('/ip').then(r => r.text()).then(txt => {
                document.getElementById('ipinfo').innerText = 'Device IP address: ' + txt;
            });
        }
        setInterval(updateLog, 1200);
        setInterval(updateIP, 3000);
        window.onload = function() { updateLog(); updateIP(); };
        </script>
    </head>
    <body>
        <div class="container">
            <h2>SMS2TG-LILYGO (Working Mode)</h2>
            <div class="desc">Device is working. To reset settings, press the button below.</div>
            <div id="ipinfo" class="ipinfo">Device IP address: ...</div>
            <div id="logbox" class="logbox">Loading logs...</div>
            <form action="/reboot" method="POST">
                <button type="submit" class="btn-reboot">Reboot Device</button>
            </form>
            <form action="/reset" method="POST">
                <button type="submit" class="btn-reset">Reset Settings and Reboot</button>
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

    serverWeb.on("/reboot", HTTP_POST, []() {
        String rebootHtml = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><title>Rebooting</title>"
            "<meta name='viewport' content='width=device-width, initial-scale=1.0' />"
            "<style>"
            "body {background: #f7f7f7; min-height: 100vh; display: flex; align-items: center; justify-content: center; font-family: 'Consolas', 'Menlo', 'Monaco', 'Liberation Mono', monospace;}"
            ".container {background: #fff; padding: 32px 18px 24px 18px; border-radius: 12px; box-shadow: 0 2px 16px rgba(0,0,0,0.07); width: 100%; max-width: 420px; text-align: center;}"
            "h2 {margin-bottom: 16px; font-size: 1.5em; font-weight: 600;}"
            ".desc {font-size: 1.05em; color: #444; margin-bottom: 18px;}"
            ".link {font-size: 1.1em; margin: 18px 0 0 0;}"
            "a.button {display: inline-block; margin-top: 12px; padding: 10px 22px; background: #1976d2; color: #fff; border-radius: 7px; text-decoration: none; font-weight: 600; font-size: 1.1em; transition: background 0.2s;}"
            "a.button:hover {background: #1256a3;}"
            "@media (max-width: 600px) {.container{width:100vw;max-width:unset;border-radius:0;box-sizing:border-box;height:100vh;padding:18px 6vw;}}"
            "</style>"
            "<script>setTimeout(function(){window.location.href='/'}, 15000);</script>"
            "</head><body>"
            "<div class='container'>"
            "<h2>🔄 Rebooting Device</h2>"
            "<div class='desc'>Device is rebooting, please wait...<br><br>"
            "You will be automatically redirected to the main page in 15 seconds.</div>"
            "<a class='button' href='/'>Return to Main Page</a>"
            "</div></body></html>";
        serverWeb.send(200, "text/html; charset=utf-8", rebootHtml);
        delay(1500);
        ESP.restart();
    });

    serverWeb.on("/reset", HTTP_POST, []() {
        preferences.begin("wifi", false);
        preferences.clear();
        preferences.end();
        preferences.begin("telegram", false);
        preferences.clear();
        preferences.end();
        String resetHtml = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><title>Settings Reset</title>"
            "<meta name='viewport' content='width=device-width, initial-scale=1.0' />"
            "<style>"
            "body {background: #f7f7f7; min-height: 100vh; display: flex; align-items: center; justify-content: center; font-family: 'Consolas', 'Menlo', 'Monaco', 'Liberation Mono', monospace;}"
            ".container {background: #fff; padding: 32px 18px 24px 18px; border-radius: 12px; box-shadow: 0 2px 16px rgba(0,0,0,0.07); width: 100%; max-width: 420px; text-align: center;}"
            "h2 {margin-bottom: 16px; font-size: 1.5em; font-weight: 600;}"
            ".desc {font-size: 1.05em; color: #444; margin-bottom: 18px;}"
            ".link {font-size: 1.1em; margin: 18px 0 0 0;}"
            ".wifi {font-size: 1.1em; color: #1976d2; margin-bottom: 10px;}"
            "a.button {display: inline-block; margin-top: 12px; padding: 10px 22px; background: #1976d2; color: #fff; border-radius: 7px; text-decoration: none; font-weight: 600; font-size: 1.1em; transition: background 0.2s;}"
            "a.button:hover {background: #1256a3;}"
            "@media (max-width: 600px) {.container{width:100vw;max-width:unset;border-radius:0;box-sizing:border-box;height:100vh;padding:18px 6vw;}}"
            "</style></head><body>"
            "<div class='container'>"
            "<h2>✅ Settings Reset!</h2>"
            "<div class='desc'>Device is rebooting...<br><br>"
            "<span class='wifi'>Connect to WiFi network <b>%AP_SSID%</b></span><br>"
            "and go to:</div>"
            "<a class='button' href='http://192.168.4.1'>http://192.168.4.1</a>"
            "<div class='link'>If the page doesn't open, try manually entering the address in your browser.</div>"
            "</div></body></html>";
        resetHtml.replace("%AP_SSID%", AP_SSID);
        serverWeb.send(200, "text/html; charset=utf-8", resetHtml);
        delay(1500);
        ESP.restart();
    });

    serverWeb.on("/ip", HTTP_GET, []() {
        String ip = WiFi.localIP().toString();
        serverWeb.send(200, "text/plain; charset=utf-8", ip);
    });

    serverWeb.onNotFound([]() {
        serverWeb.send(404, "text/html; charset=utf-8", "<b>Page not found</b>");
    });

    serverWeb.begin();
    SerialMon.println("Working mode web interface started");
}

#endif // UCS2_UTILS_H 