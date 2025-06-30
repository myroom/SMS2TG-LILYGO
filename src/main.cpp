// GPRS settings
const char apn[]  = "internet"; // Replace with your carrier's APN
const char user[] = "";
const char pass[] = "";

// Dummy values for server and port to avoid compilation errors
const char server[] = "192.168.4.1";
const int port = 80;

#include <Arduino.h>
#include <WebServer.h>
#include <Preferences.h>
#include <FS.h>
#include <SPIFFS.h>

// Set debug port (to Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set AT commands port (to SIM800 module)
#define SerialAT Serial1

// TTGO T-Call pins
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26

// TinyGSM library configuration
#define TINY_GSM_MODEM_SIM800      // SIM800 modem
#define TINY_GSM_RX_BUFFER   1024  // Set RX buffer to 1KB
//#define DUMP_AT_COMMANDS // for AT commands debugging

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>


TinyGsm modem(SerialAT);

// Initialize GSM and HTTP clients
TinyGsmClient client(modem);
HttpClient http(client, server, port);

#include "utils.h"

Preferences preferences;
WebServer serverWeb(80);

String imei = "";
WiFiClientSecure wifiClientSecure;

// --- Asynchronous state machine for modem and GSM initialization ---
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

// Counter for failed modem initialization attempts
int modemFailCount = 0;
const int MODEM_MAX_FAILS = 5;

bool isSetupMode = false;

// --- Get current date and time (simple version using millis if no RTC) ---
String getTimeStamp() {
  time_t now = time(nullptr);
  struct tm *tm_info = localtime(&now);
  char buf[24];
  if (now > 100000) {
    strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M:%S ", tm_info);
    return String(buf);
  } else {
    // If time is not set, use date 01.01.1970 and current time from millis
    unsigned long ms = millis() / 1000;
    unsigned long h = ms / 3600;
    unsigned long m = (ms % 3600) / 60;
    unsigned long s = ms % 60;
    snprintf(buf, sizeof(buf), "01.01.1970 %02lu:%02lu:%02lu ", h, m, s);
    return String(buf);
  }
}

// --- Log buffer for web interface ---
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

// --- Function to get modem state (state machine) ---
const char* getModemStateName(ModemInitState state) {
  switch(state) {
    case INIT_IDLE: return "Waiting";
    case INIT_POWER_ON: return "Powering on modem";
    case INIT_SERIAL: return "Initializing Serial";
    case INIT_AT_CMGF: return "AT+CMGF=1";
    case INIT_AT_CNMI: return "AT+CNMI=2,1,0,0,0";
    case INIT_RESTART: return "Restarting modem";
    case INIT_WAIT_NET: return "Waiting for GSM network";
    case INIT_CHECK_NET: return "Checking network";
    case INIT_READ_SMS: return "Reading SMS";
    case INIT_DONE: return "Ready";
    case INIT_FAIL: return "Error";
    default: return "?";
  }
}

// --- Make functions visible to other files ---
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
      logPrint("Modem: Power ON...");
      break;
    case INIT_POWER_ON:
      if (millis() - modemTimer > 500) {
        SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
        modemTimer = millis();
        modemState = INIT_SERIAL;
        logPrint("SerialAT started...");
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
        logPrint("Initializing modem...");
        modem.restart();
        modemTimer = millis();
        modemState = INIT_RESTART;
      }
      break;
    case INIT_RESTART:
      if (millis() - modemTimer > 1000) {
        logPrint("Waiting for GSM network (30 sec)...");
        modemTimer = millis();
        modemState = INIT_WAIT_NET;
      }
      break;
    case INIT_WAIT_NET:
      if (modem.waitForNetwork(1)) { // 1 second per attempt
        logPrintln("Network found");
        modemState = INIT_CHECK_NET;
      } else if (millis() - modemTimer > 30000) { // 30 sec timeout
        logPrintln("Failed to connect to GSM network");
        modemState = INIT_FAIL;
      } else {
        // Don't log waiting dots
        delay(200);
      }
      break;
    case INIT_CHECK_NET:
      if (modem.isNetworkConnected()) {
        logPrintln("Successfully connected to GSM network");
        modemState = INIT_READ_SMS;
      } else {
        logPrintln("GSM network connection error");
        modemState = INIT_FAIL;
      }
      break;
    case INIT_READ_SMS:
      readAllUnreadSMS();
      logPrintln("Waiting for incoming SMS...");
      modemInitOk = true;
      modemState = INIT_DONE;
      break;
    case INIT_DONE:
      modemFailCount = 0; // reset counter on successful initialization
      break;
    case INIT_FAIL:
      modemFailCount++;
      logPrintln("Modem initialization error. Restarting modem...");
      delay(1000);
      modemPowerOn();
      if (modemFailCount >= MODEM_MAX_FAILS) {
        logPrintln("Modem not recovered after " + String(MODEM_MAX_FAILS) + " attempts. Rebooting device...");
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

  // Load WiFi and Telegram settings
  String savedSsid, savedPass;
  String token, chat_id;
  if (!loadWiFiSettings(savedSsid, savedPass) || !loadTelegramSettings(token, chat_id)) {
    logPrintln("WiFi or Telegram not configured.");
    logPrintln("Starting access point and web interface...");
    startAPMode();  // Start access point and web interface
    isSetupMode = true;
    return;
  }

  // If settings exist - connect to WiFi
  logPrint("Connecting to WiFi: ");
  logPrintln(savedSsid);
  WiFi.begin(savedSsid.c_str(), savedPass.c_str());
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    logPrintln("WiFi connected, IP address: " + WiFi.localIP().toString());
    wifiClientSecure.setInsecure();

    // --- NTP time initialization ---
    configTzTime("MSK-3", "pool.ntp.org", "time.nist.gov"); // Europe/Moscow
    logPrintln("Waiting for time synchronization via NTP...");
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
      logPrintln("Time synchronized: " + String(buf));
    } else {
      logPrintln("Failed to synchronize time via NTP");
    }
    // --- end NTP ---

    // Start web interface for working mode
    startWorkModeWeb();
  } else {
    logPrintln("\nWiFi connection error, rebooting...");
    delay(2000);
    ESP.restart();
  }

  // Start asynchronous modem initialization
  modemState = INIT_IDLE;
  modemInitOk = false;
}

void loop() {
  serverWeb.handleClient(); // web interface always available

  if (isSetupMode) {
    // In setup mode, don't work with modem
    return;
  }

  if (!modemInitOk) {
    asyncModemInit();
    return; // while modem is not initialized, don't process SMS
  }

  if (SerialAT.available()) {
    String line = SerialAT.readStringUntil('\n');
    line.trim();

    // If new SMS notification received
    if (line.startsWith("+CMTI:")) {
      int idx1 = line.indexOf(',');
      if (idx1 != -1) {
        int smsIndex = line.substring(idx1 + 1).toInt();
        SerialAT.print("AT+CMGR=");
        SerialAT.println(smsIndex);
        delay(300);
      }
    }
    // If response to AT+CMGR received (SMS content)
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

      // Decode UCS2 if text contains only HEX digits
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
      logPrintln("\nNew SMS!\nSender: " + sender + "\nDate/time: " + formattedDatetime + "\nText: " + outText);
      
      // Send SMS to Telegram via WiFi
      sendToTelegram("SMS from: " + sender + "\nDate/time: " + formattedDatetime + "\nText: " + outText);
    }
  }
}