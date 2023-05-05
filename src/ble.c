 /******************************************************************************
 * Date:        02-25-2022
 * Author:      Harsh Beriwal (harsh.beriwal@colorado.edu)
 * Description: This code was created for having ble event responder and send
 *              indication and confirmation function. This code communicate
 *              with the Bluetooth Stack for both client and server.
 *              It is to be used only for ECEN 5823 "IoT Embedded Firmware".
 *
 ******************************************************************************/

#include "ble.h"
#include "gatt_db.h"
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"
#include "src/timers.h"
#include "em_letimer.h"
#include "lcd.h"
#include "ble_device_type.h"
#include "scheduler.h"
#include "gpio.h"
#include "em_gpio.h"
#include "i2c.h"

#define INTERVAL_MIN               250
#define INTERVAL_MAX               250
#define ADVERTISER_INTERVAL_MIN    (INTERVAL_MIN/0.625)
#define ADVERTISER_INTERVAL_MAX    (INTERVAL_MAX/0.625)
#define DURATION                    0
#define MAX_EVENTS                  0
#define CONNECTION_MIN_INTERVAL    (75/1.25)
#define CONNECTION_MAX_INTERVAL    (75/1.25)
#define SLAVE_LATENCY               4
#define SUPERVISION_TIMEOUT         760
#define CON_MIN_CE_LENGTH           0
#define CON_MAX_CE_LENGTH           0xFFFF
#define SCAN_INTERVAL               80      //50ms/0.625ms
#define SCAN_WINDOW                 40      //40ms/0.625ms
#define CLIENT_SUPERVISION_TIMEOUT  82      //825 ms
#define CLIENT_MAX_CE_LENGTH        5
#define QUEUE_DEPTH                (16)
#define BUFFER_SIZE                 5
#define PER_SEC_FEE                 5

#if (DEVICE_IS_BLE_SERVER == 0)
extern const uint8_t TEMPERATURE_SERVICE[2];
extern const uint8_t TEMPERATURE_CHAR[2];
#endif

uint8_t headcount = 0;
// BLE private data
ble_data_struct_t ble_data;
sl_status_t sc=0; // status code
int temperature_in_c;
uint8_t server[6] = SERVER_BT_ADDRESS;
uint8_t *temperature;
queue_struct_t   my_queue[QUEUE_DEPTH]; // the queue - an array of structs
uint32_t         wptr = 0;              // ` pointer
uint32_t         rptr = 0;              // read pointer
uint32_t    queue_length =0;
bool      isfull = false;
bool      isempty = true;
uint16_t charHandle_tmp =0;
size_t  bufferLength_tmp = 0;
uint8_t buffer[5];
uint8_t buttonStatus =0;
static uint8_t tmp_eid;



#define MINIMUM_ADVERTISING_INTERVAL 400
#define MAXIMUM_ADVERTISING_INTERVAL 400

#define CONNECTION_INTERVAL_MINIMUM 60
#define CONNECTION_INTERVAL_MAXIMUM 60
#define SLAVE_LATENCY 4


#define SOFT_TIMER_DURATION 32768
#define PB0PRESS 8
#define PB0RELEASE 16
#define PB1PRESS 32


uint32_t gattdbData;
uint32_t actual_temp_local;
uint8_t state[2];
int event;
bool read_in_flight=0;
// BLE private data
ble_data_struct_t ble_data; // this is the declaration


// function that returns a pointer to the
// BLE private data
ble_data_struct_t* getBleDataPtr()
{

    return (&ble_data);


} // getBleDataPtr()


ble_data_struct_t* ble_get_data_struct()             //Function to get a pointer to the Data Structure
{
  return (&ble_data);
}

// -----------------------------------------------
// Private function, original from Dan Walkes. I fixed a sign extension bug.
// We'll need this for Client A7 assignment to convert health thermometer
// indications back to an integer. Convert IEEE-11073 32-bit float to signed integer.
// -----------------------------------------------
static int32_t FLOAT_TO_INT32(const uint8_t *value_start_little_endian)
{
uint8_t signByte = 0;
int32_t mantissa;
// input data format is:
// [0] = flags byte
// [3][2][1] = mantissa (2's complement)
// [4] = exponent (2's complement)
// BT value_start_little_endian[0] has the flags byte
int8_t exponent = (int8_t)value_start_little_endian[4];
// sign extend the mantissa value if the mantissa is negative
if (value_start_little_endian[3] & 0x80) { // msb of [3] is the sign of the mantissa
signByte = 0xFF;
}
mantissa = (int32_t) (value_start_little_endian[1] << 0) |
(value_start_little_endian[2] << 8) |
(value_start_little_endian[3] << 16) |
(signByte << 24) ;
// value = 10^exponent * mantissa, pow() returns a double type
return (int32_t) (pow(10, exponent) * mantissa);
} // FLOAT_TO_INT32


