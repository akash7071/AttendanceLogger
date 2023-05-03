/******************************************************************************
 * Date:        02-25-2022
 * Author:      Harsh Beriwal (harsh.beriwal@colorado.edu)
 * Description: This file includes the IRQ handler for LETIMER0.
 *              It is to be used only for ECEN 5823 "IoT Embedded Firmware".
 *
 ******************************************************************************/

#include <src/timers.h>
#include "em_letimer.h"
#include "gpio.h"
#include "i2c.h"
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"
#include "scheduler.h"
#include "em_i2c.h"
#include "app.h"
#include "lcd.h"
#include "em_gpio.h"

uint32_t flags =0, TIMER_3S_INC =0;
//static bool buttonStatus = false;
void LETIMER0_IRQHandler (void) {
  flags = LETIMER_IntGetEnabled(LETIMER0);        //Get interrupt flags detail
  if(flags &LETIMER_IF_UF){                       //Checking if UF caused the int
      LETIMER_IntClear(LETIMER0,LETIMER_IF_UF);   //Clearing UF interrupt
      TIMER_3S_INC++;
      schedulerSetReadTemperature();              //Calling the particular scheduler
  }
  if(flags & LETIMER_IEN_COMP1){
      LETIMER_IntClear(LETIMER0,LETIMER_IF_COMP1);
      schedulerSetWaitDone();
  }
}

void I2C0_IRQHandler(void){
  I2C_TransferReturn_TypeDef transferStatus;
  static int count =0;
  transferStatus = I2C_Transfer(I2C0);
  if(transferStatus == i2cTransferDone){         //If i2c transfer is done, returns 1 and sets event in the scheduler
      schedulerSetI2Ctransfer();
  }
  if(transferStatus < 0){
      count++;
      LOG_ERROR("%d. Count = %d\n\r", transferStatus, count);
  }
}

void GPIO_EVEN_IRQHandler(void) {
  uint32_t gpioInput = GPIO_IntGet();
  GPIO_IntClear(gpioInput);
  bool buttonStatus = GPIO_PinInGet(5, 6);
   if(gpioInput & 0x40) {
       if(!buttonStatus) {
           schedulerSetButtonPressed();
       }
       else {
           schedulerSetButtonReleased();
       }
   }
}

void GPIO_ODD_IRQHandler(void) {
  uint32_t gpioInput = GPIO_IntGet();
  GPIO_IntClear(gpioInput);
  bool PB1_buttonStatus = GPIO_PinInGet(5,7);
  if(gpioInput & 0x80) {
    if(!PB1_buttonStatus) {
      schedulerSetPB1Pressed();
    }
    else {
        schedulerSetPB1Released();
    }
  }

}

int letimerMilliseconds() {                      //Returns Time it is running in a multiple of 3S.
  uint8_t temp =0;
  temp = TIMER_3S_INC;
  return (temp*LETIMER_PERIOD_MS);
}

