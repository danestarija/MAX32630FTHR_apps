# USB_I2C_MAX3263X
Usage
1. Assuming the app has been flashed, connect the board to a PC via USB cable.
2. Find the COM port of the board.
   - In windows, this can be done by opening Device Manager or typing "mode" into a command prompt.
   - It is required to force local echo and local line editting, as the app parses incoming data as whole lines.
3. Use a terminal program (such as PuTTY) to send commands to the board

Commands and syntax
- write
  - Description: writes a byte into a register in a slave
  - Syntax: write [slave address in hex] [register address in hex] [data in hex]
  - Example: write AA BB CC
  - Output: "ACK" or "NACK"
- read
  - Description: reads a byte from a register in a slave
  - Syntax: read [slave address in hex] [register address]
  - Example: write AA BB
  - Output: "ACK" or "NACK", register content in hex
- i2c_init
  - Description: re-initialize I2C perihperal module of the board (useful when undefined behaviors are encountered)
  - Syntax: i2c_init
- debug
  - Description: enable debug output; shows the command and the addresses received by the board from the user
  - Syntax: debug
- pmic_init
  - Description: re-initialize the PMIC with a different LDO3 voltage
  - Syntax: pmic_init [ldo3 voltage in mV]
  - Example: pmic_init 3300
  
