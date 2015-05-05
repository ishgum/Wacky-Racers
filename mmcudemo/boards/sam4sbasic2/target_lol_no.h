/** @file   target.h
    @author M. P. Hayes, UCECE
    @date   22 April 2015
    @brief 
*/
#ifndef TARGET_H
#define TARGET_H

#include "mat91lib.h"

/* This is for the modified sam4sbasic2 board.  The only difference
   between it and the older board is that it uses an 12 MHz crystal
   instead of a 18.432 MHz crystal.  */

/* System clocks  */
#define F_XTAL 12e6
#define MCU_PLL_MUL 16
#define MCU_PLL_DIV 1
#define MCU_USB_DIV 2
/* 192 MHz  */
#define F_PLL (F_XTAL / MCU_PLL_DIV * MCU_PLL_MUL)
/* 96 MHz  */
#define F_CPU (F_PLL / 2)

#define PIO_LED_R PA23_PIO
#define PIO_LED_Y PA22_PIO
#define PIO_LED_G PA21_PIO

#define LED1_PIO PA0_PIO
#define LED2_PIO PA1_PIO

//#define USB_VBUS_PIO PA24_PIO
#define PIO_USB_VBUS PA24_PIO
#define PIO_AUX_ENABLE PA20_PIO
#define PIO_SW_SLEEP PA0_PIO

#define PIO_H_1 PA16_PIO
#define PIO_H_2 PA15_PIO
#define PIO_H_3 PA14_PIO
#define PIO_H_4 PA13_PIO

#define PIO_DIP_1 PA30_PIO
#define PIO_DIP_2 PA29_PIO
#define PIO_DIP_3 PA28_PIO
#define PIO_DIP_4 PA27_PIO

#define PIO_BT_RESET PA12_PIO
#define PIO_BT_DISCOVER PA11_PIO
#define PIO_BT_RTS PA8_PIO
#define PIO_BT_CTS PA7_PIO
#define PIO_BT_RX PA6_PIO
#define PIO_BT_TX PA5_PIO


#endif /* TARGET_H  */
