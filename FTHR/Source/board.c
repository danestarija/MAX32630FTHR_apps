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
 * $Date: 2016-03-17 14:27:29 -0700 (Thu, 17 Mar 2016) $
 * $Revision: 21966 $
 *
 ******************************************************************************/

#include <stdio.h>
#include "mxc_config.h"
#include "mxc_assert.h"
#include "board.h"
#include "gpio.h"
#include "spim.h"
#include "max14690.h"
#include "mxc_sys.h"
#include "pwrman_regs.h"
#include "lp.h"
#include "led.h"
#include "tmr.h"
#include "usb.h"
#include "usb_event.h"
#include "enumerate.h"
#include "cdc_acm.h"
#include "descriptors.h"

/***** Global Variables *****/
// USB Stuff
volatile int configured;
volatile int suspended;
volatile unsigned int event_flags;
int remote_wake_en;

/* This EP assignment must match the Configuration Descriptor */
static const acm_cfg_t acm_cfg = {
	1,                  /* EP OUT */
	MXC_USB_MAX_PACKET, /* OUT max packet size */
	2,                  /* EP IN */
	MXC_USB_MAX_PACKET, /* IN max packet size */
	3,                  /* EP Notify */
	MXC_USB_MAX_PACKET, /* Notify max packet size */
};

// LEDs
// Note: FTHR board uses 3.3v supply so these must be open-drain.
const gpio_cfg_t led_pin[] = {
    { PORT_2, PIN_4, GPIO_FUNC_GPIO, GPIO_PAD_OPEN_DRAIN },
    { PORT_2, PIN_5, GPIO_FUNC_GPIO, GPIO_PAD_OPEN_DRAIN },
    { PORT_2, PIN_6, GPIO_FUNC_GPIO, GPIO_PAD_OPEN_DRAIN },
};
const unsigned int num_leds = (sizeof(led_pin) / sizeof(gpio_cfg_t));

// Pushbuttons
const gpio_cfg_t pb_pin[] = {
    { PORT_2, PIN_3, GPIO_FUNC_GPIO, GPIO_PAD_INPUT_PULLUP },
};
const unsigned int num_pbs = (sizeof(pb_pin) / sizeof(gpio_cfg_t));

// MAX14690 PMIC
const max14690_cfg_t max14690_cfg = {
	.ldo2mv = 3300,
	.ldo2mode = MAX14690_LDO_ENABLED,
	.ldo3mv = 3300,
	.ldo3mode = MAX14690_LDO_ENABLED
};

const sys_cfg_i2cm_t max14690_sys_cfg = {
        .clk_scale = CLKMAN_SCALE_DIV_1,
        .io_cfg = IOMAN_I2CM2(IOMAN_MAP_A, 1)
};

const gpio_cfg_t max14690_int   = { PORT_4, PIN_4, GPIO_FUNC_GPIO, GPIO_PAD_INPUT_PULLUP };
const gpio_cfg_t max14690_mpc0  = { PORT_4, PIN_5, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL };

/***** Function Prototypes *****/
static int setconfig_callback(usb_setup_pkt *sud, void *cbdata);
static int setfeature_callback(usb_setup_pkt *sud, void *cbdata);
static int clrfeature_callback(usb_setup_pkt *sud, void *cbdata);
static int event_callback(maxusb_event_t evt, void *data);
static void usb_app_sleep(void);
static void usb_app_wakeup(void);


/******************************************************************************/
int Board_Init()
{
    int err;

    if ((err = Console_Init()) != E_NO_ERROR) {
        MXC_ASSERT_FAIL();
        return err;
    }

    if ((err = LED_Init()) != E_NO_ERROR) {
        MXC_ASSERT_FAIL();
        return err;
    }

    if ((err = PB_Init()) != E_NO_ERROR) {
        MXC_ASSERT_FAIL();
        return err;
    }

    /* On the Pegasus board MPC1 is connected to CAP which is high when VBUS is present.
     * The LDO_OUTPUT_MPC1 setting will automatically enable the output when VBUS is present.
     * The LDO_OUTPUT_MPC1 setting will also disable the output when powered from the battery.
     * The Pegasus board uses LDO2 for VDDB (USB), LEDs and the SD card connector.
     * Use the MAX14690_LDO2setMode(mode) function to enable LDO2 when needed.
     */
    /* Configure PMIC voltages */
	if ((err = MAX14690_Init(&max14690_cfg)) != E_NO_ERROR) {
		MXC_ASSERT_FAIL();
		return err;
	}

    return E_NO_ERROR;
}

