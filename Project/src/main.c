#include "_global.h"
#include "rf24l01.h"
#include "MyMessage.h"
#include "MyMessageQueue.h"
#include "xliNodeConfig.h"
#include "ProtocolParser.h"
#include "SerialConsole.h"
#include "Uart2Dev.h"
#include "timer_4.h"

/*
License: MIT

Auther: Baoshi Sun
Email: bs.sun@datatellit.com, bs.sun@uwaterloo.ca
Github: https://github.com/sunbaoshi1975
Please visit xlight.ca for product details

RF24L01 connector pinout:
GND    VCC
CE     CSN
SCK    MOSI
MISO   IRQ

Connections:
  PC3 -> CE
  PC4 -> CSN
  PC7 -> MISO
  PC6 -> MOSI
  PC5 -> SCK
  PC2 -> IRQ

*/

// Xlight Application Identification
#define XLA_VERSION               0x01
#define XLA_ORGANIZATION          "xlight.ca"               // Default value. Read from EEPROM

// Choose Product Name & Type
#define XLA_PRODUCT_NAME          "RFSCANNER"
#define XLA_PRODUCT_Type          1
#define XLA_PRODUCT_NODEID        NODEID_RF_SCANNER

// Default RF channel for the sensor net, 0-127
#define RF24_CHANNEL	   		71

// Window Watchdog
// Uncomment this line if in debug mode
#define DEBUG_NO_WWDG
#define WWDG_COUNTER                    0x7f
#define WWDG_WINDOW                     0x77

// System Startup Status
#define SYS_INIT                        0
#define SYS_RESET                       1
#define SYS_WAIT_COMMAND                2
#define SYS_PAUSE                       4
#define SYS_RUNNING                     5

// Timeout
#define RTE_TM_SCAN_UNIT                200   // about 2s (200 * 10ms)
#define RTE_PROBE_PER_SCAN              3     // How many probes

// MMQ Attributes
#define MAX_LEN_RMMQ                    10    // Max. length of received MMQ
#define RTE_TM_RMMQ_ITEM                80    // Timeout of received MMQ item, about 800ms (80 * 10ms)

// Unique ID
#if defined(STM8S105) || defined(STM8S005) || defined(STM8AF626x)
  #define     UNIQUE_ID_ADDRESS         (0x48CD)
#endif
#if defined(STM8S103) || defined(STM8S003) ||  defined(STM8S903)
  #define     UNIQUE_ID_ADDRESS         (0x4865)
#endif

const UC RF24_BASE_RADIO_ID[ADDRESS_WIDTH] = {0x00,0x54,0x49,0x54,0x44};

// Public variables
Config_t gConfig;
MyMessage_t sndMsg, rcvMsg;
uint8_t *psndMsg = (uint8_t *)&sndMsg;
uint8_t *prcvMsg = (uint8_t *)&rcvMsg;
bool gIsChanged = FALSE;
uint8_t _uniqueID[UNIQUE_ID_LEN];
MMQ_Queue_t rfReceivedMQ;

// Moudle variables
uint16_t mTimerScan = 0;
uint8_t mStatus = SYS_INIT;
uint8_t mutex = 0;

// Initialize Window Watchdog
void wwdg_init() {
#ifndef DEBUG_NO_WWDG  
  WWDG_Init(WWDG_COUNTER, WWDG_WINDOW);
#endif  
}

// Feed the Window Watchdog
void feed_wwdg(void) {
#ifndef DEBUG_NO_WWDG    
  uint8_t cntValue = WWDG_GetCounter() & WWDG_COUNTER;
  if( cntValue < WWDG_WINDOW ) {
    WWDG_SetCounter(WWDG_COUNTER);
  }
#endif  
}

void Flash_ReadBuf(uint32_t Address, uint8_t *Buffer, uint16_t Length) {
  assert_param(IS_FLASH_ADDRESS_OK(Address));
  assert_param(IS_FLASH_ADDRESS_OK(Address+Length));
  
  for( uint16_t i = 0; i < Length; i++ ) {
    Buffer[i] = FLASH_ReadByte(Address+i);
  }
}

