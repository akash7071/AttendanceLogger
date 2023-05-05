/*
 * uart.c
 *
 *  Created on: May 4, 2023
 *      Author: akash
 */

#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_letimer.h"
#include "src/uart.h"
#include "em_usart.h"
#include "sl_iostream.h"
#include "sl_iostream_uart.h"
#include "sl_iostream_usart.h"
#include "sl_udelay.h"
#include "src/scheduler.h"
#define BUFLEN  80

uint8_t m_buffer[BUFLEN];
 uint8_t receiveAck=0;
 uint8_t fingerID=0;
 uint32_t gCommTimeOut = COMM_DEF_TIMEOUT;



uint16_t oemp_CalcChkSumOfCmdAckPkt( SB_OEM_PKT* pPkt )
{
  uint16_t wChkSum = 0;
  uint8_t* pBuf = (uint8_t*)pPkt;
  //uint8_t* pBuf = (uint16_t*)pPkt;
  unsigned int i;

  for(i=0;i<(sizeof(SB_OEM_PKT)-2);i++)
    wChkSum += pBuf[i];
  return wChkSum;
}


void init_leUART()
{
  GPIO_PinModeSet(UART_RX_PORT, UART_RX_PIN, gpioModeInput, 0);    // RX


  // Configure LETIMER for UART bit-banging
//  CMU_ClockEnable(cmuClock_LETIMER0, true);
//  LETIMER_Init_TypeDef letimerInit = LETIMER_INIT_DEFAULT;
//  letimerInit.repMode = letimerRepeatFree;
//  LETIMER_Init(LETIMER0, &letimerInit);
//  uint32_t letimerComp = BIT_TIME_US * 2; // Trigger at middle of each bit
//  LETIMER_CompareSet(LETIMER0, 0, letimerComp);
//  LETIMER_IntEnable(LETIMER0, LETIMER_IEN_COMP0);
//  NVIC_EnableIRQ(LETIMER0_IRQn);

  // Enable global interrupts
  __enable_irq();
}


void init_uart(void)


{

    CMU_ClockEnable(cmuClock_HFPER, true);
    CMU_ClockEnable(cmuClock_USART0, true);
    CMU_ClockEnable(cmuClock_GPIO, true);

    USART_InitAsync_TypeDef initAsync = USART_INITASYNC_DEFAULT;
    initAsync.baudrate = 9600;
    USART_InitAsync(USART0, &initAsync);




    USART0->ROUTEPEN |= USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN;
//    USART0->ROUTELOC0 = (USART0->ROUTELOC0
//                         & ~(_USART_ROUTELOC0_TXLOC_MASK
//                             | _USART_ROUTELOC0_RXLOC_MASK))
//                        | (GT521F52_TX_PIN << _USART_ROUTELOC0_TXLOC_SHIFT)
//                        | (GT521F52_RX_PIN << _USART_ROUTELOC0_RXLOC_SHIFT);

    USART0->ROUTELOC0 =0x00<<_USART_ROUTELOC0_TXLOC_SHIFT | 0x02<<_USART_ROUTELOC0_RXLOC_SHIFT;
    //USART0->ROUTELOC0 =0x03<<_USART_ROUTELOC0_TXLOC_SHIFT;

    GPIO_PinModeSet((GPIO_Port_TypeDef)AF_USART0_TX_PORT(GT521F52_TX_PIN), AF_USART0_TX_PIN(GT521F52_TX_PIN), gpioModePushPull, 1);
    GPIO_PinModeSet((GPIO_Port_TypeDef)AF_USART0_RX_PORT(GT521F52_RX_PIN), AF_USART0_RX_PIN(GT521F52_RX_PIN), gpioModeInput, 0);
    //GPIO_PinModeSet((GPIO_Port_TypeDef)AF_USART0_RX_PORT(GT521F52_RX_PIN), AF_USART0_RX_PIN(GT521F52_RX_PIN), gpioModeInputPull, 0);

    GPIO_PinModeSet(UART_RX_PORT, UART_RX_PIN, gpioModeInput, 0);    // RX
}

uint8_t send_cmd(uint8_t* cmd, uint32_t len)
{
  uint8_t temp;
    for (uint32_t i = 0; i < len; i++) {
        USART_Tx(USART0, cmd[i]);
        //while (!(USART0->STATUS & USART_STATUS_TXC));
        temp++;
    }
    return temp;
}


