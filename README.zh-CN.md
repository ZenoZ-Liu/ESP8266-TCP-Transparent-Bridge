# ESP8266 TCP 透传桥（STM32F103C8T6）

基于 **STM32F103C8T6**（Blue Pill）和 **ESP8266**（ESP-01）的 TCP 透传工程：STM32 将 ESP8266 配置为 **STA 模式**，连接 WiFi，与同一局域网内的 **TCP 服务器**建立连接，随后进入**透传模式**，实现 STM32 与服务器之间的双向数据透传。

[English README](README.md)

## 功能特性

- ESP8266 STA 模式初始化（重启生效）
- 连接 WiFi，并连接同一局域网内的 TCP 服务器
- USART2 + DMA + IDLE 中断实现不定长接收
- USART1 输出调试/进度信息（115200 8N1）
- 透传模式下：USART2 发出的数据直达服务器；服务器数据转发到 USART1

## 硬件接线

| STM32 引脚 | 外设 | 连接目标 |
| --- | --- | --- |
| PA2 | USART2_TX | ESP8266 RX |
| PA3 | USART2_RX | ESP8266 TX |
| PA9 | USART1_TX | USB-TTL RX（电脑） |
| PA10 | USART1_RX | USB-TTL TX（电脑） |
| GND | — | 共地 |

ESP8266（ESP-01）模块侧：

| 引脚 | 接法 |
| --- | --- |
| VCC | 独立 3.3V 供电（峰值约 300~500mA，建议外部稳压），并联 100~470µF 电容 |
| GND | 共地 |
| EN (CH_PD) | 10kΩ 上拉到 3.3V（不能悬空） |
| GPIO0 | 10kΩ 上拉到 3.3V（正常启动）；烧录固件时才拉低 |
| RST | 10kΩ 上拉到 3.3V |

波特率：两个串口均为 **115200、8N1**。

## 使用方法

1. 修改 `Core/Src/main.c`：

   ```c
   #define WIFI_SSID       "your_wifi_name"
   #define WIFI_PASSWORD   "your_wifi_password"
   #define TCP_SERVER_IP   "192.168.1.100"   /* 服务器 IP，须与模块同一网段 */
   #define TCP_SERVER_PORT 8080
   ```

2. 电脑上先启动 TCP 服务端（如网络调试助手 NetAssist），监听对应端口，并在 Windows 防火墙中放行。
3. 使用 CLion 编译烧录（CMake 工程，已包含 `Core/APP`）。
4. 串口助手连接 USART1（115200）查看初始化输出。

预期输出：

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

初始化完成后，STM32 每秒向服务器发送一条 `Hello from STM32`；服务器发回的数据会转发到 USART1 显示。

## 初始化流程

```
AT+CWMODE=1     → 进入 STA 模式（重启生效）
AT+RST          → 重启模块，等待 "ready"
AT+CIPMUX=0     → 设置单路连接
AT+CWJAP=...    → 连接 WiFi
AT+CIFSR        → 查询 IP（可选）
AT+CIPSTART=... → 连接 TCP 服务器
AT+CIPMODE=1 / AT+CIPSEND → 开启透传
```

## 目录结构

```
Core/APP/esp8266.c | esp8266.h   ESP8266 驱动（STA + TCP 透传）
Core/Src/main.c                  应用入口：6 步初始化 + 透传演示
```

## License

Unlicense（公有领域）——见 [LICENSE](LICENSE)。