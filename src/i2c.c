 /******************************************************************************
 * Date:        02-25-2022
 * Author:      Harsh Beriwal (harsh.beriwal@colorado.edu)
 * Description: This code was created for configuring the I2C bus for communicating
 *              with the temperature sensor. A functions gets the i2c reading and
 *              converts it into a temperature in C.
 *              It is to be used only for ECEN 5823 "IoT Embedded Firmware".
 *
 ******************************************************************************/

#include "sl_i2cspm.h"
#include "timers.h"
#include "gpio.h"
#include "core_cm4.h"
#include "i2c.h"

#define SI7021_DEVICE_ADDR  0x40
#define I2C0_SCL_PIN        10
#define I2C0_SDA_PIN        11
#define POWER_ON_SEQ_US     80000
#define INCLUDE_LOG_DEBUG   1
#include "log.h"

I2C_TransferReturn_TypeDef transferStatus;
I2C_TransferSeq_TypeDef transferSequence;
uint8_t cmd_data = 0;
uint16_t read_data =0;
uint16_t curr_temp =0;

void I2C_Sensor_Init() {
  I2CSPM_Init_TypeDef I2C_Config = {
  .port = I2C0,
  .sclPort = gpioPortC,
  .sclPin = I2C0_SCL_PIN,
  .sdaPort = gpioPortC,
  .sdaPin = I2C0_SDA_PIN,
  .portLocationScl = 14,
  .portLocationSda = 16,
  .i2cRefFreq = 0,
  .i2cMaxFreq = I2C_FREQ_STANDARD_MAX,
  .i2cClhr = i2cClockHLRStandard
  };
  I2CSPM_Init(&I2C_Config);
}

void I2C_write_Temperature() {
  cmd_data = 0xF3;
  transferSequence.addr = SI7021_DEVICE_ADDR << 1; // shift device address left
  transferSequence.flags = I2C_FLAG_WRITE;
  transferSequence.buf[0].data = &cmd_data;       // pointer to data to write
  transferSequence.buf[0].len = sizeof(cmd_data);
  NVIC_EnableIRQ(I2C0_IRQn);
  transferStatus = I2C_TransferInit(I2C0, &transferSequence);
  if (transferStatus < 0) {
      LOG_ERROR("I2CSPM_Transfer: I2C bus write failed\n\r");
  }
  I2C_Transfer(I2C0);
}

void I2C_read_temperature(){
  transferSequence.addr = SI7021_DEVICE_ADDR << 1;      // shift device address left
  transferSequence.flags = I2C_FLAG_READ;
  transferSequence.buf[0].data = (uint8_t*)&read_data;  // pointer to data to write
  transferSequence.buf[0].len = sizeof(read_data);
  NVIC_EnableIRQ(I2C0_IRQn);
  transferStatus = I2C_TransferInit(I2C0, &transferSequence);
  if (transferStatus < 0) {
      LOG_ERROR("I2CSPM_Transfer: I2C bus Read failed\n\r");
  }
  I2C_Transfer(I2C0);
}

uint16_t calculate_temperature() {
  uint16_t temperature =0;
  temperature = ((read_data<<8) | ((read_data >>8) &0xFC));          //Swapping the MS nibble with LS Nibble for 0xFFFC;
  temperature = ( ( 175.72 * ( temperature ) ) / 65536 ) - 46.85;    //Formula as given on Page 22 of Si7021 - A20 Datasheet
  return temperature;
}

void sensor_enable(){
  gpioSi7021Enable();                                       //Enable the SENSOR_ENABLE
  timerWaitUs_irq(POWER_ON_SEQ_US);                         //Wait for the Sensor to power up
}

void sensor_write_temperature(){
  I2C_Sensor_Init();                                        //Initialize the I2C connection
  I2C_write_Temperature();                                  //Sends a Temperature Write Command.
}


