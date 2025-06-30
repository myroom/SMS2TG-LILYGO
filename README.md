# LilyGO T-Call SIM800 — SMS Reception and Telegram Forwarding

## ☕ Support the Author

Hi! I'm a developer and tech enthusiast who loves building and sharing tools with the community. Your support helps me keep creating and improving. Thank you for your coffee and motivation ☕🙂

[![Buy Me a Coffee](https://img.shields.io/badge/☕%20Buy%20me%20a%20coffee-coffee%20support-yellow)](https://coff.ee/myroom007)

## Description

This project is designed for the LilyGO T-Call SIM800 board (ESP32 + SIM800L) and allows for automatic reception of incoming SMS messages, displaying them through a web interface, and forwarding message content to a Telegram chat. WiFi and Telegram configuration is done through a convenient web interface available on first startup or after settings reset.

## About the Device

[LilyGO T-Call v1.4](https://lilygo.cc/products/t-call-v1-4) is a development board based on ESP32 microcontroller with integrated SIM800H/SIM800L GSM/GPRS module. The board features:

-   **ESP32**: 240MHz dual-core processor with 4MB/8MB Flash and 8MB PSRAM
-   **GSM/GPRS**: SIM800H module for 2G cellular connectivity
-   **Connectivity**: Wi-Fi 802.11 b/g/n, Bluetooth v4.2 BR/EDR and BLE
-   **Power**: USB Type-C charging and JST connector for Li-Po battery (IP5306 charging IC)
-   **Serial**: CH9102 USB-to-serial converter
-   **Interfaces**: I2C, SPI, UART, SDIO, I2S, CAN

### Antenna Recommendation

For optimal signal reception and reliable SMS operation, it is **highly recommended** to install an external GSM/GPRS antenna. The built-in antenna may not provide sufficient signal strength in all locations. You can purchase a compatible external antenna from [LilyGO GSM/GPRS Antenna](https://lilygo.cc/products/gsm-gprs-antenna) which is specifically designed for T-Call v1.4 and will significantly improve signal quality and connection stability.

### 3D Printable Case

This project includes 3D printable case files in the `3D files/` folder. You can print a protective case for your LilyGO T-Call board using any 3D printer. The STL files are ready to print and will help protect your device during operation.

## Features

-   Reception of incoming SMS from SIM card
-   Automatic sending of SMS text to Telegram chat
-   Modern web interface for configuring WiFi and Telegram Token/Chat ID
-   View device operation logs through web interface
-   Reset settings through web interface with one button
-   Support for Cyrillic and other languages (UCS2 decoding)
-   All device operations are displayed in Serial Monitor for debugging

## Requirements

-   LilyGO T-Call SIM800 board (ESP32 + SIM800L)
-   SIM card with active plan
-   Arduino IDE or PlatformIO
-   Arduino library (ESP32 core)

## Power Connection

To power the device, simply connect the USB-C cable to the board. The LilyGO T-Call v1.4 has all necessary connections pre-wired and ready to use - no additional wiring is required.

## Quick Start and Setup

1. Open the project in Arduino IDE or PlatformIO.
2. Connect the board to your computer via USB.
3. Select **ESP32 Dev Module** board and the corresponding port.
4. Upload the sketch to the board.
5. Open Serial Monitor (speed 115200 baud).
6. Insert the SIM card into the module and apply power.
7. On first startup, the device will create a WiFi access point `SMS2TG-SETUP`.
8. Connect to this WiFi network from your phone or computer and open a browser at [http://192.168.4.1](http://192.168.4.1).
9. In the web interface, enter your WiFi network parameters and Telegram settings (Token and Chat ID), then save the settings.
10. After reboot, the device will connect to WiFi and start working in main mode.

## Device Operation

-   All incoming SMS are automatically forwarded to the specified Telegram chat.
-   In the web interface, you can view modem status and operation logs.
-   To reset settings, use the "Reset all settings" button in the web interface.

## Example Serial Monitor Output

```
New SMS!
Sender: +79161234567
Date/time: 01.05.2023 12:34:56 +12
Text: Test message
```

## Project Development

This project is actively being developed and improved. If you have suggestions for new features, improvements, or encounter any issues, please feel free to:

-   Open an issue on GitHub
-   Submit feature requests
-   Share your ideas for improvements

Your feedback helps make this project better for everyone!

## Sources

-   [Official LilyGO T-Call SIM800 repository](https://github.com/Xinyuan-LilyGO/LilyGo-T-Call-SIM800)
-   [LilyGO T-Call v1.4 Product Page](https://lilygo.cc/products/t-call-v1-4)
