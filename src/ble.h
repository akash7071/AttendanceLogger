 /******************************************************************************
 * Date:        02-25-2022
 * Author:      Harsh Beriwal (harsh.beriwal@colorado.edu)
 * Description: This code was created for visibility of ble event responder and
 *              ble function to send indication.
 *              It is to be used only for ECEN 5823 "IoT Embedded Firmware".
 *
 ******************************************************************************/
#ifndef SRC_BLE_H_
#define SRC_BLE_H_

#include <stdio.h>
#include <stdbool.h>
#include "sl_bt_api.h"

#define UINT8_TO_BITSTREAM(p, n) { *(p)++ = (uint8_t)(n); }
#define UINT32_TO_BITSTREAM(p, n) { *(p)++ = (uint8_t)(n); *(p)++ = (uint8_t)((n) >> 8); \
    *(p)++ = (uint8_t)((n) >> 16); *(p)++ = (uint8_t)((n) >> 24); }
#define UINT32_TO_FLOAT(m, e) (((uint32_t)(m) & 0x00FFFFFFU) | (uint32_t)((int32_t)(e) << 24))
// BLE Data Structure, save all of our private BT data in here.
// Modern C (circa 2021 does it this way)
// typedef ble_data_struct_t is referred to as an anonymous struct definition
typedef struct {
  // values that are common to servers and clients
  bd_addr       myAddress;
  uint8_t       myAddressType;
  // The advertising set handle allocated from Bluetooth stack.
  uint8_t       advertisingSetHandle;
  uint8_t       temperatureSetHandle;
  bool          connection_open;            // true when in an open connection
  bool          ok_to_send_htm_indications; // true when client enabled indications
  bool          indication_in_flight;
} ble_data_struct_t;

void handle_ble_event(sl_bt_msg_t *evt);
ble_data_struct_t* ble_get_data_struct();
void ble_send_indication(uint16_t temperature);
#endif /* SRC_BLE_H_ */