/* ************************************************************************** */
int Console_Init()
{
	/* Initialize state */
	configured = 0;
	suspended = 0;
	event_flags = 0;
	remote_wake_en = 0;

	/* Enable the USB clock and power */
	SYS_USB_Enable(1);

	/* Initialize the usb module */
	if (usb_init(NULL) != 0) {
		while (1);
	}

	/* Initialize the enumeration module */
	if (enum_init() != 0) {
		while (1);
	}

	/* Register enumeration data */
	enum_register_descriptor(ENUM_DESC_DEVICE, (uint8_t*)&device_descriptor, 0);
	enum_register_descriptor(ENUM_DESC_CONFIG, (uint8_t*)&config_descriptor, 0);
	enum_register_descriptor(ENUM_DESC_STRING, lang_id_desc, 0);
	enum_register_descriptor(ENUM_DESC_STRING, mfg_id_desc, 1);
	enum_register_descriptor(ENUM_DESC_STRING, prod_id_desc, 2);

	/* Handle configuration */
	enum_register_callback(ENUM_SETCONFIG, setconfig_callback, NULL);

	/* Handle feature set/clear */
	enum_register_callback(ENUM_SETFEATURE, setfeature_callback, NULL);
	enum_register_callback(ENUM_CLRFEATURE, clrfeature_callback, NULL);

	/* Initialize the class driver */
	if (acm_init() != 0) {
		while (1);
	}

	/* Register callbacks */
	usb_event_enable(MAXUSB_EVENT_NOVBUS, event_callback, NULL);
	usb_event_enable(MAXUSB_EVENT_VBUS, event_callback, NULL);
	acm_register_callback(ACM_CB_READ_READY, usb_read_callback);

	/* Start with USB in low power mode */
	usb_app_sleep();
	NVIC_EnableIRQ(USB_IRQn);

    return E_NO_ERROR;
}

/******************************************************************************/
static int setconfig_callback(usb_setup_pkt *sud, void *cbdata)
{
    /* Confirm the configuration value */
    if (sud->wValue == config_descriptor.config_descriptor.bConfigurationValue) {
        configured = 1;
        MXC_SETBIT(&event_flags, EVENT_ENUM_COMP);
        return acm_configure(&acm_cfg); /* Configure the device class */
    } else if (sud->wValue == 0) {
        configured = 0;
        return acm_deconfigure();
    }

    return -1;
}

/******************************************************************************/
static int setfeature_callback(usb_setup_pkt *sud, void *cbdata)
{
    if(sud->wValue == FEAT_REMOTE_WAKE) {
        remote_wake_en = 1;
    } else {
        // Unknown callback
        return -1;
    }

    return 0;
}

/******************************************************************************/
static int clrfeature_callback(usb_setup_pkt *sud, void *cbdata)
{
    if(sud->wValue == FEAT_REMOTE_WAKE) {
        remote_wake_en = 0;
    } else {
        // Unknown callback
        return -1;
    }

    return 0;
}

/******************************************************************************/
static void usb_app_sleep(void)
{
    usb_sleep();
    MXC_PWRMAN->pwr_rst_ctrl &= ~MXC_F_PWRMAN_PWR_RST_CTRL_USB_POWERED;
    if (MXC_USB->dev_cn & MXC_F_USB_DEV_CN_CONNECT) {
        usb_event_clear(MAXUSB_EVENT_DPACT);
        usb_event_enable(MAXUSB_EVENT_DPACT, event_callback, NULL);
    } else {
        usb_event_disable(MAXUSB_EVENT_DPACT);
    }
    suspended = 1;
}

