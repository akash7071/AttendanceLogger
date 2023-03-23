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
#include <math.h>

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
  bool          isBonded;
  bool          ok_to_send_button_indications;
  uint8_t       buttonStatus;
  uint8_t       queued_indications;
  uint32_t      serviceHandle;
  uint32_t      ButtonServiceHandle;
  uint16_t      characteristicHandle;
  uint16_t      ButtonCharacteristicHandle;
} ble_data_struct_t;

void handle_ble_event(sl_bt_msg_t *evt);
ble_data_struct_t* ble_get_data_struct();
void ble_send_indication(uint16_t temperature);

// button state service UUID 00000001-38c8-433e-87ec-652a2d136289
static const uint8_t PB0_Service_UUID[16] = {0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e, 0x43, 0xc8, 0x38, 0x01, 0x00, 0x00, 0x00};

// button STate characteristic UUID 00000002-38c8-433e-87ec-652a2d136289
static const uint8_t PB0_Char_UUID[16] = {0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e, 0x43, 0xc8, 0x38, 0x02, 0x00, 0x00, 0x00};

typedef struct {
  uint16_t      charHandle;
  size_t        bufferLength;
  uint8_t       buffer[5];
} queue_struct_t;

bool write_queue (uint16_t charHandle, size_t bufferLength, uint8_t *buffer);
bool read_queue (uint16_t *charHandle, size_t *bufferLength, uint8_t *buffer);

#endif /* SRC_BLE_H_ */
