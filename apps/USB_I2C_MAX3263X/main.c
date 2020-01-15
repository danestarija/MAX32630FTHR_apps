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
 * $Date: 2018-03-19 09:28:32 -0500 (Mon, 19 Mar 2018) $
 * $Revision: 34022 $
 *
 *******************************************************************************
 */

/**
 * @file    main.c
 * @brief   USB-I2C bridge
 * @details Connects a host to an I2C device via USB CDC class interface
 * @notes   none
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "mxc_config.h"
#include "mxc_sys.h"
#include "pwrman_regs.h"
#include "board.h"
#include "lp.h"
#include "led.h"
#include "cdc_acm.h"
#include "i2cm.h"
#include "i2cs.h"
#include "max14690.h"

/***** Definitions *****//***** Definitions *****/
#define LOOPBACK			0	/* 1: Master-I2CM2, I2CS connected to I2CM2; else: Master-I2CM1, LDO3=3.3V */

#if (LOOPBACK == 1)
#define I2C_MASTER          MXC_I2CM2
#define I2C_SLAVE			MXC_I2CS
#define I2C_SLAVE_SPEED     I2CS_SPEED_100KHZ
#else
#define I2C_MASTER			MXC_I2CM1
#endif

#define I2C_SPEED           I2CM_SPEED_100KHZ

/***** Global Data *****/
max14690_cfg_t all_ldo_en_3V3 = { MAX14690_LDO_ENABLED, 3300, MAX14690_LDO_ENABLED, 3300 };

/***** Function Prototypes *****/
int usb_read_callback(void);

/***** File Scope Variables *****/

#if (LOOPBACK == 1)
static sys_cfg_i2cm_t i2cm_sys_cfg = { CLKMAN_SCALE_DIV_1, IOMAN_I2CM2(IOMAN_MAP_A, 1) };
static sys_cfg_i2cs_t i2cs_sys_cfg = { CLKMAN_SCALE_DIV_1, IOMAN_I2CS(IOMAN_MAP_B, 1) };
#else
static sys_cfg_i2cm_t i2cm_sys_cfg = { CLKMAN_SCALE_DIV_1, IOMAN_I2CM1(IOMAN_MAP_A, 1) };
#endif

static uint8_t usb_data[32];
static uint8_t *argv[4];
static uint8_t debug_en = 0;

/******************************************************************************/
int main(void) {

    if (Board_Init() == E_NO_ERROR)
    	LED_On(2);
    else
    	LED_Off(2);

    /* Configure I2C pins to use VDDIOH */
    MXC_IOMAN->use_vddioh_0 |= 0x30000000;

    /* Initialize I2C master */
    I2CM_Init(I2C_MASTER, &i2cm_sys_cfg, I2C_SPEED);

#if (LOOPBACK == 1)
    /* Initialize I2C slave */
    I2CS_Init(I2C_SLAVE, &i2cs_sys_cfg, I2C_SLAVE_SPEED, 0x76, I2CS_ADDR_8);
#else
    /* Initialize PMIC */
    if (MAX14690_Init(&all_ldo_en_3V3) != E_NO_ERROR)
    	LED_Off(1);
    else
    	LED_On(1);
#endif

    /* Wait for events */
    while (1) {
        if (suspended || !configured)
            LED_Off(0);
        else
            LED_On(0);

        LP_EnterLP2();
    }
}

/******************************************************************************/
static void parse(uint8_t *buffer, uint8_t *argv[], uint8_t c) {
	uint8_t *ptr;
	uint8_t i;

    i = 0;
    ptr = buffer;

    while (i<c && *ptr != '\n' && *ptr != '\0') {
        argv[i++] = ptr;
        while (*ptr != ' ' && *ptr != '\n' && *ptr != '\0')
            ptr++;
        *(ptr++) = '\0';
    }
}

