/*
 * uart.h
 *
 *  Created on: May 4, 2023
 *      Author: akash
 */

#ifndef SRC_UART_H_
#define SRC_UART_H_


#define GT521F52_UART USART0
#define GT521F52_TX_PORT gpioPortA
#define GT521F52_TX_PIN 0       //default 0
#define GT521F52_RX_PORT gpioPortA
#define GT521F52_RX_PIN 3       //default 1
// Header Of Cmd and Ack Packets
#define STX1        0x55  //Header1
#define STX2        0xAA  //Header2
#define PKT_ERR_START -500
#define PKT_COMM_ERR  PKT_ERR_START+1
#define PKT_HDR_ERR   PKT_ERR_START+2
#define PKT_DEV_ID_ERR  PKT_ERR_START+3
#define PKT_CHK_SUM_ERR PKT_ERR_START+4
#define PKT_PARAM_ERR PKT_ERR_START+5
#define SB_OEM_PKT_SIZE     12
#define COMM_DEF_TIMEOUT 10000
#define wDevID 0x0001




#define UART_RX_PORT gpioPortA
#define UART_RX_PIN  3

#define BAUDRATE     9600

#define TIMER_FREQ   1000000
#define BIT_TIME_US  (1000000 / BAUDRATE)



// Structure Of Cmd and Ack Packets
typedef struct {
  uint8_t  Head1;
  uint8_t  Head2;
  uint16_t  wDevId;
  uint32_t   nParam;
  uint16_t  wCmd;// or nAck
  uint16_t  wChkSum;
} SB_OEM_PKT;

uint8_t receive_ack(int len);
uint16_t oemp_CalcChkSumOfCmdAckPkt( SB_OEM_PKT* pPkt );
void init_leUART();
void init_uart(void);
uint8_t send_cmd(uint8_t* cmd, uint32_t len);
int oemp_SendCmdOrAck(uint32_t nParam, uint16_t wCmdOrAck );
void cmosLED(bool state);
uint8_t receive_ack(int len);
void fpInit();
void identifyFinger();
void capturePrint();


#endif /* SRC_UART_H_ */
