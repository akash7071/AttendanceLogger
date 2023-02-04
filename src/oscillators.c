/******************************************************************************
 * Date:        02-25-2022
 * Author:      Harsh Beriwal (harsh.beriwal@colorado.edu)
 * Description: This file initializes the clocks for LETIMER0
 *              It is to be used only for ECEN 5823 "IoT Embedded Firmware".
 *
 ******************************************************************************/

#include <src/oscillators.h>
#include <stdio.h>
#include <stdint.h>
#include "em_cmu.h"
#include "app.h"

void init_Clock() {
#if (LOWEST_ENERGY_MODE == EM3)
  CMU_OscillatorEnable(cmuOsc_ULFRCO,true,true);        //Configuring ULFRCO for EM3
  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_ULFRCO);
  CMU_ClockEnable(cmuClock_LFA, true);
  CMU_ClockDivSet(cmuClock_LETIMER0,cmuClkDiv_1);       //1000/1 = 1000 Hz
#else
  CMU_OscillatorEnable(cmuOsc_LFXO,true,true);          //Configuring LFXO for EM0 - EM2
  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
  CMU_ClockEnable(cmuClock_LFA, true);
  CMU_ClockDivSet(cmuClock_LETIMER0,cmuClkDiv_4);       //32768/4 = 8192 Hz
#endif
  CMU_ClockEnable(cmuClock_LETIMER0, true);             //Enabling LETIMER0 clock
}