void Flash_WriteBuf(uint32_t Address, uint8_t *Buffer, uint16_t Length) {
  assert_param(IS_FLASH_ADDRESS_OK(Address));
  assert_param(IS_FLASH_ADDRESS_OK(Address+Length));
  
  // Init Flash Read & Write
  FLASH_SetProgrammingTime(FLASH_PROGRAMTIME_STANDARD);
  FLASH_Unlock(FLASH_MEMTYPE_DATA);
  while (FLASH_GetFlagStatus(FLASH_FLAG_DUL) == RESET);
  
  uint8_t WriteBuf[FLASH_BLOCK_SIZE];
  uint16_t nBlockNum = (Length - 1) / FLASH_BLOCK_SIZE + 1;
  for( uint16_t block = 0; block < nBlockNum; block++ ) {
    memset(WriteBuf, 0x00, FLASH_BLOCK_SIZE);
    for( uint16_t i = 0; i < FLASH_BLOCK_SIZE; i++ ) {
      WriteBuf[i] = Buffer[block * FLASH_BLOCK_SIZE + i];
    }
    FLASH_ProgramBlock(block, FLASH_MEMTYPE_DATA, FLASH_PROGRAMMODE_STANDARD, WriteBuf);
    FLASH_WaitForLastOperation(FLASH_MEMTYPE_DATA);
  }
  
  FLASH_Lock(FLASH_MEMTYPE_DATA);
}

uint8_t *Read_UniqueID(uint8_t *UniqueID, uint16_t Length)  
{
  Flash_ReadBuf(UNIQUE_ID_ADDRESS, UniqueID, Length);
  return UniqueID;
}

bool isIdentityEmpty(const UC *pId, UC nLen)
{
  for( int i = 0; i < nLen; i++ ) { if(pId[i] > 0) return FALSE; }
  return TRUE;
}

bool isIdentityEqual(const UC *pId1, const UC *pId2, UC nLen)
{
  for( int i = 0; i < nLen; i++ ) { if(pId1[i] != pId2[i]) return FALSE; }
  return TRUE;
}

// Save config to Flash
void SaveConfig()
{
  if( gIsChanged ) {
    Flash_WriteBuf(FLASH_DATA_START_PHYSICAL_ADDRESS, (uint8_t *)&gConfig, sizeof(gConfig));
    gIsChanged = FALSE;
  }
}

// Initialize Node Address and look forward to being assigned with a valid NodeID by the SmartController
void InitNodeAddress() {
  // Whether has preset node id
  gConfig.nodeID = XLA_PRODUCT_NODEID;
  memcpy(gConfig.NetworkID, RF24_BASE_RADIO_ID, ADDRESS_WIDTH);
}

void SerialSendLog(const uint8_t level, const char *pText) {
  char pBuf[MAX_UART_BUF_SIZE];
  memset(pBuf, 0x00, MAX_UART_BUF_SIZE);
  sprintf(pBuf, "RFS log_%d: %s\n\r", level, pText);
  Uart2SendString((uint8_t *)pBuf);
}

// Change status and inform PC via UART
void ChangeStatus(uint8_t _st) {
  mStatus = _st;
  
  char pBuf[MAX_UART_BUF_SIZE];
  memset(pBuf, 0x00, MAX_UART_BUF_SIZE);
  sprintf(pBuf, "RFS status changed to %d\n\r", _st);
  Uart2SendString((uint8_t *)pBuf);
}

