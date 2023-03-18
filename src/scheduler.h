 /******************************************************************************
 * Date:        02-25-2022
 * Author:      Harsh Beriwal (harsh.beriwal@colorado.edu)
 * Description: This code was created for visibility of scheduler functions for
 *              handling multiple events of different types.
 *              It is to be used only for ECEN 5823 "IoT Embedded Firmware".
 *
 ******************************************************************************/
#ifndef SRC_SCHEDULER_H_
#define SRC_SCHEDULER_H_

#include <stdint.h>
#include "sl_bt_api.h"

typedef enum {            //Event Declared as 0 has the highest priority and
    event_Timer_UF,       //the priority decreases as the count increases
    event_Timer_COMP1,
    event_I2C_transferDone,
    event_ButtonPressed,
    event_ButtonReleased,
} some_type_t;

void schedulerSetReadTemperature();
void schedulerSetI2Ctransfer() ;
void schedulerSetWaitDone();
void schedulerSetButtonPressed();
void schedulerSetButtonReleased();
void Temperature_state_machine(sl_bt_msg_t *event);
void discovery_state_machine(sl_bt_msg_t *event);
uint32_t getNextEvent();

#endif /* SRC_SCHEDULER_H_ */
