//
// Created by LZM on 2026/8/3.
//
// ESP8266 driver: STA mode + TCP transparent transmission
//
// Wiring: USART2 <-> ESP8266
//   PA2 (USART2_TX) --> ESP8266 RX
//   PA3 (USART2_RX) <-- ESP8266 TX
// RX method: USART2 DMA + IDLE interrupt (variable length reception)
//
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "usart.h"
#include "esp8266.h"

ESP8266_Rx_t g_esp8266_rx;
static uint8_t esp8266_transparent = 0;   /* 1 when transparent mode is active */

/**
 * @brief  Init ESP8266 UART reception (DMA + IDLE, variable length)
 * @note   Must be called after MX_USART2_UART_Init()
 */
void ESP8266_Init(void)
{
  g_esp8266_rx.len = 0;
  esp8266_transparent = 0;

  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, g_esp8266_rx.buf, ESP8266_RX_BUF_SIZE);
  __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);   /* IDLE/complete interrupt is enough */
}

/**
 * @brief  USART2 reception callback (DMA + IDLE)
 * @note   In transparent mode, forward server data to USART1 (PC) for debugging
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART2)
  {
    g_esp8266_rx.buf[Size] = '\0';
    g_esp8266_rx.len = Size;

    if (esp8266_transparent)
    {
      HAL_UART_Transmit(&huart1, g_esp8266_rx.buf, Size, 100);
    }

    /* Re-arm reception for the next frame */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, g_esp8266_rx.buf, ESP8266_RX_BUF_SIZE);
    __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
  }
}

/**
 * @brief  Send one AT command and wait for the expected reply keyword
 * @param  cmd     Command string, must end with "\r\n"
 * @param  res     Expected reply keyword, e.g. "OK", "ready", "CONNECT"
 * @param  timeout Wait timeout in ms
 * @retval ESP8266_OK on success, ESP8266_TIMEOUT on timeout / module error
 */
uint8_t esp_sendcmd(char *cmd, char *res, uint32_t timeout)
{
  uint32_t start = HAL_GetTick();

  g_esp8266_rx.len = 0;   /* discard stale data */

  if (HAL_UART_Transmit(&huart2, (uint8_t *)cmd, strlen(cmd), 200) != HAL_OK)
  {
    return ESP8266_TIMEOUT;
  }

  while (HAL_GetTick() - start < timeout)
  {
    if (g_esp8266_rx.len > 0)
    {
      /* Check errors first so "CONNECT FAIL" is not taken as a success */
      if ((strstr((char *)g_esp8266_rx.buf, "ERROR") != NULL) ||
          (strstr((char *)g_esp8266_rx.buf, "FAIL") != NULL) ||
          (strstr((char *)g_esp8266_rx.buf, "DNS Fail") != NULL))
      {
        // printf("WARNING!");
        return ESP8266_TIMEOUT;
      }
      if (strstr((char *)g_esp8266_rx.buf, res) != NULL)
      {
        return ESP8266_OK;
      }
    }
  }
  return ESP8266_TIMEOUT;
}

/**
 * @brief  Step1: set STA mode, restart to take effect
 */
uint8_t ESP8266_SetSTAMode(void)
{
  
  if (esp_sendcmd("AT+CWMODE=1\r\n", "OK", 2000) != ESP8266_OK)
  {
    printf("%s", (char *)g_esp8266_rx.buf);
    /* Some firmwares reply "no change" when mode is already STA */
    if (strstr((char *)g_esp8266_rx.buf, "no change") == NULL)
    {
      return ESP8266_TIMEOUT;
    }
  }
  HAL_Delay(200);

  /* Restart the module and wait until it is ready */
  if (esp_sendcmd("AT+RST\r\n", "ready", 10000) != ESP8266_OK)
  {
    /* Print what was actually received so we can tell whether the
       module rebooted at all (empty buffer = module never responded) */
    printf("3333 len=%u: [%s]\r\n", (unsigned int)g_esp8266_rx.len, (char *)g_esp8266_rx.buf);
    return ESP8266_TIMEOUT;
  }
  printf("4444\r\n");
  HAL_Delay(500);   /* wait until the module is fully up */
  return ESP8266_OK;
}

/**
 * @brief  Step2: set single connection (AT+CIPMUX=0)
 */
uint8_t ESP8266_SetSingleConnection(void)
{
  if (esp_sendcmd("AT+CIPMUX=0\r\n", "OK", 2000) != ESP8266_OK)
  {
    if (strstr((char *)g_esp8266_rx.buf, "no change") == NULL)
    {
      return ESP8266_TIMEOUT;
    }
  }
  return ESP8266_OK;
}

/**
 * @brief  Step3: connect WiFi (ESP8266 and TCP server must be in the same LAN)
 */
uint8_t ESP8266_ConnectWiFi(const char *ssid, const char *password)
{
  char cmd[160];

  sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
  return esp_sendcmd(cmd, "OK", 10000);
}

/**
 * @brief  Step4: query IP address (optional), reply stays in the RX buffer
 */
uint8_t ESP8266_GetIP(void)
{
  /* Wait for the STAIP line instead of the trailing "OK", otherwise the
     OK reply (a separate frame) overwrites the IP info in the RX buffer */
  return esp_sendcmd("AT+CIFSR\r\n", "STAIP", 3000);
}

/**
 * @brief  Step5: connect TCP server
 */
uint8_t ESP8266_ConnectTCPServer(const char *ip, uint16_t port)
{
  char cmd[160];

  sprintf(cmd, "AT+CIPSTART=\"TCP\",\"%s\",%u\r\n", ip, port);
  return esp_sendcmd(cmd, "CONNECT", 8000);
}

/**
 * @brief  Step6: enable transparent mode
 */
uint8_t ESP8266_EnableTransparent(void)
{
  uint32_t start;

  if (esp_sendcmd("AT+CIPMODE=1\r\n", "OK", 2000) != ESP8266_OK)
  {
    return ESP8266_TIMEOUT;
  }
  HAL_Delay(200);

  /* AT+CIPSEND enters transparent mode: reply may be ">" or "OK" */
  g_esp8266_rx.len = 0;
  if (HAL_UART_Transmit(&huart2, (uint8_t *)"AT+CIPSEND\r\n", strlen("AT+CIPSEND\r\n"), 200) != HAL_OK)
  {
    return ESP8266_TIMEOUT;
  }

  start = HAL_GetTick();
  while (HAL_GetTick() - start < 3000)
  {
    if (g_esp8266_rx.len > 0)
    {
      if ((strchr((char *)g_esp8266_rx.buf, '>') != NULL) ||
          (strstr((char *)g_esp8266_rx.buf, "OK") != NULL))
      {
        esp8266_transparent = 1;
        g_esp8266_rx.len = 0;
        return ESP8266_OK;
      }
      if ((strstr((char *)g_esp8266_rx.buf, "ERROR") != NULL) ||
          (strstr((char *)g_esp8266_rx.buf, "FAIL") != NULL))
      {
        return ESP8266_TIMEOUT;
      }
    }
  }
  return ESP8266_TIMEOUT;
}

/**
 * @brief  Send data in transparent mode (sent as-is to the TCP server)
 */
void ESP8266_SendData(const char *data)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)data, strlen(data), 100);
}
