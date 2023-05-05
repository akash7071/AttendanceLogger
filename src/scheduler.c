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
#include "ble_device_type.h"
#include "gatt_db.h"
#define I2C_TRANSFER_WAIT   10800
#include "lcd.h"
#include "sl_udelay.h"
#include "uart.h"
#define SECOND 1000*1000
#define WAGES 5

// DOS: If the caller needs these, shouldn't they be defined in the .h file ???? !!!!!
#define CAPTURE_FLAG        1
#define IDENTIFY_FLAG        2

#define PB0PRESS 8
#define PB0RELEASE 16
#define PB1PRESS 32
#define PB1RELEASE 128
#define TOGGLEINDICATION 64

//#define I2C_COMPLETE_EVENT 4

ble_data_struct_t *ble_data2;

enum myStates_t
{
  IDLE,
  WAIT_FOR_FINGERPRINT,
  IDENTIFY_FINGERPRINT,
  LOG_ATTENDANCE,
  PAYROLL_DISPLAY

};

uint16_t nextEvent=1;
uint8_t i=0;
bool readOp=1;
bool writeOp=0;


uint16_t eventLog=0;



extern uint8_t receiveAck;
extern uint8_t fingerID;

/*manager
Attendance Data: 587ddc07-4953-4b87-8978-584702ca95eb
Wages: cd13adec-a913-4d57-9595-e0e0b108ae0e
PushButton: 5d3becb1-8661-4c95-a440-bb0b1fe1de5c
ECEN5823 Encryption Test
ECEN5823 Encrypted Button State: 00000002-38c8-433e-87ec-652a2d136289*/

const uint8_t TEMPERATURE_SERVICE[2] = {0x09, 0x18};
const uint8_t TEMPERATURE_CHAR[2] = {0x1C, 0x2A};


static int i_curr_state =0, m_curr_state =0, curr_state =0, p_curr_state= 0;
uint8_t current_data =0;
uint16_t timeout =0;
uint8_t *Employee_data;
uint8_t temp_test[6] = {96,97,98,90,99,6};


typedef enum {
  START_Sensor,
  WRITE,
  WAIT,
  READ,
  CALCULATE,
}states_t;

typedef enum {
  i2c_READ,
  i2c_WAIT,
  i2c_WRITE,
}i2c_states_t;

typedef enum{
  i2c_READ_EMP,
  i2c_EMP_STORE,
  i2c_MANAGER_SEND,
  WRITE_CHRACTERISTIC,
}manager_states_t;


typedef enum{
  DISCOVER_HTM,
  DISCOVER_HTM_CHARACTERISTICS,
  DISCOVER_BUTTON_SERVICES,
  DISCOVER_BUTTON_CHAR,
  DISCOVER_MANAGER_SERVICES,
  DISCOVER_MANAGER_CHAR_ATT,
  DISCOVER_MANAGER_CHAR_WAGES,
  INDICATE,
  INDICATE_BUTTON,
  CHECK,
  WAIT_FOR_CLOSE,
}discovery_state_t;



typedef enum{
  i2c_TEST_WRITE,
  i2c_TEST_WR_WAIT,
  i2c_TEST_READ,
  i2c_TEST_WAIT,
  i2c_TEST_CALCULATE,
}i2c_test_t;

typedef enum {
  PAYROLL_READ,
  PAYROLL_UPDATE,
  PAYROLL_STORE,
  PAYROLL_WAIT,
  UPDATE_RESTART,
}payroll_t;

uint32_t Curr_Events =0;
int current_state =START_Sensor;
uint32_t myEvent =0;
static uint8_t current_eid;
//uint16_t word_address = 0x9855;
uint8_t send_buffer[3];

void schedulerSetReadTemperature(){
  CORE_DECLARE_IRQ_STATE;
  // set event
  CORE_ENTER_CRITICAL();         // enter critical, turn off interrupts in NVIC
 // Curr_Events |= 1<<(event_Timer_UF); // RMW 0xb0001
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
 // Curr_Events |= 1<<(event_I2C_transferDone); // RMW 0xb0100
  sl_bt_external_signal(1<<(event_I2C_transferDone));
  CORE_EXIT_CRITICAL();          // exit critical
}

