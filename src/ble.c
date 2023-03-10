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

// BLE private data
ble_data_struct_t ble_data;
sl_status_t sc=0; // status code
int temperature_in_c;
uint8_t server[6] = SERVER_BT_ADDRESS;
uint8_t *temperature;
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
    case sl_bt_evt_system_boot_id:                  //Boot Event
      sc = sl_bt_system_get_identity_address(&ble_data.myAddress, &ble_data.myAddressType);  //Returns the unique BT device Address
      displayInit();
      displayPrintf(DISPLAY_ROW_NAME, BLE_DEVICE_TYPE_STRING);
      displayPrintf(DISPLAY_ROW_BTADDR, "%x:%x:%x:%x:%x:%x", ble_data.myAddress.addr[0], ble_data.myAddress.addr[1], ble_data.myAddress.addr[2], ble_data.myAddress.addr[3], ble_data.myAddress.addr[4], ble_data.myAddress.addr[5]);
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
      displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");
      displayPrintf(DISPLAY_ROW_ASSIGNMENT, "A7");
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
      displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
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
      break;

#else
    case sl_bt_evt_system_boot_id:
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
      displayInit();
      displayPrintf(DISPLAY_ROW_NAME, BLE_DEVICE_TYPE_STRING);
      displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");
      displayPrintf(DISPLAY_ROW_BTADDR, "%x:%x:%x:%x:%x:%x", ble_data.myAddress.addr[0], ble_data.myAddress.addr[1], ble_data.myAddress.addr[2], ble_data.myAddress.addr[3], ble_data.myAddress.addr[4], ble_data.myAddress.addr[5]);
      displayPrintf(DISPLAY_ROW_ASSIGNMENT, "A7");
      break;

    case sl_bt_evt_connection_opened_id:
      ble_data.connection_open = true;
      ble_data.temperatureSetHandle  = evt->data.evt_connection_opened.connection;
      displayPrintf(DISPLAY_ROW_CONNECTION, "Connected");
      displayPrintf(DISPLAY_ROW_BTADDR2, "%x:%x:%x:%x:%x:%x", server[0], server[1], server[2], server[3], server[4], server[5]);
      break;

    case sl_bt_evt_connection_closed_id:
      sc = sl_bt_scanner_start(sl_bt_gap_phy_1m , sl_bt_scanner_discover_generic);    //Starts Scanning Again.
      displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");
      if(sc != SL_STATUS_OK){                                                         //Sets Parameters for Connection.
          LOG_ERROR("sl_bt_scanner_start() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
      }
      ble_data.connection_open = false;
      ble_data.temperatureSetHandle = 0;
      displayPrintf(DISPLAY_ROW_BTADDR2, "");                                         //Clearing all Display prints
      displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
      break;

    case sl_bt_evt_system_soft_timer_id:
      displayUpdate();                                                                //Updating Soft Timer
      break;

    case sl_bt_evt_scanner_scan_report_id:                                            //Checking if the Server Address, packet and address_type match.
      if(!memcmp(server,(uint8_t *)&evt->data.evt_scanner_scan_report.address,sizeof(server)) && !evt->data.evt_scanner_scan_report.packet_type && !evt->data.evt_scanner_scan_report.address_type) {
          uint8_t connection_handle =1;
          sl_bt_connection_open(evt->data.evt_scanner_scan_report.address, sl_bt_gap_public_address, sl_bt_gap_phy_1m, &connection_handle);
      }
      break;

    case sl_bt_evt_gatt_procedure_completed_id:
      break;

    case sl_bt_evt_gatt_service_id:                                                 //Saving the Service Handle
      ble_data.serviceHandle = evt -> data.evt_gatt_service.service;
      break;

    case sl_bt_evt_gatt_characteristic_id:                                          //Saving the Characteristics Handle
      ble_data.characteristicHandle = evt -> data.evt_gatt_characteristic.characteristic;
      break;

    case sl_bt_evt_gatt_characteristic_value_id:
      sc = sl_bt_gatt_send_characteristic_confirmation(ble_data.temperatureSetHandle);
      if(sc != SL_STATUS_OK){                                                         //Sets Parameters for Connection.
          LOG_ERROR("sl_bt_gatt_send_characteristic_confirmation() returned non-zero status=0x%04x\n\r", (unsigned int)sc);
      }
      temperature = &(evt->data.evt_gatt_characteristic_value.value.data[0]);         //Calculating Temperature
      temperature_in_c = FLOAT_TO_INT32(temperature);                                 //Converting 4 bytes of FLoat to integer
      displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temp = %d C", temperature_in_c);
      break;
#endif
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