// Load config from Flash
void LoadConfig()
{
    // Load the most recent settings from FLASH
    Flash_ReadBuf(FLASH_DATA_START_PHYSICAL_ADDRESS, (uint8_t *)&gConfig, sizeof(gConfig));
    if( gConfig.version > XLA_VERSION || gConfig.rfPowerLevel > RF24_PA_MAX || gConfig.nodeID != XLA_PRODUCT_NODEID || gConfig.rfChannel > 127 || gConfig.rfDataRate > 2 ) {
      memset(&gConfig, 0x00, sizeof(gConfig));
      gConfig.version = XLA_VERSION;
      InitNodeAddress();
      gConfig.type = XLA_PRODUCT_Type;
      gConfig.rptTimes = 1;
      //sprintf(gConfig.Organization, "%s", XLA_ORGANIZATION);
      //sprintf(gConfig.ProductName, "%s", XLA_PRODUCT_NAME);
      gConfig.rfChannel = RF24_CHANNEL;
      gConfig.rfPowerLevel = RF24_PA_MAX;
      gConfig.rfDataRate = RF24_1MBPS;
    }
    
    // Engineering code
}

void UpdateNodeAddress(void) {
  memcpy(rx_addr, gConfig.NetworkID, ADDRESS_WIDTH);
  rx_addr[0] = gConfig.nodeID;
  memcpy(tx_addr, gConfig.NetworkID, ADDRESS_WIDTH);
  tx_addr[0] = BROADCAST_ADDRESS;
  RF24L01_setup(gConfig.rfChannel, gConfig.rfDataRate, gConfig.rfPowerLevel, 1);
}

bool WaitMutex(uint32_t _timeout) {
  while(_timeout--) {
    if( mutex > 0 ) return TRUE;
    feed_wwdg();
  }
  return FALSE;
}

// Send message and switch back to receive mode
bool SendMyMessage() {
  if( bMsgReady ) {
    
    uint8_t lv_tried = 0;
    uint16_t delay;
    while (lv_tried++ <= gConfig.rptTimes ) {
      
      mutex = 0;
      RF24L01_set_mode_TX();
      RF24L01_write_payload(psndMsg, PLOAD_WIDTH);

      WaitMutex(0x1FFFF);
      if (mutex == 1) {
        break; // sent sccessfully
      }
      
      // Repeat the message if necessary
      delay = 0xFFF;
      while(delay--)feed_wwdg();
    }
    
    // Switch back to receive mode
    bMsgReady = 0;
    RF24L01_set_mode_RX();
  }

  return(mutex > 0);
}

// Process one message at a time
bool ProcessRecvMMQ() {
  if( rfReceivedMQ.length > 0 && !rfReceivedMQ.locked ) {
    PMQNode pNode = rfReceivedMQ.pHead;
    rfReceivedMQ.locked = TRUE;
    while( pNode != 0 && pNode != rfReceivedMQ.pTail )
    {
      // Parse message
      if( pNode->ttl < RTE_TM_RMMQ_ITEM ) {
        // Transfer message to serial port
        SendSerialMessage(pNode->data, MAX_MESSAGE_LENGTH);        
        // Change TTL to make the packet as finished
        pNode->ttl = RTE_TM_RMMQ_ITEM;
        break;
      }
      pNode = pNode->pNext;
    }
    rfReceivedMQ.locked = FALSE;     
  }
  return FALSE;
}

void Check_TTL_RecvMMQ() {
  if( rfReceivedMQ.length > 0 && !rfReceivedMQ.locked ) {
    PMQNode pNode = rfReceivedMQ.pHead;
    PMQNode pTempNode;
    while( pNode != 0 && pNode != rfReceivedMQ.pTail )
    {
      if( ++pNode->ttl >= RTE_TM_RMMQ_ITEM ) {
        // Remove it
        /// notes: do we need to remove all packets of the same series???
        pTempNode = pNode;
        pNode = pNode->pNext;
        MMQ_RemoveMessage(&rfReceivedMQ, pTempNode);
      } else {
        pNode = pNode->pNext;
      }
    }    
  }
}

bool startScan() {
  // stop previous scan
  stopScan();
  if( mStatus == SYS_RUNNING || mStatus == SYS_PAUSE ) {
    stopScan();
  }
  
  // start scanning
  mTimerScan = RTE_TM_SCAN_UNIT;  
  ChangeStatus(SYS_RUNNING);
  return TRUE;
}

