# MAX32630FTHR apps for Eclipse (MAX32630FTHR_apps/apps)

Apps available
1. USB_I2C_MAX3263X - a USB-to-I2C interface via CDC ACM

Usage
1. Copy/clone project files to desired location.
2. Import project to Eclipse by selecting: File > Import...

# MAX32630FTHR board files (MAX32630FTHR_apps/FTHR)
MAX32630FTHR board C source files for the Maxim ARM Cortex Toolchain

Usage
1. Install Maxim ARM Cortex Toolchain from [here](https://www.maximintegrated.com/en/design/software-description.html/swpart=SFW0001500A).
2. Copy and paste "FTHR" directory to %MAXIMDIR%\Firmware\MAX3263X\Libraries\Boards, where %MAXIMDIR& is "C:\Maxim" by default.
3. Open Eclipse.
4. Select on menu: File > New > Maxim Microcontrollers
5. Type project name on "Project name" field. Click next.
6. Set chip type as "MAX3263X".
7. Set board type as "FTHR".
8. Select example any of the examples.
9. Select appropriate adapter type.
10. Edit main.c according to your needs.
11. Run/debug program

For more info about using Eclipse, open README.pdf in %MAXIMDIR%

Differences from EvKit_V1 board
1. Console is set to USB instead of UART.
2. STDIO read() and write() are implemented for USB CDC ACM. (i.e. printf() will print to host via USB COM port)
3. MAX14690 PMIC interrupts are disabled by default.
4. MAX14690 PMIC LDO2 doesn't turn off with the absence of VBUS (due to hardware differences).

