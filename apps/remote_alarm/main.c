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
 * $Id: main.c 23132 2016-06-01 14:26:16Z kevin.gillespie $
 *
 *******************************************************************************
 */

/**
 * @file    main.c
 * @brief   USB CDC-ACM example
 * @details Configures the device as an USB-UART adaptor. After loading the elf file, connect a USB
 *          cable from CN2 to a host PC. CN1 can also be connected for debug messages. For a Windows PC
 *          the driver is the .inf file in the driver folder.
 *          1. LED0 should turn on once enumeration is complete
 *          2. Connect another UART device to communicate with to UART1 (TX = P2.1, RX = P2.0).
 *          3. Open a terminal program for the MAXIM USB-UART adaptor COM port to send/receive data with another
 *             UART device.
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "lp.h"
#include "cdc_acm.h"
#include "i2cm.h"
#include "mxc_config.h"
#include "mxc_sys.h"
#include "ioman.h"
#include "vl53l1_platform.h"
#include "vl53l1X_api.h"
#include "vl53l1X_calibration.h"
#include "tmr_utils.h"
#include "led.h"
#include "gpio.h"

/*
#include "pwrman_regs.h"
#include "board.h"
#include "led.h"
#include "uart.h"
#include "usb.h"
#include "usb_event.h"
#include "enumerate.h"
#include "descriptors.h"
*/

sys_cfg_i2cm_t i2cm0_cfg = { CLKMAN_SCALE_DIV_1 , IOMAN_I2CM(0, 1, I2CM_SPEED_100KHZ) };

void indicate_error(void);

VL53L1X_ERROR Status;
uint16_t distance;
uint8_t state = 1;
uint16_t dev = 0x29;
uint8_t dataReady;
uint8_t rangeStatus;

/******************************************************************************/
int main(void) {
	GPIO_Config(&(gpio_cfg_t){PORT_1, PIN_6 | PIN_7, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL});
	GPIO_Config(&(gpio_cfg_t){PORT_4, PIN_0, GPIO_FUNC_GPIO, GPIO_PAD_INPUT_PULLUP});		// INT

	GPIO_Config(&(gpio_cfg_t){PORT_3, PIN_6, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL});				// 3V3 VDDIOH enable
	GPIO_OutSet(&(gpio_cfg_t){PORT_3, PIN_6, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL});

	GPIO_Config(&(gpio_cfg_t){PORT_2, PIN_2, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL});				// IOH 1W enable
	GPIO_OutSet(&(gpio_cfg_t){PORT_2, PIN_2, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL});

	GPIO_Config(&(gpio_cfg_t){PORT_2, PIN_3, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL});				// HDR select
	GPIO_OutClr(&(gpio_cfg_t){PORT_2, PIN_3, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL});

	MXC_IOMAN->use_vddioh_0 |= (PIN_6|PIN_7)<<(PORT_1*8);									// set i2c to 3.3V

	if (I2CM_Init(MXC_I2CM0, &i2cm0_cfg, I2CM_SPEED_400KHZ) == E_NO_ERROR) {
		//LED_On(0);
	}

	/* Wait for device booted */
	do {
		if ((Status = VL53L1X_BootState(dev, &state)))
			indicate_error();

		TMR_Delay(MXC_TMR1, 1000*500);
		//LED_Toggle(1);
	} while(state);

	/* Sensor Initialization */
	if ((Status = VL53L1X_SensorInit(dev)))
		indicate_error();

	/* Modify the default configuration */
	if ((Status = VL53L1X_SetInterMeasurementInMs(dev, 1000)))
		indicate_error();

	//Status = VL53l1X_SetOffset(dev, );
	/* enable the ranging*/
	if ((Status = VL53L1X_StartRanging(dev)))
		indicate_error();

	LED_On(1);
	/* ranging loop */
	do {
		do {
			LED_Toggle(2);
			Status = VL53L1X_CheckForDataReady(dev, &dataReady);
		} while (dataReady == 0);
		//} while (GPIO_InGet(&(gpio_cfg_t){PORT_4, PIN_0, GPIO_FUNC_GPIO, GPIO_PAD_INPUT_PULLUP}));
		dataReady = 0;
		Status = VL53L1X_GetRangeStatus(dev, &rangeStatus);
		Status = VL53L1X_GetDistance(dev, &distance);
		Status = VL53L1X_ClearInterrupt(dev);
		if (acm_present()) {
			printf("%d\r\n", distance);
			TMR_Delay(MXC_TMR1, 500*1000);
		}
	} while (1);
}

int usb_read_callback(void) {
	printf("%d\r\n", distance);
	/*
	uint8_t usb_read_data;

	acm_read(&usb_read_data, 1);
	if (usb_read_data == '1')
		GPIO_OutToggle(&(gpio_cfg_t){PORT_1, PIN_6, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL});
	else if (usb_read_data == '2')
		GPIO_OutToggle(&(gpio_cfg_t){PORT_1, PIN_7, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL});

	while (acm_canread())
		acm_read(&usb_read_data, 1);
		*/
	return 0;
}

void indicate_error(void) {
	while (1) {
		TMR_Delay(MXC_TMR1, 1000*500);
		Status = VL53L1X_BootState(dev, &state);
		if (Status == 0)
			LED_Toggle(1);
		else
			LED_Toggle(2);
	}
}