bool stopScan() {
  if( mStatus == SYS_RUNNING || mStatus == SYS_PAUSE ) {
    // stop scanning
    mTimerScan = 0;
    ChangeStatus(SYS_WAIT_COMMAND);
    return TRUE;
  }
  
  return FALSE;
}

bool pauseScan() {
  if( mStatus == SYS_RUNNING ) {
    // pause scanning
    ChangeStatus(SYS_PAUSE);
    return TRUE;
  }
  
  return FALSE;
}

bool resumeScan() {
  if( mStatus == SYS_PAUSE ) {
    // resume scanning
    ChangeStatus(SYS_RUNNING);
    return TRUE;
  }
  
  return FALSE;
}

int main( void ) {
  //After reset, the device restarts by default with the HSI clock divided by 8.
  //CLK_DeInit();
  /* High speed internal clock prescaler: 1 */
  CLK_SYSCLKConfig(CLK_PRESCALER_HSIDIV1);  // now: HSI=16M prescale = 1; sysclk = 16M

  // Load config from Flash
  FLASH_DeInit();
  Read_UniqueID(_uniqueID, UNIQUE_ID_LEN);
  LoadConfig();

  // Init MQ
  rfReceivedMQ.locked = 0;
  rfReceivedMQ.length = 0;
  rfReceivedMQ.maxLen = 0;
  MMQ_InitQueue(&rfReceivedMQ, MAX_LEN_RMMQ);
  
  // Init Watchdog
  wwdg_init();

  // Init serial ports
  uart2_config(9600);
  SerialSendLog(LEVEL_INFO, "scanner is starting...");
  ChangeStatus(SYS_INIT);
  
  // Init timer
  TIM4_10ms_handler = tmrProcess;
  Time4_Init();
  
  // Go on only if NRF chip is presented
  disableInterrupts();
  RF24L01_init();
  u16 timeoutRFcheck = 0;
  while(!NRF24L01_Check()) {
    if( timeoutRFcheck > 50 ) {
      WWDG->CR = 0x80;
      break;
    }
    feed_wwdg();
  }
  
  // IRQ
  NRF2401_EnableIRQ();
  
  // Wait command
  ChangeStatus(SYS_WAIT_COMMAND);
  
  while (mStatus > SYS_RESET) {
    
    // Feed the Watchdog
    feed_wwdg();
        
    // Send message if ready
    SendMyMessage();
    
    // Process received message
    ProcessRecvMMQ();
    
    // Save Config if Changed
    SaveConfig();    
  }
}

// Execute timer operations
void tmrProcess() {
  // Tick
  if( mStatus == SYS_RUNNING ) {
    if( mTimerScan > 0 ) {
      if( mTimerScan == 1 ) {
        // Finish scanning
        stopScan();
        return;
      } else if( mTimerScan == RTE_TM_SCAN_UNIT ) {
        // Setup RF for new scan
        UpdateNodeAddress();
      }
      
      if(mTimerScan % (RTE_TM_SCAN_UNIT / RTE_PROBE_PER_SCAN) == 0) {
        // Send probe message
        Msg_ProbeMsg();
      }
      mTimerScan--;
    }
  }
  
  // Check TTL
  Check_TTL_RecvMMQ();  
}

INTERRUPT_HANDLER(EXTI_PORTC_IRQHandler, 5) {
  if(RF24L01_is_data_available()) {
    //Packet was received
    RF24L01_clear_interrupts();
    RF24L01_read_payload(prcvMsg, PLOAD_WIDTH);
    bMsgReady = ParseProtocol();
    return;
  }
 
  uint8_t sent_info;
  if (sent_info = RF24L01_was_data_sent()) {
    //Packet was sent or max retries reached
    RF24L01_clear_interrupts();
    mutex = sent_info;
    return;
  }

   RF24L01_clear_interrupts();
}