void schedulerSetButtonPressed(){
  CORE_DECLARE_IRQ_STATE;
  // set event
  CORE_ENTER_CRITICAL();         // enter critical, turn off interrupts in NVIC
  //Curr_Events |= 1<<(event_Timer_COMP1); // RMW 0xb0010
  sl_bt_external_signal(1<<(event_ButtonPressed));
  CORE_EXIT_CRITICAL();          // exit critical
}

void schedulerSetButtonReleased(){
  CORE_DECLARE_IRQ_STATE;
  // set event
  CORE_ENTER_CRITICAL();         // enter critical, turn off interrupts in NVIC
  //Curr_Events |= 1<<(event_Timer_COMP1); // RMW 0xb0010
  sl_bt_external_signal(1<<(event_ButtonReleased));
  CORE_EXIT_CRITICAL();          // exit critical
}

void schedulerSetPB1Pressed() {
  CORE_DECLARE_IRQ_STATE;
   // set event
   CORE_ENTER_CRITICAL();         // enter critical, turn off interrupts in NVIC
   //Curr_Events |= 1<<(event_Timer_COMP1); // RMW 0xb0010
   sl_bt_external_signal(1<<(event_PB1Pressed));
   CORE_EXIT_CRITICAL();          // exit critical
}

void schedulerSetPB1Released() {
  CORE_DECLARE_IRQ_STATE;
   // set event
   CORE_ENTER_CRITICAL();         // enter critical, turn off interrupts in NVIC
   //Curr_Events |= 1<<(event_Timer_COMP1); // RMW 0xb0010
   sl_bt_external_signal(1<<(event_PB1Released));
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

void Temperature_state_machine(sl_bt_msg_t *event){
#if 1
  uint16_t temperature =0;
  if(SL_BT_MSG_ID(event->header) != sl_bt_evt_system_external_signal_id)
    return;
  ble_data_struct_t *ble_temp= ble_get_data_struct();
  switch(current_state) {                                 //TUrns the Sensor on Underflow event
    case START_Sensor: if(ble_temp->connection_open ==true && ble_temp->ok_to_send_htm_indications == true) {
        if(event -> data.evt_system_external_signal.extsignals== (1<<event_Timer_UF)) {
            sensor_enable();
            current_state = WRITE;
        }
    }
    else if(ble_temp->connection_open == true && ble_temp ->ok_to_send_htm_indications == false) {
        displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
    }
    break;

    case WRITE: if(event -> data.evt_system_external_signal.extsignals == (1<<event_Timer_COMP1)) {          //Starts I2C Write after Power On Reset
        sensor_write_temperature();
        current_state = WAIT;
        sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
    }
    break;

    case WAIT: if(event -> data.evt_system_external_signal.extsignals == (1<<event_I2C_transferDone)) {      //Waits until the Write is Complete
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
#endif
}


void i2c_sequencial_test(uint32_t event)
{
  switch(curr_state) {

    case i2c_TEST_WRITE: if(event == event_Timer_UF) {
        i2c_page_write(0x02, &temp_test[0], 6);
        curr_state = i2c_TEST_WR_WAIT;
    }
    break;

    case i2c_TEST_WR_WAIT: if(event == event_I2C_transferDone) {
        timerWaitUs_irq(10800);
        curr_state = i2c_TEST_READ;
      }
      break;
    case i2c_TEST_READ: if(event == event_Timer_COMP1) {
                        i2c_sequential_read(0x02, 6);
                        curr_state = i2c_TEST_WAIT;
                    }
                    break;

    case i2c_TEST_WAIT:  if(event == event_I2C_transferDone) {
                            timerWaitUs_irq(10800);
                            curr_state = i2c_TEST_CALCULATE;
                          }
                          break;

    case i2c_TEST_CALCULATE:  if(event == event_Timer_COMP1) {
                          Employee_data = get_all_data();
                          displayPrintf(DISPLAY_ROW_TEMPVALUE, "%d %d %d %d %d %d", Employee_data[0], Employee_data[1], Employee_data[2], Employee_data[3], Employee_data[4], Employee_data[5]);
                          if(!memcmp(temp_test, Employee_data, 6)){
                              LOG_INFO("I2C Unit Test Passed\n\r");
                          }
                          else {
                              LOG_INFO("I2C Unit Test Failed\n\r");
                          }
                          curr_state = i2c_TEST_WRITE;
                    }
                    break;
  }
}



void i2c_store_attendance(sl_bt_msg_t *event) {
  ble_data_struct_t *ble_client= ble_get_data_struct();
  switch(i_curr_state){
  case i2c_READ:  if(ble_client -> isBonded && ble_client -> store_attendance  == true) {
                        current_eid = get_eid();
                        LOG_INFO("EID is %d\n\r", current_eid);
                        i2c_read(2*(current_eid));
                        i_curr_state = i2c_WAIT;
                  }
                  break;
  case i2c_WAIT: if(event -> data.evt_system_external_signal.extsignals == (1<<event_I2C_transferDone)) {
                    LOG_INFO("i2cWait state\n\r");
                    timerWaitUs_irq(10800);
                    i_curr_state = i2c_WRITE;
                  }
                  break;

  case i2c_WRITE: if(event -> data.evt_system_external_signal.extsignals == (1<<event_Timer_COMP1)) {
                     uint8_t temp_read = get_EEPROM_data();
                     LOG_INFO("STORED ATTENDANCE FOR EID %d is %d\n\r", current_eid, temp_read);
                     if(temp_read == 1) {
                         i2c_write(2*(current_eid), 2);
                         employee_report_table[current_eid].status = 2;
                         memset(&employee_report_table[current_eid].attendance_status[0], 0, 4);
                         strcpy(employee_report_table[current_eid].attendance_status, "Out");
                         displayPrintf(4, "Employee - %d Clk out", current_eid);
                     }
                     else {
                         i2c_write(2*(current_eid), 1);
                         employee_report_table[current_eid].status = 1;
                         memset(&employee_report_table[current_eid].attendance_status[0], 0, 4);
                         strcpy(employee_report_table[current_eid].attendance_status, "In ");
                         displayPrintf(4, "Employee - %d Clk in", current_eid);
                     }
                     ble_client -> store_attendance = false;
                     i_curr_state = i2c_READ;
                  }
                  break;
  }
}


void Manager_Access(sl_bt_msg_t *event) {
  ble_data_struct_t *ble_client= ble_get_data_struct();
  uint8_t sc= 0;
  switch(m_curr_state) {
    case i2c_READ_EMP: if(ble_client -> isBonded && ble_client ->managerLogin == true && ble_client -> sent_once) {
       //i2c_sequential_read(0x02,6);
       m_curr_state = i2c_EMP_STORE;
       memset(send_buffer, 0, 3);
    }
    break;
    case i2c_EMP_STORE: //if(event -> data.evt_system_external_signal.extsignals == (1<<event_I2C_transferDone)) {
        timerWaitUs_irq(10800);
        //Employee_data = get_all_data();
        send_buffer[0] = employee_report_table[0].status;
        send_buffer[1] = employee_report_table[1].status;
        send_buffer[2] = employee_report_table[2].status;
        send_buffer[3] = employee_report_table[0].Payroll;
        send_buffer[4] = employee_report_table[1].Payroll;
        send_buffer[5] = employee_report_table[2].Payroll;
        m_curr_state = i2c_MANAGER_SEND;
    //}
    break;
    case i2c_MANAGER_SEND: if(event -> data.evt_system_external_signal.extsignals == (1<<event_Timer_COMP1)){
        if(ble_client -> indication_in_flight == false) {
            LOG_INFO("Emp 1: %d , Emp 2: %d, Emp 3: %d\n\r", send_buffer[0], send_buffer[1], send_buffer[2]);
            sc = sl_bt_gatt_write_characteristic_value(ble_client -> Connection_1_Handle, ble_client -> Manager_ATT_CharacteristicHandle, 6, &send_buffer[0]);
            if(sc != SL_STATUS_OK){                                                         //Sets Parameters for Connection.
                 LOG_ERROR("sl_bt_gatt_write_characteristic_value_without_response() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
            }
            m_curr_state = i2c_READ_EMP;
            ble_client -> sent_once = false;
        }
    }
    break;
  }
}


void update_Payroll (sl_bt_msg_t *event)
{
  uint8_t headcount =0;
  ble_data_struct_t *ble_client= ble_get_data_struct();
  switch(p_curr_state){
   case PAYROLL_READ:  if(ble_client -> update_Payroll  == true) {
                        // LOG_INFO("Updating Payroll\n\r");
                         i2c_sequential_read(2,6);
                         p_curr_state = PAYROLL_UPDATE;
                   }
                   break;
   case PAYROLL_UPDATE: if(event -> data.evt_system_external_signal.extsignals == (1<<event_I2C_transferDone)) {
                     timerWaitUs_irq(10800);
                     Employee_data = get_all_data();
                     for(int i = 0; i<6; i=i+2) {                       //Payroll Update
                         if(Employee_data[i]==1){
                             headcount++;
                             Employee_data[i+1]++;
                             employee_report_table[(i/2)].Payroll = Employee_data[i+1];
                         }
                         else {
                             employee_report_table[i/2].Payroll = Employee_data[i+1];
                         }
                     }
                     for(int i = 0; i<6; i=i+2) {                        //Attendance update
                         if(Employee_data[i] != employee_report_table[i/2].status) {
                             if(Employee_data[i] == 1){
                                 employee_report_table[i/2].status = 1;
                                 memset(&employee_report_table[i/2].attendance_status[0], 0, 4);
                                 strcpy(employee_report_table[i/2].attendance_status, "In ");
                                 //LOG_INFO("Updated Emp %d status to %s\n\r", employee_report_table[i/2].employee_id, employee_report_table[i/2].attendance_status);
                             }
                             else if(Employee_data[i] == 2){
                                 employee_report_table[i/2].status = 2;
                                 memset(&employee_report_table[i/2].attendance_status[0], 0, 4);
                                 strcpy(employee_report_table[i/2].attendance_status, "Out");
                                 //LOG_INFO("Updated Emp %d status to %s\n\r", employee_report_table[i/2].employee_id, employee_report_table[i/2].attendance_status);
                             }
                             else {
                                 employee_report_table[i/2].status = 0;
                                 memset(&employee_report_table[i/2].attendance_status[0], 0, 4);
                                 strcpy(employee_report_table[i/2].attendance_status, "Abs");
                                 //LOG_INFO("Updated Emp %d status to %s\n\r", employee_report_table[i/2].employee_id, employee_report_table[i/2].attendance_status);
                             }
                         }
                     }
                     displayPrintf(5, "Headcount = %d", headcount);
                     displayPrintf(DISPLAY_ROW_8, " %d      %s     $%d", employee_report_table[0].employee_id, employee_report_table[0].attendance_status, employee_report_table[0].Payroll);
                     displayPrintf(9, " %d      %s     $%d", employee_report_table[1].employee_id, employee_report_table[1].attendance_status, employee_report_table[1].Payroll);
                     displayPrintf(10, " %d      %s     $%d", employee_report_table[2].employee_id, employee_report_table[2].attendance_status, employee_report_table[2].Payroll);
                     p_curr_state = PAYROLL_STORE;
                   }
                   break;

   case PAYROLL_STORE: if(event -> data.evt_system_external_signal.extsignals == (1<<event_Timer_COMP1)) {
                      i2c_page_write(0x02, Employee_data, 6);
                      p_curr_state = PAYROLL_WAIT;
                   }
                   break;

   case PAYROLL_WAIT: if(event -> data.evt_system_external_signal.extsignals == (1<<event_I2C_transferDone)) {
                       timerWaitUs_irq(10800);
                       p_curr_state = UPDATE_RESTART;
                     }
                     break;

   case UPDATE_RESTART: if(event -> data.evt_system_external_signal.extsignals == (1<<event_Timer_COMP1)) {
                       ble_client -> update_Payroll  = false;
                       //LOG_INFO("Updated Payroll\n\r");
                       p_curr_state = PAYROLL_READ;
                    }
                    break;
  }
}


#if (DEVICE_IS_BLE_SERVER == 0)
void discovery_state_machine(sl_bt_msg_t *event) {
  static int d_curr_state;
  uint8_t sc= 0;
  ble_data_struct_t *ble_client= ble_get_data_struct();
  switch(d_curr_state){
    case DISCOVER_HTM: if(SL_BT_MSG_ID(event->header) == sl_bt_evt_connection_opened_id){
        sc = sl_bt_gatt_discover_primary_services_by_uuid(ble_client ->Connection_1_Handle, sizeof(TEMPERATURE_SERVICE), &TEMPERATURE_SERVICE[0]);  //employee UUID
        if(sc != SL_STATUS_OK){                                                        //Starts Advertising Back again
            LOG_ERROR("sl_bt_gatt_discover_primary_services_by_uuid() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
        }
        d_curr_state = DISCOVER_HTM_CHARACTERISTICS;
    }
    break;

    case DISCOVER_HTM_CHARACTERISTICS: if(SL_BT_MSG_ID(event->header) == sl_bt_evt_gatt_procedure_completed_id) {
        sc = sl_bt_gatt_discover_characteristics_by_uuid(ble_client->Connection_1_Handle, ble_client->EmployeeServiceHandle, sizeof(TEMPERATURE_CHAR), &TEMPERATURE_CHAR[0]); //Attendance Logger Char UUID
        if(sc != SL_STATUS_OK){                                                       //Starts Advertising Back again
            LOG_ERROR("sl_bt_gatt_discover_characteristics_by_uuid() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
        }
        d_curr_state = DISCOVER_BUTTON_SERVICES;
    }
    else if(SL_BT_MSG_ID(event->header) == sl_bt_evt_connection_closed_id) {
        d_curr_state = DISCOVER_HTM;
        ble_client -> connection_open = false;
    }
    break;

    case DISCOVER_BUTTON_SERVICES: if(SL_BT_MSG_ID(event->header) == sl_bt_evt_gatt_procedure_completed_id) {
        sc = sl_bt_gatt_discover_primary_services_by_uuid(ble_client->Connection_1_Handle, sizeof(PB0_Service_UUID), &PB0_Service_UUID[0]);  ///Manager Service UUID
        if(sc != SL_STATUS_OK){                                                       //Starts scanning for service UUID
            LOG_ERROR("sl_bt_gatt_discover_characteristics_by_uuid() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
        }
        d_curr_state = DISCOVER_BUTTON_CHAR;
    }
    else if(SL_BT_MSG_ID(event->header) == sl_bt_evt_connection_closed_id) {
       d_curr_state = DISCOVER_HTM;
       ble_client -> connection_open = false;
    }
    break;

    case DISCOVER_BUTTON_CHAR: if(SL_BT_MSG_ID(event->header) == sl_bt_evt_gatt_procedure_completed_id) {
        sc = sl_bt_gatt_discover_characteristics_by_uuid(ble_client->Connection_1_Handle, ble_client->ButtonServiceHandle, sizeof(PB0_Char_UUID), &PB0_Char_UUID[0]);   //Attendance data UUID
        if(sc != SL_STATUS_OK){                                                       //Starts scanning for characteristics UUID
            LOG_ERROR("sl_bt_gatt_discover_characteristics_by_uuid() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
        }
        d_curr_state = DISCOVER_MANAGER_SERVICES;
    }
    else if(SL_BT_MSG_ID(event->header) == sl_bt_evt_connection_closed_id) {
       d_curr_state = DISCOVER_HTM;
       ble_client -> connection_open = false;
    }
    break;

    case DISCOVER_MANAGER_SERVICES: if(SL_BT_MSG_ID(event->header) == sl_bt_evt_gatt_procedure_completed_id) {
        sc = sl_bt_gatt_discover_primary_services_by_uuid(ble_client->Connection_1_Handle, sizeof(MANAGER_S), &MANAGER_S[0]);  ///Manager Service UUID
        if(sc != SL_STATUS_OK){                                                       //Starts scanning for service UUID
            LOG_ERROR("sl_bt_gatt_discover_characteristics_by_uuid() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
        }
        d_curr_state = DISCOVER_MANAGER_CHAR_ATT;
    }
    else if(SL_BT_MSG_ID(event->header) == sl_bt_evt_connection_closed_id) {
       d_curr_state = DISCOVER_HTM;
       ble_client -> connection_open = false;
    }
    break;

    case DISCOVER_MANAGER_CHAR_ATT: if(SL_BT_MSG_ID(event->header) == sl_bt_evt_gatt_procedure_completed_id) {
        sc = sl_bt_gatt_discover_characteristics_by_uuid(ble_client->Connection_1_Handle, ble_client->ManagerServiceHandle, sizeof(ATTENDANCE_DATA_C), &ATTENDANCE_DATA_C[0]);   //Attendance data UUID
        if(sc != SL_STATUS_OK){                                                       //Starts scanning for characteristics UUID
            LOG_ERROR("sl_bt_gatt_discover_characteristics_by_uuid() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
        }
        d_curr_state = DISCOVER_MANAGER_CHAR_WAGES;
    }
    else if(SL_BT_MSG_ID(event->header) == sl_bt_evt_connection_closed_id) {
       d_curr_state = DISCOVER_HTM;
       ble_client -> connection_open = false;
    }
    break;

    case DISCOVER_MANAGER_CHAR_WAGES: if(SL_BT_MSG_ID(event->header) == sl_bt_evt_gatt_procedure_completed_id) {
        sc = sl_bt_gatt_discover_characteristics_by_uuid(ble_client->Connection_1_Handle, ble_client->ManagerServiceHandle, sizeof(WAGES_CHARACTERISTIC), &WAGES_CHARACTERISTIC[0]);   //Attendance data UUID
        if(sc != SL_STATUS_OK){                                                       //Starts scanning for characteristics UUID
            LOG_ERROR("sl_bt_gatt_discover_characteristics_by_uuid() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
        }
        d_curr_state = INDICATE;
    }
    else if(SL_BT_MSG_ID(event->header) == sl_bt_evt_connection_closed_id) {
       d_curr_state = DISCOVER_HTM;
       ble_client -> connection_open = false;
    }
    break;

    case INDICATE: if(SL_BT_MSG_ID(event->header) == sl_bt_evt_gatt_procedure_completed_id) {
        sc = sl_bt_gatt_set_characteristic_notification(ble_client->Connection_1_Handle, ble_client->EmployeeCharacteristicHandle, gatt_indication); //Indicate char for attendance logger
        if(sc != SL_STATUS_OK){                                                       //Starts Advertising Back again
            LOG_ERROR("sl_bt_gatt_set_characteristic_notification() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
        }
        d_curr_state = INDICATE_BUTTON;
        ble_client -> ok_to_send_htm_indications = true;
        displayPrintf(3, "Handling Indications");
    }
    else if(SL_BT_MSG_ID(event->header) == sl_bt_evt_connection_closed_id) {
         d_curr_state = DISCOVER_HTM;
         ble_client -> connection_open = false;
    }
    break;

    case INDICATE_BUTTON: if(SL_BT_MSG_ID(event->header) == sl_bt_evt_gatt_procedure_completed_id) {
        sc = sl_bt_gatt_set_characteristic_notification(ble_client->Connection_1_Handle, ble_client->ButtonCharacteristicHandle, gatt_indication);  //Indicate char for Manager Attendance data
        if(sc != SL_STATUS_OK) {                                                       //Starts Advertising Back again
            LOG_ERROR("sl_bt_gatt_set_characteristic_notification() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
        }
        d_curr_state = CHECK;
        ble_client -> ok_to_send_button_indications = true;
    }
    else if(SL_BT_MSG_ID(event->header) == sl_bt_evt_connection_closed_id) {
         d_curr_state = DISCOVER_HTM;
         ble_client -> connection_open = false;
    }
    break;

    case CHECK: if(event == (sl_bt_msg_t *)sl_bt_evt_gatt_procedure_completed_id) {
        d_curr_state = WAIT_FOR_CLOSE;
    }
    else if(SL_BT_MSG_ID(event->header) == sl_bt_evt_connection_closed_id) {
         d_curr_state = DISCOVER_HTM;
         ble_client -> connection_open = false;
     }
    break;

    case WAIT_FOR_CLOSE: if(ble_client -> connection_open == 0){
        d_curr_state = DISCOVER_HTM;
    }
    break;
  }
}
#endif

#if DEVICE_IS_BLE_SERVER
/**************************************************************************//**
 * Function to set the event for read temperature every 3s
 *****************************************************************************/


/**************************************************************************//**
 * Function to set the event for button press
 *****************************************************************************/
void SetPB0Press()
{CORE_DECLARE_IRQ_STATE;
CORE_ENTER_CRITICAL();
  sl_bt_external_signal(PB0PRESS);
  CORE_EXIT_CRITICAL();
}



/**************************************************************************//**
 * Function to set the event for button release
 *****************************************************************************/
void SetPB0Release()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  sl_bt_external_signal(PB0RELEASE);
  CORE_EXIT_CRITICAL();
}



/**************************************************************************//**
 * Function to set the event for button press
 *****************************************************************************/
void SetPB1Press()
{CORE_DECLARE_IRQ_STATE;
CORE_ENTER_CRITICAL();
  sl_bt_external_signal(PB1PRESS);
  CORE_EXIT_CRITICAL();
}



/**************************************************************************//**
 * Function to set the event for button release
 *****************************************************************************/
void SetPB1Release()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  sl_bt_external_signal(PB1RELEASE);
  CORE_EXIT_CRITICAL();
}


/**************************************************************************//**
 * Function to set the event for indication toggle
 *****************************************************************************/
void setEventToggleIndication()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  sl_bt_external_signal(TOGGLEINDICATION);
  CORE_EXIT_CRITICAL();
}




void setCaptureEvent()
{

  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  sl_bt_external_signal(CAPTURE_FLAG);
  CORE_EXIT_CRITICAL();

 }



void setIdentifyEvent()
{

  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  sl_bt_external_signal(IDENTIFY_FLAG);
  CORE_EXIT_CRITICAL();

 }




void sendPayrollIndication()
{
  //gpioLed1SetOff();
  uint8_t payRollID=0x05;
  sl_status_t sc = sl_bt_gatt_server_write_attribute_value(
      gattdb_wages, // handle from gatt_db.h
    0,                              // offset
    sizeof(payRollID), // length
    &payRollID);    // pointer to buffer where data is

  if (sc != SL_STATUS_OK)
           LOG_ERROR("GATT DB WRITE ERROR");

  if(ble_data2->connection_open==1
      && ble_data2->indication_in_flight==0 && ble_data2->ok_to_send_htm_indications)
    {
      sc = sl_bt_gatt_server_send_indication(ble_data2->connectionHandle,
                                                   gattdb_wages,
                                                   sizeof(payRollID),
                                                   &payRollID);
      ble_data2->indication_in_flight=1;
      if (sc != SL_STATUS_OK)
        gpioLed0SetOff();

    }
}


void sendManagerIndication()
{
  uint8_t managerID=0x04;
  sl_status_t sc = sl_bt_gatt_server_write_attribute_value(
      gattdb_employee_id_s, // handle from gatt_db.h
    0,                              // offset
    sizeof(managerID), // length
    &managerID);    // pointer to buffer where data is

  if (sc != SL_STATUS_OK)
           LOG_ERROR("GATT DB WRITE ERROR");

  if(ble_data2->connection_open==1
      && ble_data2->indication_in_flight==0 && ble_data2->ok_to_send_htm_indications)
    {
      sc = sl_bt_gatt_server_send_indication(ble_data2->connectionHandle,
                                             gattdb_employee_id_s,
                                                   sizeof(managerID),
                                                   &managerID);
      ble_data2->indication_in_flight=1;
      if (sc != SL_STATUS_OK)
               LOG_ERROR("GATT INDICATION WRITE ERROR");

    }
}

void sendEmployeeIndication(uint8_t fingerID)
{

    sl_status_t sc = sl_bt_gatt_server_write_attribute_value(
    gattdb_employee_id_s, // handle from gatt_db.h
    0,                              // offset
    sizeof(fingerID), // length
    &fingerID);    // pointer to buffer where data is

  if (sc != SL_STATUS_OK)
           LOG_ERROR("GATT DB WRITE ERROR");

  if(ble_data2->connection_open==1
      && ble_data2->indication_in_flight==0 && ble_data2->ok_to_send_htm_indications)
    {
      sc = sl_bt_gatt_server_send_indication(ble_data2->connectionHandle,
                                             gattdb_employee_id_s,
                                                   sizeof(fingerID),
                                                   &fingerID);
      ble_data2->indication_in_flight=1;
      if (sc != SL_STATUS_OK)
               LOG_ERROR("GATT INDICATION WRITE ERROR");

    }
}


void stateMachine(sl_bt_msg_t *evt)
{

  ble_data2=getBleDataPtr();
  if(ble_data2->connection_open==1 && ble_data2->isBonded && ble_data2->ok_to_send_htm_indications)
    {

      enum myStates_t currentState=IDLE;
      static enum myStates_t nextState = IDLE;
      uint32_t event=evt->data.evt_system_external_signal.extsignals;
      currentState = nextState;

       switch(currentState)
            {

            case IDLE:
                  capturePrint();                 //start capturing finger prints
                  displayPrintf(3, "Ready",0);
                  nextState=WAIT_FOR_FINGERPRINT;

              break;


            case WAIT_FOR_FINGERPRINT:

              if(event == 1)
                {

                  identifyFinger();
                  nextState=IDENTIFY_FINGERPRINT;

                }
              break;


            case IDENTIFY_FINGERPRINT:

              if( (event == 2) && (fingerID==0x00))
                {

                  sendManagerIndication();
                  //log out
                  if(ble_data2->managerLoggedIn)
                    {
                      displayPrintf(3, "Ready",0);
                      displayPrintf(4, "", 0);
                      displayPrintf(5, " ", 0);
                      displayPrintf(6, "    ",0);
                      displayPrintf(7, "",0);
                      displayPrintf(DISPLAY_ROW_8, "", 0);
                      displayPrintf(DISPLAY_ROW_9, " ", 0);
                      displayPrintf(DISPLAY_ROW_10, "    ", 0);

                      ble_data2->managerLoggedIn=0;
                      nextState=IDLE;
                      ble_data2 -> payroll =0;
                    }

                  //log in
                  else
                    {
                      displayPrintf(3, "Manager Access",0);// change row
                      displayPrintf(4, "Getting Data...",0);

                      ble_data2->managerLoggedIn=1;
                      nextState=PAYROLL_DISPLAY;
                    }



                }
              //labor access
              else if( (event == 2) && (fingerID<=0x03))
                {
                  if(ble_data2->managerLoggedIn)
                    {
                      displayPrintf(4, "Not allowed",0);
                      break;
                    }

                  nextState=LOG_ATTENDANCE;
                  sendEmployeeIndication(fingerID);

                }
              else if((event == 2) && (fingerID>0x03))
                {
                  displayPrintf(3, "Unrecognized",0);// change row
                  displayPrintf(4, "Try Again",0);
                  nextState=IDLE;
                }

              break;


            case PAYROLL_DISPLAY:
              if((SL_BT_MSG_ID(evt->header)==sl_bt_evt_gatt_server_attribute_value_id))
                {
                  displayPrintf(4, "",0);
                  nextState=IDLE;
                }
              break;


            case LOG_ATTENDANCE:

                {
                  displayPrintf(4, "",0);//print current state
                  displayPrintf(3, "Employee %d Logged",fingerID);
                  sl_udelay_wait(SECOND);
                  sl_udelay_wait(SECOND);
                  sl_udelay_wait(SECOND);
                  sl_udelay_wait(SECOND);
                  sl_udelay_wait(SECOND);
                  displayPrintf(3, "",0);
                  sl_udelay_wait(1000);
                  displayPrintf(3, "Ready",0);
                  nextState=IDLE;
                }
              break;

            } // switch
    }//if

}
#endif