/******************************************************************************/
int usb_read_callback(void) {
	int chars;
	long i, r = 1;
	int retval;
	uint8_t slave_addr = 0;
	uint8_t data[2];

#if (LOOPBACK == 1)
	uint8_t s_data;
#endif

	static enum {write, read, i2c_init, debug, pmic_init, invalid} command;

	if ((chars = acm_canread()) > 0) {
	    if (chars > sizeof(usb_data)-1)
			chars = sizeof(usb_data)-1;

	    // Read the data from USB
	    if (acm_read(usb_data, chars) != chars)
			return 0;
	    usb_data[chars] = '\0';

	    // Transfer to I2CM
	    if (acm_present() && usb_data[0] != '\r' && usb_data[0] != '\n' ) {
	    	parse(usb_data, argv, 4);

			if (!strcmp((char*)argv[0], "/")) {
				/* repeat command, do nothing else */
			} else if (!strcmp((char*)argv[0], "r") && (command == write || command == read) ) {
				r = strtol((char*)argv[1], NULL, 10);
			} else {
				r = 1;
		    	slave_addr = strtol((char*)argv[1], NULL, 16);
		    	data[0] = strtol((char*)argv[2], NULL, 16);
				data[1] = strtol((char*)argv[3], NULL, 16);
				if (!strcmp((char*)argv[0], "write")) {
					command = write;
				} else if (!strcmp((char*)argv[0], "read")) {
					command = read;
				} else if (!strcmp((char*)argv[0], "i2c_init")) {
					command = i2c_init;
				} else if (!strcmp((char*)argv[0], "debug")) {
					command = debug;
				} else if (!strcmp((char*)argv[0], "pmic_init")) {
					command = pmic_init;
				} else {
					command = invalid;
				}
			}

			/* debug info */
			if (debug_en && (command == write || command == read)) {
				printf("command: %s\r\n", argv[0]);
				printf("slave address: 0x%02X\r\n", slave_addr);
				printf("data: 0x%02X\r\n", data[0]);
				printf("data: 0x%02X\r\n", data[1]);
			}

			for (i=0; i<r; i++) {
				switch (command) {
					case write:
						/* Check number of ACKed bytes */
						if((retval = I2CM_Write(I2C_MASTER, slave_addr, NULL, 0, data, 2)) > 0) {
							/* If number of sent bytes == ACKed bytes, print "ACK" */
							printf("ACK: %d\r\n", retval);
#if (LOOPBACK == 1)
							/* Check if slave received the same data */
							s_data = I2CS_Read(I2C_SLAVE, data[0]);
							if (s_data == data[1]) {
								printf("Slave data matches master data: %x - %x\r\n", s_data, data[1]);
							} else {
								printf("Slave data DOES NOT match master data: %x - %x\r\n", s_data, data[1]);
							}
#endif
						} else {
							/* else, print "NACK" */
							printf("NACK: %d\r\n", retval);
						}
						break;
					case read:
						/* Check number of read bytes */
						if((retval = I2CM_Read(I2C_MASTER, slave_addr, NULL, 0, data, 2)) > 0) {
							printf("ACK: %d\r\n", retval);
						} else {
							printf("NACK: %d\r\n", retval);
						}
						break;
					case i2c_init:
						/* master */
						printf("initializing i2c master... ");
						if (I2CM_Init(I2C_MASTER, &i2cm_sys_cfg, I2C_SPEED) != E_NO_ERROR) {
							printf("I2CM init failed\r\n");
						} else {
							printf("I2CM init successful\r\n");
						}
#if (LOOPBACK == 1)
						/* slave */
						printf("initializing i2c slave... ");
						if (I2CS_Init(I2C_SLAVE, &i2cs_sys_cfg, I2C_SLAVE_SPEED, 0x76, I2CS_ADDR_8) != E_NO_ERROR) {
							printf("I2CS init failed\r\n");
						} else {
							printf("I2CS init successful\r\n");
						}
#endif
						break;
					case debug:
						debug_en = ~debug_en;
						printf("debug toggled %s\r\n", debug_en? "ON" : "OFF");
						break;
					case pmic_init:
						if (MAX14690_Init(&all_ldo_en_3V3) != E_NO_ERROR)
							printf("PMIC init successful\r\n");
						else
							printf("PMIC Init failed\r\n");
						break;
					default:
						printf("invalid command\r\n");
				}
			}

			printf("\r\n");
	    }
	}
    return 0;
}
