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
#include "gpio.h"
#include "i2c.h"
#include "timers.h"
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

#define I2C_TRANSFER_WAIT   10800

typedef enum {
  START_Sensor,
  WRITE,
  WAIT,
  READ,
  CALCULATE,
}states_t;

uint32_t Curr_Events =0;
int current_state =START_Sensor;
uint32_t myEvent =0;

void schedulerSetReadTemperature(){
  CORE_DECLARE_IRQ_STATE;
  // set event
  CORE_ENTER_CRITICAL();         // enter critical, turn off interrupts in NVIC
  Curr_Events |= 1<<(event_Timer_UF); // RMW 0xb0001
  CORE_EXIT_CRITICAL();          // exit critical
}

void schedulerSetWaitDone(){
  CORE_DECLARE_IRQ_STATE;
  // set event
  CORE_ENTER_CRITICAL();         // enter critical, turn off interrupts in NVIC
  Curr_Events |= 1<<(event_Timer_COMP1); // RMW 0xb0010
  CORE_EXIT_CRITICAL();          // exit critical
}

void schedulerSetI2Ctransfer() {
  CORE_DECLARE_IRQ_STATE;
  // set event
  CORE_ENTER_CRITICAL();         // enter critical, turn off interrupts in NVIC
  Curr_Events |= 1<<(event_I2C_transferDone); // RMW 0xb0100
  CORE_EXIT_CRITICAL();          // exit critical
}

uint32_t getNextEvent(){
  uint32_t get_event =3;
  if(Curr_Events & (1<<event_Timer_UF)){                //Checking for Underflow event
      get_event = event_Timer_UF;
  }
  else if(Curr_Events & (1<<event_Timer_COMP1)){        //Checking for COMP1 compare event
      get_event = event_Timer_COMP1;
  }
  else if(Curr_Events & (1<<event_I2C_transferDone)){   //Checking for I2CTransfer is completion
      get_event = event_I2C_transferDone;
  }
  CORE_DECLARE_IRQ_STATE;
  // set event
  CORE_ENTER_CRITICAL();         // enter critical, turn off interrupts in NVIC
  Curr_Events &= ~(1<<get_event); // Clearing the event from all the event as it will be processed next
  CORE_EXIT_CRITICAL();          // exit critical
  return get_event;
}

void Temperature_state_machine(int event){

  uint8_t temperature =0;
  switch(current_state) {                                 //TUrns the Sensor on Underflow event
    case START_Sensor: if(event == event_Timer_UF){
        sensor_enable();
        current_state = WRITE;
    }
    break;
    case WRITE: if(event == event_Timer_COMP1){          //Starts I2C Write after Power On Reset
        sensor_write_temperature();
        current_state = WAIT;
        sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
    }
    break;
    case WAIT: if(event == event_I2C_transferDone){      //Waits until the Write is Complete
        NVIC_DisableIRQ(I2C0_IRQn);
        sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
        timerWaitUs_irq(I2C_TRANSFER_WAIT);
        current_state = READ;
    }
    break;
    case READ: if(event == event_Timer_COMP1){          //Sends a I2c Read to get Temperature
        I2C_read_temperature();
        current_state = CALCULATE;
        sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
    }
    break;
    case CALCULATE: if(event == event_I2C_transferDone){    //Processes and calculates temperature
        gpioSi7021Disable();                                  //Disable the SENSOR_ENABLE
        NVIC_DisableIRQ(I2C0_IRQn);
        sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
        temperature = calculate_temperature();
        LOG_INFO("Temperature: %d C\n\r", temperature);
        current_state = START_Sensor;
    }
    break;
  }
}

