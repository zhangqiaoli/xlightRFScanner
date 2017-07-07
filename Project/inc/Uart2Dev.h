#ifndef _UART2_DEV_H_
#define _UART2_DEV_H_

#include "stm8s.h"

#define MAX_UART_BUF_SIZE               50

void uart2_config(uint32_t speed);
uint8_t Usart2SendBuffer(uint8_t *pBuf, uint8_t ucLen);
uint8_t Uart2SendString(uint8_t *pBuf);

#endif