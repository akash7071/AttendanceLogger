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
  uint8_t       Connection_1_Handle;
 // uint8_t       Connection_2_Handle;
  bool          connection_open;            // true when in an open connection
  bool          ok_to_send_htm_indications; // true when client enabled indications
  bool          indication_in_flight;
  bool          isBonded;
  bool          ok_to_send_button_indications;
  uint8_t       buttonStatus;
  uint8_t       queued_indications;
  uint32_t      EmployeeServiceHandle;
  uint32_t      ButtonServiceHandle;
  uint32_t      ManagerServiceHandle;
  uint16_t      EmployeeCharacteristicHandle;
  uint16_t      ButtonCharacteristicHandle;
  uint16_t      Manager_ATT_CharacteristicHandle;
  uint16_t      Manager_Wages_CharacteristicHandle;
  bool          store_attendance;
  bool          managerLogin;
  bool          update_Payroll;
  bool          send_payroll;
} ble_data_struct_t;

void handle_ble_event(sl_bt_msg_t *evt);
ble_data_struct_t* ble_get_data_struct();
void ble_send_indication(uint16_t temperature);

// button state service UUID 00000001-38c8-433e-87ec-652a2d136289
static const uint8_t PB0_Service_UUID[16] = {0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e, 0x43, 0xc8, 0x38, 0x01, 0x00, 0x00, 0x00};

// button STate characteristic UUID 00000002-38c8-433e-87ec-652a2d136289
static const uint8_t PB0_Char_UUID[16] = {0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e, 0x43, 0xc8, 0x38, 0x02, 0x00, 0x00, 0x00};
/*employee:
attendance logger : 43a8a838-4f55-4e10-8afa-59c0f8d4442b*/
static const uint8_t ATTENDANCE_LOGGER_C[16] = {0x2B, 0x44, 0xD4, 0xF8, 0xC0, 0x59, 0xFA, 0x8A, 0x10, 0x4E, 0x55, 0x4F, 0x38, 0xA8, 0xA8, 0x43};

//Attendance Data: 587ddc07-4953-4b87-8978-584702ca95eb
static const uint8_t ATTENDANCE_DATA_C[16] = {0xEB, 0X95, 0xCA, 0x02, 0x47, 0x58, 0x78, 0x89, 0x87, 0x4B, 0x53, 0x49, 0x07, 0xDC, 0x7D, 0x58};
static const uint8_t WAGES_CHARACTERISTIC[16] = {0X0E, 0xAE, 0x08, 0xB1, 0xE0, 0xE0, 0x95, 0x95, 0x57, 0x4D, 0x13, 0xA9, 0xEC, 0xAD, 0x13, 0xCD};
//23f4f2b2-49b6-4d49-950f-c257e7b8e123
static const uint8_t EMPLOYEE_S[16] = {0X23, 0xE1, 0xB8, 0xE7, 0x57, 0xC2, 0x0F, 0x95, 0x49, 0x4D, 0xB6, 0x49, 0xB2, 0xF2, 0xF4, 0x23};
//3b14e668-eb36-470e-8abe-f8c630eefd28
static const uint8_t MANAGER_S[16] = {0X28, 0xFD, 0xEE, 0x30, 0xC6, 0xF8, 0xBE, 0x8A, 0x0E, 0x47, 0x36, 0xEB, 0x68, 0xE6, 0x14, 0x3B};

typedef struct {
  uint16_t      charHandle;
  size_t        bufferLength;
  uint8_t       buffer[5];
} queue_struct_t;


typedef enum {
  Absent,
  Clocked_in,
  Clocked_out,
}attendance_status_t ;

typedef struct {
  uint8_t employee_id;
  uint8_t status;
  char  attendance_status[11];
  uint16_t Payroll;
}employee_report_t;

static employee_report_t employee_report_table[] = {
    {1, Absent, "Absent", 0},
    {2, Absent, "Absent", 0},
    {3, Absent, "Absent", 0},
};

bool write_queue (uint16_t charHandle, size_t bufferLength, uint8_t *buffer);
bool read_queue (uint16_t *charHandle, size_t *bufferLength, uint8_t *buffer);
void get_eid(uint8_t *eid);

#endif /* SRC_BLE_H_ */
