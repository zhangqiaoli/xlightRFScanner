#ifndef __PROTOCOL_PARSER_H
#define __PROTOCOL_PARSER_H

#include "_global.h"

extern uint8_t bMsgReady;

uint8_t ParseProtocol();
void build(uint8_t _destination, uint8_t _sensor, uint8_t _command, uint8_t _type, bool _enableAck, bool _isAck);
void Msg_ProbeMsg();
void Msg_TransferMsg(const uint8_t *pBuf, const uint8_t nLen);
void Msg_NodeScanMsg(const uint8_t _subType, const uint8_t _nodeID, const uint8_t _subID);

#endif /* __PROTOCOL_PARSER_H */