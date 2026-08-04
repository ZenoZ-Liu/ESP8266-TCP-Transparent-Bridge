#ifndef __ESP8266_H
#define __ESP8266_H

#include "main.h"

#define ESP8266_RX_BUF_SIZE   512      /* RX buffer size (bytes) */

/* Return values of esp_sendcmd */
#define ESP8266_OK            1
#define ESP8266_TIMEOUT       0

typedef struct
{
  volatile uint16_t len;                 /* Bytes received in the latest frame */
  uint8_t  buf[ESP8266_RX_BUF_SIZE + 1]; /* RX buffer, +1 for '\0' */
} ESP8266_Rx_t;

extern ESP8266_Rx_t g_esp8266_rx;

void    ESP8266_Init(void);                                          /* Init USART2 DMA RX */
uint8_t esp_sendcmd(char *cmd, char *res, uint32_t timeout);         /* Send command, wait for reply */
uint8_t ESP8266_SetSTAMode(void);                                    /* Step1: STA mode + restart */
uint8_t ESP8266_SetSingleConnection(void);                           /* Step2: single connection */
uint8_t ESP8266_ConnectWiFi(const char *ssid, const char *password); /* Step3: connect WiFi */
uint8_t ESP8266_GetIP(void);                                         /* Step4: query IP (optional) */
uint8_t ESP8266_ConnectTCPServer(const char *ip, uint16_t port);     /* Step5: connect TCP server */
uint8_t ESP8266_EnableTransparent(void);                             /* Step6: transparent mode */
void    ESP8266_SendData(const char *data);                          /* Send data in transparent mode */

#endif /* __ESP8266_H */
