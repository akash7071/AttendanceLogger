 /******************************************************************************
 * Date:        02-25-2022
 * Author:      Harsh Beriwal (harsh.beriwal@colorado.edu)
 * Description: This code was created for having ble event responder and send
 *              indication function. This code communicate with the Bluetooth Stack.
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

// BLE private data
ble_data_struct_t ble_data;
sl_status_t sc=0; // status code

ble_data_struct_t* ble_get_data_struct()             //Function to get a pointer to the Data Structure
{
  return (&ble_data);
}

void handle_ble_event(sl_bt_msg_t *evt) {           //Ble Event Responder
  switch (SL_BT_MSG_ID(evt->header)) {
    case sl_bt_evt_system_boot_id:                  //Boot Event
      sc = sl_bt_system_get_identity_address(&ble_data.myAddress, &ble_data.myAddressType);  //Returns the unique BT device Address
      displayInit();
      displayPrintf(DISPLAY_ROW_NAME, BLE_DEVICE_TYPE_STRING);
      displayPrintf(DISPLAY_ROW_BTADDR, "%x:%x:%x:%x:%x", ble_data.myAddress.addr[0], ble_data.myAddress.addr[1], ble_data.myAddress.addr[2], ble_data.myAddress.addr[3], ble_data.myAddress.addr[4], ble_data.myAddress.addr[5]);
      if (sc != SL_STATUS_OK) {
          LOG_ERROR("sl_bt_system_get_identity_address() returned non-zero status=0x%04x\n\r", (unsigned int) sc);
      }
      sc = sl_bt_advertiser_create_set(&ble_data.advertisingSetHandle);                     //Create an Advertising set
      if(sc != SL_STATUS_OK){
          LOG_ERROR("sl_bt_advertiser_create_set() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
      }
      sc = sl_bt_advertiser_set_timing(ble_data.advertisingSetHandle, ADVERTISER_INTERVAL_MIN,  //Sets the Timing for Advertising Packets
                                       ADVERTISER_INTERVAL_MAX, DURATION, MAX_EVENTS);
      if(sc != SL_STATUS_OK){
          LOG_ERROR("sl_bt_advertiser_set_timing() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
      }
      sc = sl_bt_advertiser_start(ble_data.advertisingSetHandle, sl_bt_advertiser_general_discoverable,sl_bt_advertiser_connectable_scannable);
      if(sc != SL_STATUS_OK){                                                                  //Starts Sending Advertising Packets.
          LOG_ERROR("sl_bt_advertiser_start() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
      }
      break;

    case sl_bt_evt_connection_opened_id:                    //Connection Open Event
      ble_data.temperatureSetHandle = evt->data.evt_connection_opened.connection;     //Getting the connection Handle
      sc = sl_bt_advertiser_stop(ble_data.advertisingSetHandle);                      //Stops Advertising
      if(sc != SL_STATUS_OK){
          LOG_ERROR("sl_bt_advertiser_stop() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
      }
      sc = sl_bt_connection_set_parameters(ble_data.temperatureSetHandle, CONNECTION_MIN_INTERVAL, CONNECTION_MAX_INTERVAL, SLAVE_LATENCY, SUPERVISION_TIMEOUT, CON_MIN_CE_LENGTH, CON_MAX_CE_LENGTH);
      if(sc != SL_STATUS_OK){                                                         //Sets Parameters for Connection.
          LOG_ERROR("sl_bt_connection_set_parameters() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
      }
      ble_data.connection_open = true;
      displayPrintf(DISPLAY_ROW_CONNECTION, "Connected");
      // handle open event
      break;
    case sl_bt_evt_connection_closed_id:                                          // handle close event
      sc = sl_bt_advertiser_start(ble_data.advertisingSetHandle, sl_bt_advertiser_general_discoverable,sl_bt_advertiser_connectable_scannable);
      if(sc != SL_STATUS_OK){                                                       //Starts Advertising Back again
          LOG_ERROR("sl_bt_advertiser_start() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
      }
      ble_data.connection_open = false;
      ble_data.indication_in_flight = false;
      displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");
      break;
    case sl_bt_evt_connection_parameters_id:                            //Displays Connection Parameters
      LOG_INFO("Master Connection Parameters - INTERVAL = %dms, SLAVE LATENCY = %d, TIMEOUT = %dms\n\r",
               ((int)evt->data.evt_connection_parameters.interval),
               ((int)evt->data.evt_connection_parameters.latency),
               ((int)evt->data.evt_connection_parameters.timeout));
      break;
    case sl_bt_evt_gatt_server_characteristic_status_id:                //Checks if indication is pressed on temperature measurement
      if  (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_temperature_measurement)
      {
        if(evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config){
            if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_indication){
                ble_data.ok_to_send_htm_indications = true;
            }
            if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_disable){
                ble_data.ok_to_send_htm_indications = false;
            }
        }
        if(evt->data.evt_gatt_server_characteristic_status.status_flags  == sl_bt_gatt_server_confirmation){
            ble_data.indication_in_flight = false;
        }
      }
      break;
    case sl_bt_evt_gatt_server_indication_timeout_id:
      LOG_ERROR("Indication Error\n\r");
      ble_data.indication_in_flight=false;
      break;
    case sl_bt_evt_system_soft_timer_id:
      displayUpdate();
  } // end - switch
} // handle_ble_event()


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
        sc = sl_bt_gatt_server_send_indication(ble_data.temperatureSetHandle,gattdb_temperature_measurement,5,&htm_temperature_buffer[0]);
        if (sc != SL_STATUS_OK) {
            LOG_ERROR("sl_bt_gatt_server_send_indication() returned non-zero status=0x%04x\n\r", (unsigned int) sc);
        }
        else {
            ble_data.indication_in_flight = true;
            LOG_INFO("Temperature=%d C\n\r", temperature);
        }
    }
  }
}


