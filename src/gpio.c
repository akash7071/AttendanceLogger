/*
  gpio.c
 
   Created on: Dec 12, 2018
       Author: Dan Walkes
   Updated by Dave Sluiter Dec 31, 2020. Minor edits with #defines.

   March 17
   Dave Sluiter: Use this file to define functions that set up or control GPIOs.

 *
 * Student: Harsh Beriwal (harsh.beriwal@colorado.edu)
 *
 *
 */


// *****************************************************************************
// Students:
// We will be creating additional functions that configure and manipulate GPIOs.
// For any new GPIO function you create, place that function in this file.
// *****************************************************************************

#include <stdbool.h>
#include "em_gpio.h"
#include <string.h>

#include "gpio.h"


// Student Edit: Define these, 0's are placeholder values.
// See the radio board user guide at https://www.silabs.com/documents/login/user-guides/ug279-brd4104a-user-guide.pdf
// and GPIO documentation at https://siliconlabs.github.io/Gecko_SDK_Doc/efm32g/html/group__GPIO.html
// to determine the correct values for these.

#define LED0_port  5
#define LED0_pin   4
#define LED1_port  5
#define LED1_pin   5


// Set GPIO drive strengths and modes of operation
void gpioInit()
{
  // Student Edit:
	GPIO_DriveStrengthSet(LED0_port, gpioDriveStrengthWeakAlternateWeak);
	GPIO_PinModeSet(LED0_port, LED0_pin, gpioModePushPull, false);

	/**********************LED SETTING FOR Q2**************************************/
	/*GPIO_DriveStrengthSet(LED0_port, gpioDriveStrengthStrongAlternateStrong);
	 GPIO_PinModeSet(LED0_port, LED0_pin, gpioModePushPull, false);*/

	GPIO_DriveStrengthSet(LED1_port, gpioDriveStrengthWeakAlternateWeak);
	GPIO_PinModeSet(LED1_port, LED1_pin, gpioModePushPull, false);

  GPIO_PinModeSet(3, 15, gpioModePushPull, true);
}


void gpioLed0SetOn()
{
	GPIO_PinOutSet(LED0_port,LED0_pin);
}


void gpioLed0SetOff()
{
	GPIO_PinOutClear(LED0_port,LED0_pin);
}


void gpioLed1SetOn()
{
	GPIO_PinOutSet(LED1_port,LED1_pin);
}


void gpioLed1SetOff()
{
	GPIO_PinOutClear(LED1_port,LED1_pin);
}

void gpioSi7021Enable()
{
    GPIO_PinOutSet(3, 15);
}

void gpioSi7021Disable()
{
    GPIO_PinOutClear(3, 15);
}