void handle_ble_event(sl_bt_msg_t *evt) {           //Ble Event Responder
  switch (SL_BT_MSG_ID(evt->header)) {

#if DEVICE_IS_BLE_SERVER
    case sl_bt_evt_system_boot_id:
        {
          state[0]=0x00;
          sl_bt_sm_delete_bondings();
          ble_data.indication_in_flight=0;
          ble_data.connection_open=0;
          ble_data.ok_to_send_htm_indications=0;
          ble_data.ok_to_send_button_indications=0;
          ble_data.isBonded=0;
          gpioLed0SetOff();
          gpioLed1SetOff();

          sc = sl_bt_system_get_identity_address(&ble_data.myAddress, &ble_data.myAddressType);
          if (sc != SL_STATUS_OK)
            {
              LOG_ERROR("sl_bt_system_get_identity_address() returned != 0 status=0x%04x", (unsigned int) sc);
            }

          // Create an advertising set.
          sc = sl_bt_advertiser_create_set(&ble_data.advertisingSetHandle);

          // Set advertising interval to 250ms.
          sc = sl_bt_advertiser_set_timing(ble_data.advertisingSetHandle,MINIMUM_ADVERTISING_INTERVAL,MAXIMUM_ADVERTISING_INTERVAL,0,0);

          // Start general advertising and enable connections.
          sc = sl_bt_advertiser_start(ble_data.advertisingSetHandle, sl_bt_advertiser_general_discoverable,
            sl_bt_advertiser_connectable_scannable);
          if (sc != SL_STATUS_OK)
            {
              LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x", (unsigned int) sc);
            }

          displayInit();                                //init the LCD display
          displayPrintf(DISPLAY_ROW_NAME, "Attendance");
          displayPrintf(DISPLAY_ROW_BTADDR, "Monitoring");
          displayPrintf(DISPLAY_ROW_ASSIGNMENT, "Server");
          displayPrintf(4, "Advertising",0);//print current state

          sl_bt_sm_configure(0x0F, sm_io_capability_displayyesno);
          sl_status_t          timer_response;
          timer_response = sl_bt_system_set_soft_timer(SOFT_TIMER_DURATION,1,0);// set to 1s, handler 1 and recurring mode
          if (timer_response != SL_STATUS_OK) {
              LOG_ERROR("Timer soft error");
           }




        }// handle boot event

      break;






      case sl_bt_evt_connection_opened_id:
        {

          ble_data.connectionHandle=evt->data.evt_connection_opened.connection;

          sc = sl_bt_connection_set_parameters(ble_data.connectionHandle,CONNECTION_INTERVAL_MINIMUM,CONNECTION_INTERVAL_MAXIMUM,
                                          SLAVE_LATENCY, SUPERVISION_TIMEOUT,0,0xffff);
          if (sc != SL_STATUS_OK)
          {
            LOG_ERROR("sl_bt_connection_set_parameters() returned != 0 status=0x%04x", (unsigned int) sc);
          }
          sc= sl_bt_advertiser_stop(ble_data.advertisingSetHandle);
          if (sc != SL_STATUS_OK)
          {
            LOG_ERROR("sl_bt_advertiser_stop() returned != 0 status=0x%04x", (unsigned int) sc);
          }
          ble_data.connection_open=1;
          ble_data.indication_in_flight=0;

          displayPrintf(4, "Connected",0);//print current state
          //sl_bt_sm_configure(0b00001111, sm_io_capability_displayyesno);


        }// handle open event
        break;

      case sl_bt_evt_sm_confirm_bonding_id:
        ble_data.connectionHandle=evt->data.evt_sm_confirm_bonding.connection;
          sl_bt_sm_bonding_confirm(ble_data.connectionHandle,true);
          displayPrintf(5, " ",0);//print current state
          displayPrintf(6, " ",0);//print current state
          displayPrintf(9, " ",0);//print current state
          break;



      case sl_bt_evt_sm_confirm_passkey_id:
          {
            ble_data.connectionHandle=evt->data.evt_sm_confirm_passkey.connection;
          uint32_t key;
          key=evt->data.evt_sm_confirm_passkey.passkey;

          displayPrintf(5, "Passkey %d",key);//print current state
          displayPrintf(6, "Confirm with PB0",0);//print current state

          }
          break;

      case sl_bt_evt_sm_bonded_id:
          {
            ble_data.connectionHandle=evt->data.evt_sm_bonded.connection;
                  displayPrintf(5, " ",0);//print current state
                  displayPrintf(6, " ",0);//print current state
                  displayPrintf(4, "Bonded",0);//print current state
                  ble_data.isBonded=1;
          }
        break;


      case sl_bt_evt_gatt_server_attribute_value_id:

        if((evt->data.evt_gatt_server_attribute_value.attribute==gattdb_attendance_data_c) )
          {


        employee_report_table[0].status=evt->data.evt_gatt_server_attribute_value.value.data[0];
        employee_report_table[1].status=evt->data.evt_gatt_server_attribute_value.value.data[1];
        employee_report_table[2].status=evt->data.evt_gatt_server_attribute_value.value.data[2];
        employee_report_table[0].Payroll=evt->data.evt_gatt_server_attribute_value.value.data[3];
        employee_report_table[1].Payroll=evt->data.evt_gatt_server_attribute_value.value.data[4];
        employee_report_table[2].Payroll=evt->data.evt_gatt_server_attribute_value.value.data[5];
        //displayPrintf(2,"%d %d %d", employee_report_table[0].Payroll, employee_report_table[1].Payroll, employee_report_table[2].Payroll);
        for(int i=0; i<3 ;i++)
           {
             if(employee_report_table[i].status == 2)
               {
                 memset(&employee_report_table[i].attendance_status[0], 0, 11);
                 strcpy(employee_report_table[i].attendance_status, "Out");
               }
             else if(employee_report_table[i].status == 1)
               {
                 memset(&employee_report_table[i].attendance_status[0], 0, 11);
                 strcpy(employee_report_table[i].attendance_status, "In ");
                 ble_data.headcount++;
               }
             else
               {
                 memset(&employee_report_table[i].attendance_status[0], 0, 11);
                 strcpy(employee_report_table[i].attendance_status, "Abs");
               }
         }


         displayPrintf(DISPLAY_ROW_NAME, "Attendance");
         displayPrintf(DISPLAY_ROW_BTADDR, "Monitoring");
         displayPrintf(3, "Manager Access",0);// change row

         displayPrintf(5, "Headcount = %d", ble_data.headcount);
         displayPrintf(DISPLAY_ROW_8, " %d      %s     $%d", employee_report_table[0].employee_id, employee_report_table[0].attendance_status, employee_report_table[0].Payroll);
         displayPrintf(9, " %d      %s     $%d", employee_report_table[1].employee_id, employee_report_table[1].attendance_status, employee_report_table[1].Payroll);
         displayPrintf(10, " %d      %s     $%d", employee_report_table[2].employee_id, employee_report_table[2].attendance_status, employee_report_table[2].Payroll);
         displayPrintf(6, "EMPID ATTENDANCE PAY");
         displayPrintf(7, "----- ---------- ---");

         ble_data.headcount=0;
          }

        if(evt->data.evt_gatt_server_attribute_value.attribute==gattdb_wages)
          {
            employee_report_table[0].Payroll=evt->data.evt_gatt_server_attribute_value.value.data[0];
            employee_report_table[1].Payroll=evt->data.evt_gatt_server_attribute_value.value.data[1];
            employee_report_table[2].Payroll=evt->data.evt_gatt_server_attribute_value.value.data[2];



  //          displayPrintf(DISPLAY_ROW_NAME, "Attendance");
  //          displayPrintf(DISPLAY_ROW_BTADDR, "Monitoring");
  //          displayPrintf(DISPLAY_ROW_3, "Manager Access",0);// change row
            displayPrintf(4, "Headcount = %d", ble_data.headcount);
  //
            displayPrintf(6,  "EMPID ATTENDANCE    ");
            displayPrintf(7,  "----- ---------- ---");
            displayPrintf(DISPLAY_ROW_8,  " %d       %s    %d ", employee_report_table[0].employee_id, employee_report_table[0].attendance_status,employee_report_table[0].Payroll);
            displayPrintf(DISPLAY_ROW_9,  " %d       %s    %d ", employee_report_table[1].employee_id, employee_report_table[1].attendance_status,employee_report_table[1].Payroll);
            displayPrintf(DISPLAY_ROW_10, " %d       %s    %d ", employee_report_table[2].employee_id, employee_report_table[2].attendance_status,employee_report_table[2].Payroll);
  //          displayPrintf(DISPLAY_ROW_ASSIGNMENT, "Server");
            ble_data.headcount=0;
          }
        break;


      case sl_bt_evt_system_external_signal_id:
        event=evt->data.evt_system_external_signal.extsignals;
        if(event==PB0PRESS || event==PB0RELEASE)
          {




        //for bonding
        if(ble_data.isBonded==0)
          {


            if(event==PB0PRESS)
              {
                //displayPrintf(DISPLAY_ROW_9, "Button Pressed",0);//print current state
                sl_bt_sm_passkey_confirm(ble_data.connectionHandle,true);//confirm passkey
              }
            else if(event==PB0RELEASE)
              {
                //displayPrintf(DISPLAY_ROW_9, "Button Released",0);//print current state
              }

          }


        //for sending button state
        else
        {
            gpioLed1SetOff();


            if(event==PB0PRESS)
              {

                //sendPayrollIndication();
                //displayPrintf(DISPLAY_ROW_9, "Button Pressed",0);//print current state
                state[1]=0x01;
                uint8_t temp_btn_state=1;
                sl_status_t sc = sl_bt_gatt_server_write_attribute_value(
                gattdb_button_state, // handle from gatt_db.h
                0,                              // offset
                1, // length
                &temp_btn_state);    // pointer to buffer where data is
                if (sc != SL_STATUS_OK)
                  {
                     LOG_ERROR("sl_bt_gatt_server_write_attribute_value() returned != 0 status=0x%04x", (unsigned int) sc);
                  }

              }
          }
          }
            else if(event==PB0RELEASE)
              {
                //displayPrintf(DISPLAY_ROW_9, "Button Released",0);//print current state
                state[1]=0x00;
                uint8_t temp_btn_state=0;
                sl_status_t sc = sl_bt_gatt_server_write_attribute_value(
               gattdb_button_state, // handle from gatt_db.h
               0,                              // offset
               1, // length
               &temp_btn_state);    // pointer to buffer where data is
               if (sc != SL_STATUS_OK)
                 {
                    LOG_ERROR("sl_bt_gatt_server_write_attribute_value() returned != 0 status=0x%04x", (unsigned int) sc);
                 }
              }


            if(ble_data.connection_open==1 && ble_data.ok_to_send_button_indications==1 &&
               ble_data.isBonded==1 && ble_data.indication_in_flight==0)
               {

                 state[1]=0x01;
                 //sendPayrollIndication();
                 sc = sl_bt_gatt_server_send_indication(ble_data.connectionHandle,
                                                        gattdb_button_state,
                                                        sizeof(state),
                                                        &state[0]);
                 if (sc != SL_STATUS_OK)
                  {
                     LOG_ERROR("sl_bt_advertiser_start release() returned != 0 status=0x%04x", (unsigned int) sc);
                  }

                 else
                   ble_data.indication_in_flight=1;
               }
             else if(ble_data.connection_open==1 && ble_data.ok_to_send_button_indications==1 &&
                     ble_data.isBonded==1 && ble_data.indication_in_flight==1)
               {
                 //LOG_INFO("write queue in button\n\r");
                 //write_queue(gattdb_button_state,sizeof(state),state);
               }






        break;


      case sl_bt_evt_sm_bonding_failed_id:
        ble_data.connectionHandle=evt->data.evt_sm_bonding_failed.connection;
        sl_bt_sm_bonding_confirm(ble_data.connectionHandle,true);
        break;

      case sl_bt_evt_connection_closed_id:
        {
          ble_data.connectionHandle=evt->data.evt_connection_closed.connection;
          // Start general advertising and enable connections.
          sc = sl_bt_advertiser_start(ble_data.advertisingSetHandle, sl_bt_advertiser_general_discoverable,
                                      sl_bt_advertiser_connectable_scannable);

          if (sc != SL_STATUS_OK)
          {
            LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x", (unsigned int) sc);
          }
          ble_data.connection_open=0;
          ble_data.indication_in_flight=0;

          displayPrintf(7, "",0);
          displayPrintf(4, "Advertising",0);//print current state
          sl_bt_sm_delete_bondings();
          displayPrintf(5, " ",0);//print current state
          displayPrintf(6, " ",0);//print current state


          break;

        }// handle close event







      case sl_bt_evt_connection_parameters_id:
        {
          struct sl_bt_evt_connection_parameters_s param_info;
          param_info.connection=(evt->data.evt_connection_parameters.connection);
          param_info.interval=(evt->data.evt_connection_parameters.interval);
          param_info.latency=(evt->data.evt_connection_parameters.latency);
          param_info.timeout=(evt->data.evt_connection_parameters.timeout);
          param_info.security_mode=(evt->data.evt_connection_parameters.security_mode);
  //        LOG_INFO("/rconnection %d interval %d latency %d timeout %d security %d/r/n",param_info.connection,param_info.interval,param_info.latency,param_info.timeout,param_info.security_mode);
          (void) param_info;
        }


          break;


      case sl_bt_evt_gatt_server_characteristic_status_id:

        {
        if(evt->data.evt_gatt_server_characteristic_status.characteristic==gattdb_button_state)
          {
            if(evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config )
            {
              if(evt->data.evt_gatt_server_characteristic_status.client_config_flags==gatt_indication )
               {
                  ble_data.ok_to_send_button_indications=1;
                  gpioLed1SetOn();
               }
              else if(evt->data.evt_gatt_server_characteristic_status.client_config_flags==gatt_disable )
                {
                  ble_data.ok_to_send_button_indications=0;
                  gpioLed1SetOff();
                }
            }
          }

        if(evt->data.evt_gatt_server_characteristic_status.characteristic==gattdb_temperature_measurement)
          {

            if(evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config )
              {
                if(evt->data.evt_gatt_server_characteristic_status.client_config_flags==gatt_indication )
                  {
                    ble_data.ok_to_send_htm_indications=1;
                    gpioLed0SetOn();

                  }
                else if(evt->data.evt_gatt_server_characteristic_status.client_config_flags==gatt_disable )
                  {
                    ble_data.ok_to_send_htm_indications=0;
                    gpioLed0SetOff();
                  }

               }
           }

          if(evt->data.evt_gatt_server_characteristic_status.status_flags==sl_bt_gatt_server_confirmation)
            {
              ble_data.indication_in_flight=0;
            }

          if(ble_data.ok_to_send_htm_indications)
            {
              //displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temp=%d C",actual_temp_local);
            }
          else
            {
              //displayPrintf(DISPLAY_ROW_TEMPVALUE, "",0);
            }

        }

            break;

      case sl_bt_evt_gatt_server_indication_timeout_id:

        ble_data.indication_in_flight=0;
        break;



  ////////////////////common/////////////

      case sl_bt_evt_system_soft_timer_id:
           displayUpdate();
           break;

#else
    case sl_bt_evt_system_boot_id:
      sc = sl_bt_sm_delete_bondings();
       if(sc != SL_STATUS_OK){
           LOG_ERROR("sl_bt_sm_delete_bondings() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
       }
      sc = sl_bt_system_get_identity_address(&ble_data.myAddress, &ble_data.myAddressType);
      if(sc != SL_STATUS_OK){                                                         //Sets Parameters for Connection.
          LOG_ERROR("sl_bt_system_get_identity_address() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
      }
      sc = sl_bt_scanner_set_mode(sl_bt_gap_phy_1m,0);
      if(sc != SL_STATUS_OK){                                                         //Sets Mode for Connection.
          LOG_ERROR("sl_bt_scanner_set_mode() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
      }
      sc = sl_bt_scanner_set_timing(sl_bt_gap_phy_1m, SCAN_INTERVAL, SCAN_WINDOW);   //50 *0.625 = 80 ms, 25 ms Scan window
      if(sc != SL_STATUS_OK){
          LOG_ERROR("sl_bt_scanner_set_timing() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
      }
      sc = sl_bt_scanner_start(sl_bt_gap_phy_1m,sl_bt_scanner_discover_generic);   //Start Scanning
      if(sc != SL_STATUS_OK){
          LOG_ERROR("sl_bt_scanner_start() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
      }
      sc = sl_bt_connection_set_default_parameters(CONNECTION_MIN_INTERVAL, CONNECTION_MAX_INTERVAL, SLAVE_LATENCY, CLIENT_SUPERVISION_TIMEOUT, 0, CLIENT_MAX_CE_LENGTH);    //max_ce_length = 5 * 0.625
      if(sc != SL_STATUS_OK){                                                       //Sets Parameters for Connection.
          LOG_ERROR("sl_bt_connection_set_default_parameters() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
      }
      sc = sl_bt_sm_configure(0x0F, sl_bt_sm_io_capability_displayyesno);
      if(sc != SL_STATUS_OK){
          LOG_ERROR("sl_bt_sm_configure() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
      }
      displayInit();
      displayPrintf(DISPLAY_ROW_NAME, BLE_DEVICE_TYPE_STRING);
      displayPrintf(DISPLAY_ROW_CLIENTADDR, "Discovering");
      displayPrintf(DISPLAY_ROW_BTADDR, "%x:%x:%x:%x:%x:%x", ble_data.myAddress.addr[0], ble_data.myAddress.addr[1], ble_data.myAddress.addr[2], ble_data.myAddress.addr[3], ble_data.myAddress.addr[4], ble_data.myAddress.addr[5]);
      displayPrintf(DISPLAY_ROW_ASSIGNMENT, "Project");
      break;

    case sl_bt_evt_connection_opened_id:
      ble_data.connection_open = true;
      ble_data.Connection_1_Handle  = evt->data.evt_connection_opened.connection;
      displayPrintf(DISPLAY_ROW_CLIENTADDR, "Connected");
      //displayPrintf(DISPLAY_ROW_BTADDR2, "%x:%x:%x:%x:%x:%x", server[0], server[1], server[2], server[3], server[4], server[5]);
      break;

    case sl_bt_evt_connection_closed_id:
      sc = sl_bt_scanner_start(sl_bt_gap_phy_1m , sl_bt_scanner_discover_generic);    //Starts Scanning Again.
      displayPrintf(DISPLAY_ROW_CLIENTADDR, "Discovering");
      if(sc != SL_STATUS_OK){                                                         //Sets Parameters for Connection.
          LOG_ERROR("sl_bt_scanner_start() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
      }
      sc = sl_bt_sm_delete_bondings();
      if(sc != SL_STATUS_OK){
          LOG_ERROR("sl_bt_sm_delete_bondings() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
      }
      ble_data.connection_open = false;
      ble_data.Connection_1_Handle = 0;
      displayPrintf(DISPLAY_ROW_BTADDR2, "");                                         //Clearing all Display prints
      displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
      break;

    case sl_bt_evt_system_soft_timer_id:
      displayUpdate();                                                                //Updating Soft Timer
      if(ble_data.isBonded) {
          ble_data.update_Payroll = true;
          displayPrintf(DISPLAY_ROW_ACTION, "EMPID ATTENDANCE PAY");
          displayPrintf(DISPLAY_ROW_TEMPVALUE, "----- ---------- ---");
         /* displayPrintf(DISPLAY_ROW_8, " %d      %s    $%d", employee_report_table[0].employee_id, employee_report_table[0].attendance_status, employee_report_table[0].Payroll*PER_SEC_FEE);
          displayPrintf(9, " %d      %s    $%d", employee_report_table[1].employee_id, employee_report_table[1].attendance_status, employee_report_table[1].Payroll*PER_SEC_FEE);
          displayPrintf(10, " %d      %s    $%d", employee_report_table[2].employee_id, employee_report_table[2].attendance_status, employee_report_table[2].Payroll*PER_SEC_FEE);*/
      }
      //i2c_sequential_read()
      //if(any of 0,2,4 is 1, then add 1 to index 1,3,5 to their corresponding index)
      //i2c_page_write()
      break;

    case sl_bt_evt_scanner_scan_report_id:                                            //Checking if the Server Address, packet and address_type match.
      if(!memcmp(server,(uint8_t *)&evt->data.evt_scanner_scan_report.address,sizeof(server)) && !evt->data.evt_scanner_scan_report.packet_type && !evt->data.evt_scanner_scan_report.address_type) {
          uint8_t connection_handle =1;
          sl_bt_connection_open(evt->data.evt_scanner_scan_report.address, sl_bt_gap_public_address, sl_bt_gap_phy_1m, &connection_handle);
      }
      break;

    case sl_bt_evt_gatt_procedure_completed_id:
      if(evt->data.evt_gatt_procedure_completed.result == 0x110F){
          sc = sl_bt_sm_increase_security(ble_data.Connection_1_Handle);
          if(sc != SL_STATUS_OK) {
             LOG_ERROR("sl_bt_sm_increase_security() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
          }
      }
      break;

    case sl_bt_evt_system_external_signal_id:
      if(evt -> data.evt_system_external_signal.extsignals == (1 << event_PB1Pressed)) {
          if(ble_data.isBonded) {

          }
          else {
              sc = sl_bt_gatt_read_characteristic_value(ble_data.Connection_1_Handle, ble_data.ButtonCharacteristicHandle);
               if(sc != SL_STATUS_OK) {                                                          //Sets Parameters for Connection.
                  LOG_ERROR("sl_bt_gatt_read_characteristic_value() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
               }
          }
      }
      if(evt -> data.evt_system_external_signal.extsignals == (1 << event_ButtonPressed)) {
        if(ble_data.isBonded == true) {
            uint8_t temp_b[6] = {0,0,0,0,0,0};
            i2c_page_write(2,&temp_b[0],6);
        }
        else {
            sc = sl_bt_sm_passkey_confirm(ble_data.Connection_1_Handle, 1);
            if(sc != SL_STATUS_OK) {                                                         //Sets Parameters for Connection.
               LOG_ERROR("sl_bt_sm_passkey_confirm() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
            }
        }
      }
      break;

    case sl_bt_evt_sm_confirm_passkey_id:
         displayPrintf(DISPLAY_ROW_PASSKEY, "Passkey %d", evt ->data.evt_sm_passkey_display.passkey);
         displayPrintf(DISPLAY_ROW_ACTION, "Confirm with PB0");
     break;

    case sl_bt_evt_gatt_service_id:                                                 //Saving the Service Handle
      if(!memcmp(evt ->data.evt_gatt_service.uuid.data, &PB0_Service_UUID[0], sizeof(PB0_Service_UUID))) {
          ble_data.ButtonServiceHandle = evt -> data.evt_gatt_service.service;
      }
      if(!memcmp(evt ->data.evt_gatt_service.uuid.data, &TEMPERATURE_SERVICE[0], sizeof(TEMPERATURE_SERVICE))) {
          ble_data.EmployeeServiceHandle = evt -> data.evt_gatt_service.service;
      }
      if(!memcmp(evt ->data.evt_gatt_service.uuid.data, &MANAGER_S[0], sizeof(MANAGER_S))) {
          ble_data.ManagerServiceHandle = evt -> data.evt_gatt_service.service;
      }
      break;

    case sl_bt_evt_gatt_characteristic_id:                                          //Saving the Characteristics Handle
      if(!memcmp(evt ->data.evt_gatt_characteristic.uuid.data, &TEMPERATURE_CHAR[0], sizeof(TEMPERATURE_CHAR))){
          ble_data.EmployeeCharacteristicHandle = evt -> data.evt_gatt_characteristic.characteristic;
      }
      if(!memcmp(evt ->data.evt_gatt_characteristic.uuid.data, &ATTENDANCE_DATA_C[0], sizeof(ATTENDANCE_DATA_C))) {
          ble_data.Manager_ATT_CharacteristicHandle = evt -> data.evt_gatt_characteristic.characteristic;
      }
      if(!memcmp(evt ->data.evt_gatt_characteristic.uuid.data, &WAGES_CHARACTERISTIC[0], sizeof(WAGES_CHARACTERISTIC))) {
          ble_data.Manager_Wages_CharacteristicHandle = evt -> data.evt_gatt_characteristic.characteristic;
      }
      if(!memcmp(evt ->data.evt_gatt_characteristic.uuid.data, PB0_Char_UUID, sizeof(PB0_Char_UUID))) {
          ble_data.ButtonCharacteristicHandle = evt -> data.evt_gatt_characteristic.characteristic;
      }
      break;

    case sl_bt_evt_sm_bonded_id:
       LOG_INFO("Bonded\n\r");
       ble_data.isBonded = true;
       displayPrintf(DISPLAY_ROW_PASSKEY, " ");
       displayPrintf(DISPLAY_ROW_ACTION, " ");
       displayPrintf(DISPLAY_ROW_CLIENTADDR, "Bonded");
       break;

     case sl_bt_evt_sm_bonding_failed_id:
       ble_data.isBonded = false;
       LOG_INFO("BONDING FAILED = %x\n\r", evt->data.evt_sm_bonding_failed.reason);
       displayPrintf(DISPLAY_ROW_PASSKEY, " ");
       displayPrintf(DISPLAY_ROW_ACTION, " ");
       break;

    case sl_bt_evt_gatt_characteristic_value_id:
      if(evt->data.evt_gatt_characteristic_value.characteristic == ble_data.ButtonCharacteristicHandle) {
          LOG_INFO("Button Indication received %d\n\r", ble_data.ButtonCharacteristicHandle);
          sc = sl_bt_gatt_send_characteristic_confirmation(ble_data.Connection_1_Handle);
          if(sc != SL_STATUS_OK){                                                         //Sets Parameters for Connection.
              LOG_ERROR("sl_bt_gatt_send_characteristic_confirmation() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
          }
          if(evt-> data.evt_gatt_characteristic_value.value.data[0] && ble_data.managerLogin) {
               uint8_t send_payroll[3];
               uint16_t send_len;
               for(int i =0; i < 3; i++) {
                    send_payroll[i] = employee_report_table[i].Payroll;
               }
               if(!ble_data.indication_in_flight) {
                   sl_bt_gatt_write_characteristic_value_without_response(ble_data.Connection_1_Handle, ble_data.Manager_Wages_CharacteristicHandle, 3, &send_payroll[0], &send_len);
               }
           }
      }
      if(evt->data.evt_gatt_characteristic_value.characteristic == ble_data.EmployeeCharacteristicHandle) {
          sc = sl_bt_gatt_send_characteristic_confirmation(ble_data.Connection_1_Handle);
          if(sc != SL_STATUS_OK){                                                         //Sets Parameters for Connection.
              LOG_ERROR("sl_bt_gatt_send_characteristic_confirmation() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
          }
          tmp_eid = (evt->data.evt_gatt_characteristic_value.value.data[0]);         //Calculating Employee information
          if(tmp_eid == 4) {
              if(!ble_data.managerLogin){
                  ble_data.managerLogin = true;
                  ble_data.sent_once = true;
                  displayPrintf(DISPLAY_ROW_CONNECTION, "Manager Access");
              }
              else {
                  ble_data.managerLogin = false;
                  displayPrintf(DISPLAY_ROW_CONNECTION, "");
              }
          }
          else if(tmp_eid < 4) {
              ble_data.store_attendance = true;
          }
      }
      break;
#endif
  } // end - switch
} // handle_ble_event()


uint8_t get_eid() {
   return tmp_eid;
}

void ble_send_indication(uint16_t temperature)
{
  uint8_t        htm_temperature_buffer[5];                                   //5 bytes, 1st byte is metadata and the last 4 bytes stores the value
  uint8_t        *p = htm_temperature_buffer;
  uint32_t       htm_temperature_flt;
  UINT8_TO_BITSTREAM(p,0);
  htm_temperature_flt = UINT32_TO_FLOAT(temperature*1000, -3);                //converts the temperature to 4 bytes data.
  UINT32_TO_BITSTREAM(p, htm_temperature_flt);
  sc = sl_bt_gatt_server_write_attribute_value(gattdb_temperature_measurement,0,5,&htm_temperature_buffer[0]);  //Writes Attribute
  if(sc != SL_STATUS_OK){
      LOG_ERROR("sl_bt_gatt_server_write_attribute_value() returned non-zero status=0x%04x\n\r", (unsigned int) sc);
  }
  if((ble_data.connection_open==true) && (ble_data.ok_to_send_htm_indications=true))
  {
    if(ble_data.indication_in_flight==false) {                              //If the radio is not busy Starts sending indication
        sc = sl_bt_gatt_server_send_indication(ble_data.Connection_1_Handle,gattdb_temperature_measurement,5,&htm_temperature_buffer[0]);
        if (sc != SL_STATUS_OK) {
            LOG_ERROR("sl_bt_gatt_server_send_indication 1() returned non-zero status=0x%04x\n\r", (unsigned int) sc);
        }
        else {
            ble_data.indication_in_flight = true;
            LOG_INFO("Temperature=%d C\n\r", temperature);
        }
    }
    else {
        write_queue(gattdb_temperature_measurement, 5, &htm_temperature_buffer[0]);
        ble_data.queued_indications++;
    }
  }
}

static uint32_t nextPtr(uint32_t ptr) {
  ptr = (ptr +1)&(QUEUE_DEPTH-1);         //Advance Pointer and Rollover
  return ptr;
} // nextPtr()

bool read_queue (uint16_t *charHandle, size_t *bufferLength, uint8_t *buffer) {
  if(!isempty){
    *charHandle = my_queue[rptr].charHandle;             //Dequeuing Element out
    *bufferLength = my_queue[rptr].bufferLength;
    memcpy(buffer,my_queue[rptr].buffer , my_queue[rptr].bufferLength);
    if(isfull){                        //If full release pointer and clear flag
      isfull = false;
      wptr = nextPtr(wptr);
    }
    rptr = nextPtr(rptr);       //Advance Read Pointer after Dequeuing
    queue_length --;
    if(wptr == rptr) {          //Checking for Empty condition
      isempty =true;
    }
    return false;
  }
  else {
    return true;
  }
} // read_queue()


bool write_queue (uint16_t charHandle, size_t bufferLength, uint8_t *buffer) {
  if(!isfull){
    my_queue[wptr].charHandle = charHandle;     //Inserting the element in the queue
    my_queue[wptr].bufferLength = bufferLength;
    memcpy(my_queue[wptr].buffer, buffer, bufferLength);
    isempty = false;        //After write, clearing the empty flag
    queue_length ++;
    if(nextPtr(wptr) != rptr) {     //If increment wptr is not equal to rptr
      wptr = nextPtr(wptr);   // we advance the pointer
    }
    else{                           //Otherwise we set the Full flag
      isfull =true;
    }
    return false;
  }
  else {
    return true;
  }
} // write_queue()



