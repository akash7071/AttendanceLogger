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

#define SI7021_DEVICE_ADDR  0x40
#define I2C0_SCL_PIN        10
#define I2C0_SDA_PIN        11
#define POWER_ON_SEQ_US     80000
#define I2C_TRANSFER_WAIT   10800
#define INCLUDE_LOG_DEBUG   1
#include "log.h"

I2C_TransferReturn_TypeDef transferStatus;
uint8_t cmd_data;
uint16_t read_data;
uint16_t curr_temp =0;
void I2CSPM_Sensor_Init() {
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

void I2CSPM_Get_Temperature() {
  I2C_TransferSeq_TypeDef transferSequence;        // this one can be local
  cmd_data = 0xF3;
  transferSequence.addr = SI7021_DEVICE_ADDR << 1; // shift device address left
  transferSequence.flags = I2C_FLAG_WRITE;
  transferSequence.buf[0].data = &cmd_data;       // pointer to data to write
  transferSequence.buf[0].len = sizeof(cmd_data);
  transferStatus = I2CSPM_Transfer (I2C0, &transferSequence);
  if (transferStatus != i2cTransferDone) {
      LOG_ERROR("I2CSPM_Transfer: I2C bus write failed\n\r");
  }
}

uint16_t I2CSPM_read_temperature() {
  uint16_t temperature;
  I2C_TransferSeq_TypeDef transferSequence;
  transferSequence.addr = SI7021_DEVICE_ADDR << 1;      // shift device address left
  transferSequence.flags = I2C_FLAG_READ;
  transferSequence.buf[0].data = (uint8_t*)&read_data;  // pointer to data to write
  transferSequence.buf[0].len = sizeof(read_data);
  transferStatus = I2CSPM_Transfer(I2C0, &transferSequence);
  if (transferStatus != i2cTransferDone) {
      LOG_ERROR("I2CSPM_Transfer: I2C bus Read failed\n\r");
  }
  else{
    temperature = ((read_data<<8) | ((read_data >>8) &0xFC));          //Swapping the MS nibble with LS Nibble for 0xFFFC;
    temperature = ( ( 175.72 * ( temperature ) ) / 65536 ) - 46.85;    //Formula as given on Page 22 of Si7021 - A20 Datasheet
  }
  return temperature;
}

void read_temp_from_si7021() {
  gpioSi7021Enable();                                   //Enable the SENSOR_ENABLE
  timerWaitUs(POWER_ON_SEQ_US);                         //Wait for the Sensor to power up
  I2CSPM_Sensor_Init();                                 //ENable the i2c bus
  I2CSPM_Get_Temperature();                             //Sends a write command for temperature
  timerWaitUs(I2C_TRANSFER_WAIT);                       //Wait for the i2c transfer to complete
  curr_temp = I2CSPM_read_temperature();                //Reads temperature and converts it into degree celsius
  gpioSi7021Disable();                                  //Disable the SENSOR_ENABLE
  LOG_INFO("Temperature is %d degree C\n\r", curr_temp);
}

