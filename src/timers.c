
 /******************************************************************************
 * Date:        02-25-2022
 * Author:      Harsh Beriwal (harsh.beriwal@colorado.edu)
 * Description: This code was created for configuring the LETIMER0 with COMP0 and
 *              COMP1 interrupt.
 *              It is to be used only for ECEN 5823 "IoT Embedded Firmware".
 *
 ******************************************************************************/

#include <src/oscillators.h>
#include <src/timers.h>
#include "em_letimer.h"
#include "app.h"
#define INCLUDE_LOG_DEBUG   1
#include "log.h"
#if (LOWEST_ENERGY_MODE == EM3)
  #define ACTUAL_CLK_FREQ   1000
#else
  #define ACTUAL_CLK_FREQ   8192
#endif
//#define COMP1_CNT     ((LETIMER_ON_TIME_MS*ACTUAL_CLK_FREQ)/1000)
#define COMP0_CNT     ((LETIMER_PERIOD_MS*ACTUAL_CLK_FREQ)/1000)

uint32_t temp =0, current_tick =0, wait_ticks =0;
uint32_t COMP1_CNT =  ((LETIMER_ON_TIME_MS*ACTUAL_CLK_FREQ)/1000);
void LETIMER0_init() {
    const LETIMER_Init_TypeDef timer = {
        false,
        true,
        true,
        false,
        0,
        0,
        letimerUFOANone,
        letimerUFOANone,
        letimerRepeatFree,
        COMP0_CNT,
    };
    LETIMER_Init(LETIMER0, &timer);
    LETIMER_CompareSet(LETIMER0, 0, COMP0_CNT);  // COMP0
    LETIMER_IntClear (LETIMER0, 0xFFFFFFFF);     // Clear all IRQ flags in the LETIMER0 IF status register
    //LETIMER_IntEnable (LETIMER0, LETIMER_IEN_UF);//  Enabling UF Interrupt
    LETIMER_Enable(LETIMER0, true);              //  Turning on LETIMER0
}

void timerWaitUs_irq(int wait) {
   //***********************RANGE_CHECK IMPLEMTATION*********************************
   if((wait < 1000) || (wait > 65535000)) {
       LOG_ERROR("Timer Wait function Range is 1ms to 65535ms at 1000Hz");
       return;
   }
   current_tick  = LETIMER_CounterGet(LETIMER0);              //Get the current tick
   wait_ticks = (wait * ACTUAL_CLK_FREQ)/1000000;             //convert wait in us to ticks
   if(current_tick < wait_ticks) {
       wait_ticks = (current_tick - wait_ticks) + COMP0_CNT;  //timer overflow condition
   }
   else {
       wait_ticks = current_tick - wait_ticks;
   }
   COMP1_CNT = wait_ticks;
   LETIMER_CompareSet(LETIMER0, 1, COMP1_CNT);                //Sets Compare1 Register
   LETIMER_IntClear(LETIMER0, LETIMER_IFC_COMP1);             //Clear if any flag is set before
   LETIMER0 -> IEN |= LETIMER_IEN_COMP1;                      //Enables Interrupt for COMP1
}

