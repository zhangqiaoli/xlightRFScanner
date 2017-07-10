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
    gConfig.rfChannel = 71;
    gConfig.rfDataRate = RF24_1MBPS;
    gConfig.rfPowerLevel = RF24_PA_MAX;
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
  uint16_t check_sum = nLen;
  for( uint8_t i = 0; i < nLen; i++ ) {
    check_sum += pBuf[i];
  }
  Uart2SendByte((uint8_t)(check_sum / 256));
  Uart2SendByte((uint8_t)(check_sum % 256));
  return TRUE;
}