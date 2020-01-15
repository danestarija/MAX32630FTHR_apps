# MAX32630FTHR apps (MAX32630FTHR_apps/apps)
Apps available
1. USB_I2C_MAX3263X - a USB-to-I2C interface via CDC ACM

# MAX32630FTHR board files (MAX32630FTHR_apps/FTHR)
MAX32630FTHR board C source files for the Maxim ARM Cortex Toolchain

Installation
Copy and paste "FTHR" directory to %MAXIMDIR%\Firmware\MAX3263X\Libraries\Boards, where %MAXIMDIR& is "C:\Maxim" by default

Usage
1. Install Maxim ARM Cortex Toolchain: https://www.maximintegrated.com/en/design/software-description.html/swpart=SFW0001500A.
2. Open Eclipse.
3. Select on menu: File > New > Maxim Microcontrollers
4. Type project name on "Project name" field. Click next.
5. Set chip type as "MAX3263X".
6. Set board type as "FTHR".
7. Select example any of the examples.
8. Select appropriate adapter type.

Differences from EvKit_V1 board
1. Console is set to USB instead of UART.
2. STDIO read() and write() are implemented for USB CDC ACM. (i.e. printf() will print to host via USB COM port)
3. MAX14690 PMIC interrupts are disabled by default.
4. MAX14690 PMIC LDO2 doesn't turn off with the absence of VBUS (due to hardware differences).

