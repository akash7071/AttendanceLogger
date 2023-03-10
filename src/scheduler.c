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
#include "ble.h"
#include "em_letimer.h"
#define I2C_TRANSFER_WAIT   10800
#include "lcd.h"
uint16_t TEMPERATURE_SERVICE_UUID = 0x1809;
uint16_t TEMPERATURE_CHAR_UUID    = 0x1C2A;
const uint8_t TEMPERATURE_SERVICE[2] = {0x09, 0x18};
const uint8_t TEMPERATURE_CHAR[2] = {0x1C, 0x2A};

typedef enum {
  START_Sensor,
  WRITE,
  WAIT,
  READ,
  CALCULATE,
}states_t;

typedef enum{
  DISCOVER_SERVICES,
  DISCOVER_CHARACTERISTICS,
  INDICATE,
  CHECK,
  WAIT_FOR_CLOSE,
}discovery_state_t;

uint32_t Curr_Events =0;
int current_state =START_Sensor;
uint32_t myEvent =0;

void schedulerSetReadTemperature(){
  CORE_DECLARE_IRQ_STATE;
  // set event
  CORE_ENTER_CRITICAL();         // enter critical, turn off interrupts in NVIC
  //Curr_Events |= 1<<(event_Timer_UF); // RMW 0xb0001
  sl_bt_external_signal(1<<(event_Timer_UF));
  CORE_EXIT_CRITICAL();          // exit critical
}

void schedulerSetWaitDone(){
  CORE_DECLARE_IRQ_STATE;
  // set event
  CORE_ENTER_CRITICAL();         // enter critical, turn off interrupts in NVIC
  //Curr_Events |= 1<<(event_Timer_COMP1); // RMW 0xb0010
  sl_bt_external_signal(1<<(event_Timer_COMP1));
  CORE_EXIT_CRITICAL();          // exit critical
}

void schedulerSetI2Ctransfer() {
  CORE_DECLARE_IRQ_STATE;
  // set event
  CORE_ENTER_CRITICAL();         // enter critical, turn off interrupts in NVIC
  //Curr_Events |= 1<<(event_I2C_transferDone); // RMW 0xb0100
  sl_bt_external_signal(1<<(event_I2C_transferDone));
  CORE_EXIT_CRITICAL();          // exit critical
}

/*uint32_t getNextEvent(){
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
}*/

void Temperature_state_machine(sl_bt_msg_t *event){
  uint16_t temperature =0;
  if(SL_BT_MSG_ID(event->header) != sl_bt_evt_system_external_signal_id)
    return;
  ble_data_struct_t *ble_temp= ble_get_data_struct();
  switch(current_state) {                                 //TUrns the Sensor on Underflow event
    case START_Sensor: if(ble_temp->connection_open ==true && ble_temp->ok_to_send_htm_indications == true){
        if(event -> data.evt_system_external_signal.extsignals== (1<<event_Timer_UF)) {
            sensor_enable();
            current_state = WRITE;
        }
    }
    else if(ble_temp->connection_open == true && ble_temp ->ok_to_send_htm_indications == false){
        displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
    }
    break;

    case WRITE: if(event -> data.evt_system_external_signal.extsignals == (1<<event_Timer_COMP1)){          //Starts I2C Write after Power On Reset
        sensor_write_temperature();
        current_state = WAIT;
        sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
    }
    break;

    case WAIT: if(event -> data.evt_system_external_signal.extsignals == (1<<event_I2C_transferDone)){      //Waits until the Write is Complete
        NVIC_DisableIRQ(I2C0_IRQn);
        sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
        timerWaitUs_irq(I2C_TRANSFER_WAIT);
        current_state = READ;
    }
    break;

    case READ: if(event -> data.evt_system_external_signal.extsignals == (1<<event_Timer_COMP1)){          //Sends a I2c Read to get Temperature
        I2C_read_temperature();
        current_state = CALCULATE;
        sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
    }
    break;

    case CALCULATE: if(event -> data.evt_system_external_signal.extsignals== (1<<event_I2C_transferDone)){    //Processes and calculates temperature
        NVIC_DisableIRQ(I2C0_IRQn);
        sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
        temperature = calculate_temperature();
        ble_send_indication(temperature);
        displayPrintf(DISPLAY_ROW_TEMPVALUE, "TEMP = %d", temperature);
        current_state = START_Sensor;
    }
    break;
  }
}

void discovery_state_machine(sl_bt_msg_t *event) {
  static int d_curr_state;
  uint8_t sc= 0;
  ble_data_struct_t *ble_client= ble_get_data_struct();
  switch(d_curr_state){
    case DISCOVER_SERVICES: if(SL_BT_MSG_ID(event->header) == sl_bt_evt_connection_opened_id){
        sc = sl_bt_gatt_discover_primary_services_by_uuid(ble_client ->temperatureSetHandle, sizeof(uint16_t), &TEMPERATURE_SERVICE[0]);
        if(sc != SL_STATUS_OK){                                                       //Starts Advertising Back again
            LOG_ERROR("sl_bt_gatt_discover_primary_services_by_uuid() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
        }
        d_curr_state = DISCOVER_CHARACTERISTICS;
    }
    break;

    case DISCOVER_CHARACTERISTICS: if(SL_BT_MSG_ID(event->header) == sl_bt_evt_gatt_procedure_completed_id) {
        LOG_INFO("COMING INSIDE\n\r");
        sc = sl_bt_gatt_discover_characteristics_by_uuid(ble_client->temperatureSetHandle, ble_client->serviceHandle, sizeof(uint16_t), &TEMPERATURE_CHAR[0]);
        if(sc != SL_STATUS_OK){                                                       //Starts Advertising Back again
            LOG_ERROR("sl_bt_gatt_discover_characteristics_by_uuid() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
        }
        d_curr_state = INDICATE;
    }
    else if(SL_BT_MSG_ID(event->header) == sl_bt_evt_connection_closed_id) {
        d_curr_state = DISCOVER_SERVICES;
        ble_client -> connection_open = false;
    }
    break;

    case INDICATE: if(SL_BT_MSG_ID(event->header) == sl_bt_evt_gatt_procedure_completed_id) {
        sc = sl_bt_gatt_set_characteristic_notification(ble_client->temperatureSetHandle, ble_client->characteristicHandle, gatt_indication);
        if(sc != SL_STATUS_OK){                                                       //Starts Advertising Back again
            LOG_ERROR("sl_bt_gatt_set_characteristic_notification() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
        }
        d_curr_state = CHECK;
        displayPrintf(DISPLAY_ROW_CONNECTION, "Handling Indications");
    }
    else if(SL_BT_MSG_ID(event->header) == sl_bt_evt_connection_closed_id) {
         d_curr_state = DISCOVER_SERVICES;
         ble_client -> connection_open = false;
    }
    break;

    case CHECK: if(event == (sl_bt_msg_t *)sl_bt_evt_gatt_procedure_completed_id) {
        d_curr_state = WAIT_FOR_CLOSE;
    }
    else if(SL_BT_MSG_ID(event->header) == sl_bt_evt_connection_closed_id) {
         d_curr_state = DISCOVER_SERVICES;
         ble_client -> connection_open = false;
     }
    break;

    case WAIT_FOR_CLOSE: if(SL_BT_MSG_ID(event->header) == sl_bt_evt_connection_closed_id){
        d_curr_state = DISCOVER_SERVICES;
    }
    break;
  }
}

