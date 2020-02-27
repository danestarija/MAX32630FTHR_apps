/*******************************************************************************
 * Copyright (C) 2016 Maxim Integrated Products, Inc., All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Maxim Integrated
 * Products, Inc. shall not be used except as stated in the Maxim Integrated
 * Products, Inc. Branding Policy.
 *
 * The mere transfer of this software does not imply any licenses
 * of trade secrets, proprietary technology, copyrights, patents,
 * trademarks, maskwork rights, or any other form of intellectual
 * property whatsoever. Maxim Integrated Products, Inc. retains all
 * ownership rights.
 *
 * $Date: 2016-03-11 10:46:02 -0700 (Fri, 11 Mar 2016) $
 * $Revision: 21838 $
 *
 ******************************************************************************/

/**
 * @file    board.h
 * @brief   Board support package API.
 */

#ifndef _BOARD_H
#define _BOARD_H

#include "gpio.h"
#include "spim.h"
#include "ioman.h"
#include "led.h"
#include "pb.h"

#ifdef __cplusplus
extern "C" {
#endif

// Pushbutton Indices
#define SW1             0       /// Pushbutton index for SW1

#define LED_OFF         1       /// Inactive state of LEDs
#define LED_ON          0       /// Active state of LEDs

#define LED_RED		(0) /* (&led_pin[0]) */
#define LED_GREEN	(1) /* &led_pin[1] */
#define LED_BLUE	(2) /* led_pin[2] */

// MAX14690 PMIC
#define MAX14690_I2CM_INST  2           /**< MAX14690 is connected to the I2C Master Port 0 */
#define MAX14690_I2CM       MXC_I2CM2   /**< Using I2C Master 0 Base Peripheral Address. */
extern const sys_cfg_i2cm_t max14690_sys_cfg;
extern const gpio_cfg_t max14690_int;
extern const gpio_cfg_t max14690_mpc0;
extern const gpio_cfg_t max14690_pfn2;

// USB stuff
#define EVENT_ENUM_COMP     MAXUSB_NUM_EVENTS
#define EVENT_REMOTE_WAKE   (EVENT_ENUM_COMP + 1)

#define WORKAROUND_TIMER    0           // Timer used for USB wakeup workaround
#define TMRn_IRQHandler     TMR0_IRQHandler
#define MXC_TMRn            MXC_TMR_GET_TMR(WORKAROUND_TIMER)
#define TMRn_IRQn           MXC_TMR_GET_IRQ_32(WORKAROUND_TIMER)
extern volatile int configured;
extern volatile int suspended;
extern volatile unsigned int event_flags;

extern int usb_read_callback(void);

/**
 * \brief   Initialize the BSP and board interfaces.
 * \returns #E_NO_ERROR if everything is successful
 */
int Board_Init();

/**
 * \brief   Initialize or reinitialize the console. This may be necessary if the
 *          system clock rate is changed.
 * \returns #E_NO_ERROR if everything is successful
 */
int Console_Init();

/**
 * \brief   Attempt to prepare the console for sleep.
 * \returns #E_NO_ERROR if ready to sleep, #E_BUSY if not ready for sleep.
 */
int Console_PrepForSleep(void);

#ifdef __cplusplus
}
#endif

#endif /* _BOARD_H */
