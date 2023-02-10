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
#include "log.h"
#include "scheduler.h"

uint32_t flags =0;

void LETIMER0_IRQHandler (void) {
  flags = LETIMER_IntGetEnabled(LETIMER0);        //Get interrupt flags detail
  if(flags &LETIMER_IF_UF){                       //Checking if UF caused the int
      LETIMER_IntClear(LETIMER0,LETIMER_IF_UF);   //Clearing UF interrupt
      schedulerSetReadTemperature();              //Calling the particular scheduler
  }
}

