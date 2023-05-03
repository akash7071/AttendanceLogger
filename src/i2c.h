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

void I2C_Sensor_Init();
void I2C_write_Temperature();
void I2C_read_temperature();
uint16_t calculate_temperature();
void sensor_enable();
void sensor_write_temperature();
void i2c_write(uint8_t w_address, uint8_t w_data);
void i2c_read(uint8_t word_address_r);
uint8_t get_EEPROM_data();
void i2c_sequential_read(uint8_t start_address, uint8_t len);
uint8_t* get_all_data();
void i2c_page_write(uint8_t start_address, uint8_t *data, uint8_t len);

#endif /* SRC_I2C_H_ */
