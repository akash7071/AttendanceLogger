 /******************************************************************************
 * Date:        02-25-2022
 * Author:      Harsh Beriwal (harsh.beriwal@colorado.edu)
 * Description: This code was created for created a scheduler for handling multiple
 *              events of different types.
 *              It is to be used only for ECEN 5823 "IoT Embedded Firmware".
 *
 ******************************************************************************/


#include "scheduler.h"
#include "app_assert.h"


uint32_t Curr_Events =0;

void schedulerSetReadTemperature(){
  CORE_DECLARE_IRQ_STATE;
  // set event
  CORE_ENTER_CRITICAL();         // enter critical, turn off interrupts in NVIC
  Curr_Events |= 1<<(event_Timer_UF); // RMW 0xb0011
  CORE_EXIT_CRITICAL();          // exit critical
}

uint32_t getNextEvent(){
  uint32_t get_event =0 ;
  int i =0;
  while(Curr_Events & (1<<i)){
      get_event =i;
      i++;
  }
  CORE_DECLARE_IRQ_STATE;
    // set event
  CORE_ENTER_CRITICAL();         // enter critical, turn off interrupts in NVIC
  Curr_Events &= ~(1<<get_event); // RMW 0xb0011
  CORE_EXIT_CRITICAL();          // exit critical
  return get_event;
}


