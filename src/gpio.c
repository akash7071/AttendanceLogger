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
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

// Student Edit: Define these, 0's are placeholder values.
// See the radio board user guide at https://www.silabs.com/documents/login/user-guides/ug279-brd4104a-user-guide.pdf
// and GPIO documentation at https://siliconlabs.github.io/Gecko_SDK_Doc/efm32g/html/group__GPIO.html
// to determine the correct values for these.

#define LED0_port       5
#define LED0_pin        4
#define LED1_port       5
#define LED1_pin        5
#define EXTCOMIN_PIN    3
#define EXTCOMIN_PORT  13
#define PB0_PORT        5
#define PB0_PIN         6
#define PB1_PORT        5
#define PB1_PIN         7

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

  GPIO_PinModeSet(PB0_PORT, PB0_PIN, gpioModeInputPullFilter, 1);
  GPIO_ExtIntConfig (PB0_PORT, PB0_PIN, PB0_PIN, true, true, true);

  GPIO_PinModeSet(PB1_PORT, PB1_PIN, gpioModeInputPullFilter, 1);
  GPIO_ExtIntConfig (PB1_PORT, PB1_PIN, PB1_PIN, true, true, true);
  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);   //Enabling Interrupt in NVIC
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
}


void gpioLed0SetOn()
{
  //LOG_INFO("LED ON\n\r");
	GPIO_PinOutSet(LED0_port,LED0_pin);
}


void gpioLed0SetOff()
{
  //LOG_INFO("LED OFF\n\r");
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

void gpioSetDisplayExtcomin(bool status)
{
   if(status) {
       GPIO_PinOutSet(EXTCOMIN_PORT,EXTCOMIN_PIN);
   }
   else {
       GPIO_PinOutClear(EXTCOMIN_PORT,EXTCOMIN_PIN);
   }

}