int oemp_SendCmdOrAck(uint32_t nParam, uint16_t wCmdOrAck )
{
  SB_OEM_PKT pkt;
  int nSentBytes;


  pkt.Head1 = (uint8_t)STX1;
  pkt.Head2 = (uint8_t)STX2;
  pkt.wDevId = wDevID;
  pkt.wCmd = wCmdOrAck;
  pkt.nParam = nParam;
  pkt.wChkSum = oemp_CalcChkSumOfCmdAckPkt( &pkt );

  //nSentBytes = comm_send( (uint8_t*)&pkt, SB_OEM_PKT_SIZE, gCommTimeOut );
  nSentBytes = send_cmd((uint8_t*)&pkt, SB_OEM_PKT_SIZE);
  if( nSentBytes != SB_OEM_PKT_SIZE )
    return PKT_COMM_ERR;

  return 0;
}

void cmosLED(bool state)
{
  USART0->CMD |= 1 <_USART_CMD_CLEARRX_SHIFT;
  uint32_t paramLED = 0x00000000;
  paramLED |= state;
  oemp_SendCmdOrAck(paramLED, 0x0012);
  receive_ack(12);
  sl_udelay_wait(50000);
}
uint8_t receive_ack(int len)
{

  for(int i=0;i<len;i++)
    {
      //m_buffer[i] = USART_RxDataGet(USART0);
      m_buffer[i] = USART_Rx(USART0);
    }

  //gpioLed1SetOn();
  USART0->CMD |= 1 <_USART_CMD_CLEARRX_SHIFT;
  return m_buffer[8];

}

void fpInit()
{
  sl_udelay_wait(100000);


  USART0->CMD |= 1 <_USART_CMD_CLEARRX_SHIFT;
  oemp_SendCmdOrAck(0x00002580, 0x0004);  //{0x55, 0xAA, 0x01, 0x00, 0x80, 0x25, 0x00, 0x00, 0x04, 0x00, 0xA9, 0x01};
  receive_ack(12);
  sl_udelay_wait(42000);

  USART0->CMD |= 1 <_USART_CMD_CLEARRX_SHIFT;
  oemp_SendCmdOrAck(00000001, 0x0001); //uint8_t init_cmdb[] = {0x55, 0xAA,   0x01, 0x00,    0x01, 0x00, 0x00, 0x00,    0x01, 0x00,    0x02, 0x01};
  receive_ack(42);
  sl_udelay_wait(76000);


  USART0->CMD |= 1 <_USART_CMD_CLEARRX_SHIFT;
  oemp_SendCmdOrAck(0x00000000, 0x0006);//uint8_t init_cmdc[] = {0x55, 0xAA,   0x01, 0x00,  0x00, 0x00, 0x00, 0x00,   0x06, 0x00,   0x06, 0x01};
  receive_ack(56);
  sl_udelay_wait(100000);
  sl_udelay_wait(100000);
  sl_udelay_wait(100000);


  cmosLED(1);
//  receive_ack(12);
  cmosLED(0);
//  receive_ack(12);

}


void identifyFinger()
{

  USART0->CMD |= 1 <_USART_CMD_CLEARRX_SHIFT;
  oemp_SendCmdOrAck(0x00000000, 0x0051);
  //sl_udelay_wait(60000);


  for(int i=0;i<12;i++)
    {
      //m_buffer[i] = USART_RxDataGet(USART0);
      m_buffer[i] = USART_Rx(USART0);
    }

  //gpioLed1SetOn();
  USART0->CMD |= 1 <_USART_CMD_CLEARRX_SHIFT;

  if(m_buffer[8]==0x30)
    {
      fingerID=m_buffer[4];

    }
  else
    {
      fingerID=0xFF;
    }


  cmosLED(0);
  setIdentifyEvent();

}

void capturePrint()
{
  uint8_t attempts;
CAPTUREAGAIN:
  attempts=0;
  //turn on LED
  cmosLED(1);

  //get enroll count
  USART0->CMD |= 1 <_USART_CMD_CLEARRX_SHIFT;
  oemp_SendCmdOrAck(0x00000000, 0x0020);
  receive_ack(12);
  sl_udelay_wait(100000);

  //capture prints
  USART0->CMD |= 1 <_USART_CMD_CLEARRX_SHIFT;
  oemp_SendCmdOrAck(0x00000000, 0x0060);
  //sl_udelay_wait(60000);
  receiveAck=receive_ack(12);
  //sl_udelay_wait(10000);

  //if no finger pressed, repeat above step

  while( receiveAck!=0x30)
    {
      oemp_SendCmdOrAck(0x00000000, 0x0060);
      //sl_udelay_wait(30000);
      receiveAck=receive_ack(12);
      attempts++;
      if(attempts>15)
        {

          cmosLED(0);
          goto CAPTUREAGAIN;
        }
    }
  setCaptureEvent();



}

