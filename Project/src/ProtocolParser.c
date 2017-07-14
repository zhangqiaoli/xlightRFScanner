#include "ProtocolParser.h"
#include "_global.h"
#include "MyMessage.h"
#include "MyMessageQueue.h"
#include "xliNodeConfig.h"

uint8_t bMsgReady = 0;

// Assemble message
void build(uint8_t _destination, uint8_t _sensor, uint8_t _command, uint8_t _type, bool _enableAck, bool _isAck)
{
    sndMsg.header.version_length = PROTOCOL_VERSION;
    sndMsg.header.sender = gConfig.nodeID;
    sndMsg.header.destination = _destination;
    sndMsg.header.sensor = _sensor;
    sndMsg.header.type = _type;
    moSetCommand(_command);
    moSetRequestAck(_enableAck);
    moSetAck(_isAck);
}

uint8_t ParseProtocol(){
  if( rcvMsg.header.destination != gConfig.nodeID && rcvMsg.header.destination != BROADCAST_ADDRESS ) return 0;
  
  uint8_t _cmd = miGetCommand();
  uint8_t _sender = rcvMsg.header.sender;  // The original sender
  uint8_t _type = rcvMsg.header.type;
  uint8_t _sensor = rcvMsg.header.sensor;
  uint8_t _lenPayl = miGetLength();
  bool _needAck = (bool)miGetRequestAck();
  bool _isAck = (bool)miGetAck();
  bool _OnOff;
  uint8_t targetSubID;
  
  switch( _cmd ) {
  case C_INTERNAL:
    if( _type == I_CONFIG ) {
    } else if( _type == I_GET_NONCE_RESPONSE ) {
      // transfer data
      // MMQ_AddMessage(&rfReceivedMQ, (const u8 *)&rcvMsg);
      AddSerialOutputBuf((const u8 *)&rcvMsg);
    }
    break;
    
  case C_REQ:
    break;
    
  case C_SET:
    break;
  }
  
  return 0;
}

// Prepare Probe message
void Msg_ProbeMsg() {
  build(NODEID_DUMMY, 0x00, C_INTERNAL, I_GET_NONCE, 1, 0);
  sndMsg.payload.data[0] = SCANNER_PROBE;
  moSetLength(1);
  moSetPayloadType(P_BYTE);
  bMsgReady = 1;
}

void Msg_TransferMsg(const uint8_t *pBuf, const uint8_t nLen) {
  memcpy((u8 *)&sndMsg, pBuf, nLen);
  bMsgReady = 1;
}

void Msg_NodeScanMsg(const uint8_t _subType, const uint8_t _nodeID, const uint8_t _subID) {
  uint8_t payl_len = 0;
  build(_nodeID, 0x00, C_INTERNAL, I_GET_NONCE, 1, 0);

  sndMsg.payload.data[payl_len++] = _subType;
  sndMsg.payload.data[payl_len++] = _nodeID;
  sndMsg.payload.data[payl_len++] = _subID;
  
  switch( _subType ) {
  case SCANNER_SETUP_RF:
    // ToDo: set setup parameters
    //...
    break;
    
  case SCANNER_GETCONFIG:
    // ToDo: set query parameters
    //...
    break;

  case SCANNER_TEST_NODE:
    // ToDo: set test parameters
    //...
    break;
  }
  
  moSetLength(payl_len);
  moSetPayloadType(P_CUSTOM);
  bMsgReady = 1;
}