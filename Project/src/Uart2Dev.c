#include "Uart2Dev.h"
#include "SerialConsole.h"

uint8_t uartReceiveDataBuf[MAX_UART_BUF_SIZE];
uint8_t uartDataPtr = 0;

void uart2_config(uint32_t speed)
{
  CLK_PeripheralClockConfig(CLK_PERIPHERAL_UART2, ENABLE);
  
  UART2_DeInit();
  
  GPIO_ExternalPullUpConfig(GPIOD, GPIO_PIN_5, ENABLE);//TX
  GPIO_ExternalPullUpConfig(GPIOD, GPIO_PIN_6, ENABLE);//RX
  
  UART2_Init(speed, UART2_WORDLENGTH_8D, UART2_STOPBITS_1, UART2_PARITY_NO, UART2_SYNCMODE_CLOCK_DISABLE, UART2_MODE_TXRX_ENABLE);
  
  UART2_ITConfig(UART2_IT_RXNE, ENABLE);
  
  UART2_Cmd(ENABLE);

  /* Enable general interrupts */
  enableInterrupts();
}

void Uart2SendByte(uint8_t data)
{
  // Usart2_SendData8 ((unsigned char) data);
  UART2_SendData8( data);
  
  // Loop until the end of tranmission
  while (UART2_GetFlagStatus(UART2_FLAG_TXE) == RESET);
}

uint8_t Usart2SendBuffer(uint8_t *pBuf, uint8_t ucLen)
{
  if ((!pBuf) || (!ucLen))
    return 0;
  
  for( uint8_t uc = 0; uc < ucLen; uc++ ) {
    Uart2SendByte(pBuf[uc]);
  }
  
  return 1;
}

uint8_t Uart2SendString(uint8_t *pBuf)
{
  unsigned char ucPos;
  if (!pBuf)
    return 0;
  
  ucPos = 0;
  do {
    if (pBuf[ucPos] != '\0') {
      Uart2SendByte(pBuf[ucPos ++]);
    } else 
      break;
  } while (1);
  
  return 1;
}

#define UART_STEP_WAIT_HEAD_0           0
#define UART_STEP_WAIT_HEAD_1           1
#define UART_STEP_WAIT_LEN              2
#define UART_STEP_WAIT_PAYL             3
#define UART_STEP_WAIT_CHECKSUM0        4
#define UART_STEP_WAIT_CHECKSUM1        5

uint8_t uart_step = UART_STEP_WAIT_HEAD_0;

INTERRUPT_HANDLER(UART2_RX_IRQHandler, 21)
{
  /* In order to detect unexpected events during development,
  it is recommended to set a breakpoint on the following instruction.
  */
  u8 data;
  if( UART2_GetITStatus(UART2_IT_RXNE) == SET ) {
    data = UART2_ReceiveData8();
    switch( uart_step ) {
    case UART_STEP_WAIT_HEAD_0:
      if( data == RFS_MESSAGE_HEAD_0 ) uart_step = UART_STEP_WAIT_HEAD_1;
      break;
    case UART_STEP_WAIT_HEAD_1:
      if( data == RFS_MESSAGE_HEAD_1 ) uart_step = UART_STEP_WAIT_LEN;
      else {
        uartDataPtr = 0;
        uart_step = UART_STEP_WAIT_HEAD_0;
      }
      break;
    case UART_STEP_WAIT_LEN:
      if( data > 1 && data < MAX_UART_BUF_SIZE ) {
        uartDataPtr = 0;
        uartReceiveDataBuf[uartDataPtr++] = data;
        uart_step = UART_STEP_WAIT_PAYL;
      } else {
        uartDataPtr = 0;
        uart_step = UART_STEP_WAIT_HEAD_0;
      }
      break;
    case UART_STEP_WAIT_PAYL:   
      uartReceiveDataBuf[uartDataPtr++] = data;    
      if( uartDataPtr == uartReceiveDataBuf[0] ) uart_step = UART_STEP_WAIT_CHECKSUM0;
      break;
    case UART_STEP_WAIT_CHECKSUM0:
      uartReceiveDataBuf[uartDataPtr++] = data;
      uart_step = UART_STEP_WAIT_CHECKSUM1;
      break;
    case UART_STEP_WAIT_CHECKSUM1:
      uartReceiveDataBuf[uartDataPtr++] = data;
      uart_step = UART_STEP_WAIT_CHECKSUM1;
      // Got an entire message
      ProcessSerialMessage(uartReceiveDataBuf, uartDataPtr);
      uartDataPtr = 0;
      uart_step = UART_STEP_WAIT_HEAD_0;
      break;
    default:
      uartDataPtr = 0;
      uart_step = UART_STEP_WAIT_HEAD_0;
      break;
    }
    
    // Echo back
    // Uart2SendByte(data);
    UART2_ClearITPendingBit(UART2_IT_RXNE);
  }
}