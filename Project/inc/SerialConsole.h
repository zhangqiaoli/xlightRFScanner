#ifndef _SERIAL_CONSOLE_H_
#define _SERIAL_CONSOLE_H_

#include "stm8s.h"

#define RFS_MESSAGE_HEAD_0      0x55
#define RFS_MESSAGE_HEAD_1      0xAA

bool ProcessSerialMessage(const uint8_t *pBuf, const uint8_t nLen);
bool SendSerialMessage(uint8_t *pBuf, const uint8_t nLen);

#endif // _SERIAL_CONSOLE_H_