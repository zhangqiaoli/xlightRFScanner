#ifndef _SERIAL_CONSOLE_H_
#define _SERIAL_CONSOLE_H_

#include "stm8s.h"

#define RFS_MESSAGE_HEAD_0      0x55
#define RFS_MESSAGE_HEAD_1      0xAA

#define RFS_CMD_SCAN_START      'S'
#define RFS_CMD_SCAN_STOP       'T'
#define RFS_CMD_SCAN_PAUSE      'P'
#define RFS_CMD_SCAN_RESUME     'R'
#define RFS_RPT_SCAN_STATUS     'C'

bool ProcessSerialMessage(const uint8_t *pBuf, const uint8_t nLen);
bool SendSerialMessage(uint8_t *pBuf, const uint8_t nLen);
bool AddSerialOutputBuf(const uint8_t *pBuf);
bool ProcessOutputSerialMsg();

#endif // _SERIAL_CONSOLE_H_