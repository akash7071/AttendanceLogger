 /******************************************************************************
 * Date:        02-25-2022
 * Author:      Harsh Beriwal (harsh.beriwal@colorado.edu)
 * Description: This code was created for providing visibility for configuring
 *              the I2C bus for communicating with the temperature sensor.
 *              A functions gets the i2c reading and converts it into a
 *              temperature in C.
 *              It is to be used only for ECEN 5823 "IoT Embedded Firmware".
 *
 ******************************************************************************/

#ifndef SRC_I2C_H_
#define SRC_I2C_H_

void I2CSPM_Sensor_Init();
void I2CSPM_Get_Temperature();
uint16_t I2CSPM_read_temperature();
void read_temp_from_si7021();
#endif /* SRC_I2C_H_ */
