#ifndef _SERIAL_CONSOLE_H_
#define _SERIAL_CONSOLE_H_

#include "stm8s.h"

bool ProcessSerialMessage(const uint8_t *pBuf, const uint8_t nLen);

#endif // _SERIAL_CONSOLE_H_