/******************************************************************************/
static void usb_app_wakeup(void)
{
    usb_event_disable(MAXUSB_EVENT_DPACT);
    MXC_PWRMAN->pwr_rst_ctrl |= MXC_F_PWRMAN_PWR_RST_CTRL_USB_POWERED;
    usb_wakeup();
    suspended = 0;
}

/******************************************************************************/
static int event_callback(maxusb_event_t evt, void *data)
{
    /* Set event flag */
    MXC_SETBIT(&event_flags, evt);

    switch (evt) {
        case MAXUSB_EVENT_NOVBUS:
            usb_event_disable(MAXUSB_EVENT_BRST);
            usb_event_disable(MAXUSB_EVENT_SUSP);
            usb_event_disable(MAXUSB_EVENT_DPACT);
            usb_disconnect();
            configured = 0;
            enum_clearconfig();
            acm_deconfigure();
            usb_app_sleep();
            break;
        case MAXUSB_EVENT_VBUS:
            usb_event_clear(MAXUSB_EVENT_BRST);
            usb_event_enable(MAXUSB_EVENT_BRST, event_callback, NULL);
            usb_event_clear(MAXUSB_EVENT_SUSP);
            usb_event_enable(MAXUSB_EVENT_SUSP, event_callback, NULL);
            usb_connect();
            usb_app_sleep();
            break;
        case MAXUSB_EVENT_BRST:
            TMR32_Stop(MXC_TMRn);   /* No need for workaround if bus reset */
            usb_app_wakeup();
            enum_clearconfig();
            acm_deconfigure();
            configured = 0;
            suspended = 0;
            break;
        case MAXUSB_EVENT_SUSP:
            usb_app_sleep();
            break;
        case MAXUSB_EVENT_DPACT:
            usb_app_wakeup();
            /* Workaround to reset internal suspend timer flag. Wait to determine if this is resume signaling or a bus reset. */
            TMR_Init(MXC_TMRn, TMR_PRESCALE_DIV_2_5, NULL);
            tmr32_cfg_t tmr_cfg;
            tmr_cfg.mode = TMR32_MODE_ONE_SHOT;
            tmr_cfg.polarity = TMR_POLARITY_UNUSED;
            TMR32_TimeToTicks(MXC_TMRn, 30, TMR_UNIT_MICROSEC, &tmr_cfg.compareCount);
            TMR32_Config(MXC_TMRn, &tmr_cfg);
            TMR32_EnableINT(MXC_TMRn);
            NVIC_EnableIRQ(TMRn_IRQn);
            TMR32_Start(MXC_TMRn);
            MXC_CLRBIT(&event_flags, MAXUSB_EVENT_DPACT);   /* delay until we know this is resume signaling */
            break;
        default:
            break;
    }

    return 0;
}

/******************************************************************************/
void USB_IRQHandler(void)
{
    usb_event_handler();
}

/******************************************************************************/
void TMRn_IRQHandler(void)
{
    TMR32_Stop(MXC_TMRn);
    TMR32_ClearFlag(MXC_TMRn);
    NVIC_DisableIRQ(TMRn_IRQn);

    /* No Bus Reset occurred. We woke due to resume signaling from the host.
     * Workaround to reset internal suspend timer flag. The host will hold
     * the resume signaling for 20ms. This remote wakeup signaling will
     * complete before the host sets the bus to idle.
     */
    usb_remote_wakeup();
    MXC_SETBIT(&event_flags, MAXUSB_EVENT_DPACT);
}

/******************************************************************************/
void FaultISR_C(uint32_t *hardfault_args)
{
    /* spin so we can use a debugger to anlayze the situation */
    while(1);
    /* reset the system */
    //NVIC_SystemReset();
}

/******************************************************************************/
void HardFault_Handler(void)
{
	printf("HardFault_Handler! (main)\n");
    __asm(
        " TST LR, #4\n"
        " ITE EQ \n"
        " MRSEQ R0, MSP \n"
        " MRSNE R0, PSP \n"
        " B FaultISR_C \n");
}

/******************************************************************************/
void mxc_assert(const char *expr, const char *file, int line)
{
    printf("MXC_ASSERT %s #%d: (%s)\n", file, line, expr);
    while (1);
}
