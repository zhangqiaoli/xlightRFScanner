#include "_global.h"
#include "MyMessage.h"
#include "ProtocolParser.h"
#include "SerialConsole.h"
#include "Uart2Dev.h"

bool ProcessSerialMessage(const uint8_t *pBuf, const uint8_t nLen) {
  // ToDo: parse serial protocol or use state machine
  // commands: 
  ///   Scan: start, pause, resume, stop, etc.
  ///   Config: by device id (unique id), by nodeid, by nodeid & subid, etc.
  ///   Query: state, config, etc.
  ///   Test: on, off, brightness, cct, color, special effect, relay key, etc.
  
  // Checksum
  uint16_t check_sum = 0;
  uint16_t got_check_sum = pBuf[nLen - 1] + pBuf[nLen - 2] * 256;
  for( uint8_t i = 0; i < nLen - 2; i++ ) {
    check_sum += pBuf[i];
  }
  if( check_sum != got_check_sum ) return FALSE;
      
  uint8_t cmdLen = pBuf[0];
  uint8_t cmdType = pBuf[1];
  switch( cmdType ) {
  case RFS_CMD_SCAN_START:              // start
    // Read parameters from message
    // ToDo:...
    //memcpy(gConfig.NetworkID, ..., sizeof(gConfig.NetworkID));
    uint8_t pldata_start = 8;
    gConfig.rfChannel = pBuf[pldata_start++];
    gConfig.rfDataRate = pBuf[pldata_start++];
    gConfig.rfPowerLevel = pBuf[pldata_start++];
    memcpy(gConfig.NetworkID, pBuf+pldata_start, 5);   
    startScan();
    break;
  case RFS_CMD_SCAN_STOP:              // stop
    stopScan();
    break;
  case RFS_CMD_SCAN_PAUSE:             // pause
    pauseScan();
    break;
  case RFS_CMD_SCAN_RESUME:            // resume
    resumeScan();
    break;
    
  default:
    // Transfer transparently (length should be MAX_MESSAGE_LENGTH)
    Msg_TransferMsg(pBuf + 1, cmdLen - 1);
    break;
  }
  
  return TRUE;
}
#define MAX_OUT_MESSAGE_LENGTH 32
typedef struct _node_t
{
  u8 ttl;       // Time to live
  u8 data[MAX_OUT_MESSAGE_LENGTH];
} SerialMsg;

#define OUT_MESSAGE_LEN           10
SerialMsg msg_out_buf[OUT_MESSAGE_LEN];
u8 msg_out_buf_read_ptr = 0;
u8 msg_out_buf_write_ptr = 0;

bool AddSerialOutputBuf(const uint8_t *pBuf) {  
  SerialMsg msg;
  memcpy(msg.data, pBuf, MAX_OUT_MESSAGE_LENGTH);
  msg.ttl = 0;
  msg_out_buf[msg_out_buf_write_ptr++] = msg;
  msg_out_buf_write_ptr %= OUT_MESSAGE_LEN;
  return TRUE;
}

bool ProcessOutputSerialMsg() {
  // Send output serial msg
  while( msg_out_buf_read_ptr != msg_out_buf_write_ptr) {
    SendSerialMessage(msg_out_buf[msg_out_buf_read_ptr++].data,MAX_OUT_MESSAGE_LENGTH);
    msg_out_buf_read_ptr %= OUT_MESSAGE_LEN;
  }
  return TRUE;
}

bool SendSerialMessage(uint8_t *pBuf, const uint8_t nLen) {
  // Construct serial message
  /// Head
  Uart2SendByte(RFS_MESSAGE_HEAD_0);
  Uart2SendByte(RFS_MESSAGE_HEAD_1);
  /// Length
  Uart2SendByte(nLen + 1);
  /// Data
  Usart2SendBuffer(pBuf, nLen);
  /// Checksum
  uint16_t check_sum = nLen + 1;
  for( uint8_t i = 0; i < nLen; i++ ) {
    check_sum += pBuf[i];
  }
  Uart2SendByte((uint8_t)(check_sum / 256));
  Uart2SendByte((uint8_t)(check_sum % 256));
  return TRUE;
}