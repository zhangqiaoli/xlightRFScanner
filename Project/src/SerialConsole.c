#include "_global.h"
#include "MyMessage.h"
#include "SerialConsole.h"

bool ProcessSerialMessage(const uint8_t *pBuf, const uint8_t nLen) {
  // ToDo: parse serial protocol or use state machine
  // commands: 
  ///   Scan: start, pause, resume, stop, etc.
  ///   Config: by device id (unique id), by nodeid, by nodeid & subid, etc.
  ///   Query: state, config, etc.
  ///   Test: on, off, brightness, cct, color, special effect, relay key, etc.
  
  // example:
  uint8_t cmdType = pBuf[0];
  switch( cmdType ) {
  case 'a':              // start
    // Read parameters from message
    //memcpy(gConfig.NetworkID, ..., sizeof(gConfig.NetworkID));
    gConfig.rfChannel = 71;
    gConfig.rfDataRate = RF24_1MBPS;
    gConfig.rfPowerLevel = RF24_PA_MAX;
    startScan();
    break;
  case 'b':              // stop
    stopScan();
    break;
  case 'c':              // pause
    pauseScan();
    break;
  case 'd':              // resume
    resumeScan();
    break;
  }
  
  return TRUE;
}