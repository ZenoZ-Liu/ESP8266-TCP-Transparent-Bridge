# ESP8266 TCP Transparent Bridge

A TCP transparent-transmission bridge built on **STM32F103C8T6** (Blue Pill) and **ESP8266** (ESP-01). The STM32 puts the ESP8266 into **STA mode**, joins a WiFi network, connects to a TCP server on the same LAN, then enters **transparent mode** so UART data flows bidirectionally between the STM32 and the server.

[中文版说明](README.zh-CN.md)

## Features

- ESP8266 STA-mode initialization (restart to take effect)
- Connects to WiFi and to a TCP server on the same LAN
- Variable-length reception over **USART2 + DMA + IDLE interrupt**
- Debug / progress output over USART1 (PC, 115200 8N1)
- In transparent mode: data sent on USART2 goes straight to the server; server data is forwarded to USART1

## Hardware

| STM32 pin | Peripheral | Connects to |
| --- | --- | --- |
| PA2 | USART2_TX | ESP8266 RX |
| PA3 | USART2_RX | ESP8266 TX |
| PA9 | USART1_TX | USB-TTL RX (PC) |
| PA10 | USART1_RX | USB-TTL TX (PC) |
| GND | - | common ground |

ESP8266 (ESP-01) side:

| Pin | Connection |
| --- | --- |
| VCC | independent 3.3V supply (peak ~300-500 mA; external regulator recommended) plus a 100-470 uF capacitor |
| GND | common ground |
| EN (CH_PD) | 10k pull-up to 3.3V (must not float) |
| GPIO0 | 10k pull-up to 3.3V (normal boot); pull low only when flashing firmware |
| RST | 10k pull-up to 3.3V |

Baud rate: **115200, 8N1** on both UARTs.

## Getting Started

1. Edit `Core/Src/main.c`:

   ```c
   #define WIFI_SSID       "your_wifi_name"
   #define WIFI_PASSWORD   "your_wifi_password"
   #define TCP_SERVER_IP   "192.168.1.100"   /* server IP, same LAN as the module */
   #define TCP_SERVER_PORT 8080
   ```

2. Start a TCP server on your PC (e.g. NetAssist) listening on the configured port, and allow it through Windows Firewall.
3. Build and flash with CLion (CMake project; `Core/APP` is already included).
4. Open a serial terminal on USART1 (115200) and watch the initialization progress.

Expected output:

```
ESP8266 STA mode init...
Step1 STA mode OK
Step2 single connection OK
Step3 WiFi connected
Step4 IP: AT+CIFSR
+CIFSR:STAIP,"192.168.1.100"
...
Step5 TCP connected
Step6 transparent mode OK, start sending...
```

After initialization, the STM32 sends `Hello from STM32` to the server every second; data received from the server is forwarded to USART1.

## Initialization Sequence

```
AT+CWMODE=1      -> enter STA mode (restart required)
AT+RST           -> restart module, wait for "ready"
AT+CIPMUX=0      -> single connection
AT+CWJAP=...     -> connect WiFi
AT+CIFSR         -> query IP (optional)
AT+CIPSTART=...  -> connect TCP server
AT+CIPMODE=1 / AT+CIPSEND -> enable transparent mode
```

## Project Structure

```
Core/APP/esp8266.c | esp8266.h   ESP8266 driver (STA + TCP transparent mode)
Core/Src/main.c                  application entry: 6-step init + demo loop
```

## License

Unlicense (public domain) - see [LICENSE](LICENSE).