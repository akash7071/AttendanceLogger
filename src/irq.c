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

uint32_t flags =0;

void LETIMER0_IRQHandler (void) {
  flags = LETIMER_IntGetEnabled(LETIMER0);         //Get interrupt flags detail
  if(flags &LETIMER_IF_COMP0){                      //Checking if COMP0 caused the int
      LETIMER_IntClear(LETIMER0,LETIMER_IF_COMP0);  //Clearing COMP0 interrupt
      gpioLed0SetOff();                             //Turn LED0 OFF
  }
  if(flags &LETIMER_IF_COMP1){                      //Checking if COMP1 caused the int
      LETIMER_IntClear(LETIMER0,LETIMER_IF_COMP1);  //Clearing COMP1 interrupt
      gpioLed0SetOn();                              //Turn LED1 OFF
  }
}

