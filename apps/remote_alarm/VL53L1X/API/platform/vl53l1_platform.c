
/* 
* This file is part of VL53L1 Platform 
* 
* Copyright (c) 2016, STMicroelectronics - All Rights Reserved 
* 
* License terms: BSD 3-clause "New" or "Revised" License. 
* 
* Redistribution and use in source and binary forms, with or without 
* modification, are permitted provided that the following conditions are met: 
* 
* 1. Redistributions of source code must retain the above copyright notice, this 
* list of conditions and the following disclaimer. 
* 
* 2. Redistributions in binary form must reproduce the above copyright notice, 
* this list of conditions and the following disclaimer in the documentation 
* and/or other materials provided with the distribution. 
* 
* 3. Neither the name of the copyright holder nor the names of its contributors 
* may be used to endorse or promote products derived from this software 
* without specific prior written permission. 
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
* 
*/

#include "vl53l1_platform.h"
#include <string.h>
#include <time.h>
#include <math.h>
#include "i2cm.h"
#include "tmr_utils.h"

uint8_t i2cindex[2];
uint8_t i2cbuff[4];

int8_t VL53L1_WriteMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count) {
	if (count > sizeof(i2cbuff))
		return -1;

	i2cindex[0] = index>>8;
	i2cindex[1] = index & 0xFF;
	memcpy(&i2cbuff[2], pdata, count);

	if (I2CM_Write(MXC_I2CM0, (uint8_t)dev, i2cindex, 2, pdata, count) != count)
		return -1;
	return 0;
}

int8_t VL53L1_ReadMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count) {
	if (count > sizeof(i2cbuff))
		return -1;

	i2cindex[0] = index>>8;
	i2cindex[1] = index & 0xFF;
	memcpy(&i2cbuff[2], pdata, count);

	if (I2CM_Read(MXC_I2CM0, (uint8_t)dev, i2cindex, 2, pdata, count) != count)
		return -1;
	return 0;
}

int8_t VL53L1_WrByte(uint16_t dev, uint16_t index, uint8_t data) {
	i2cindex[0] = index>>8;
	i2cindex[1] = index & 0xFF;
	i2cbuff[0] = data;

	if (I2CM_Write(MXC_I2CM0, (uint8_t)dev, i2cindex, 2, i2cbuff, 1) != 1)
		return -1;
	return 0;
}

int8_t VL53L1_WrWord(uint16_t dev, uint16_t index, uint16_t data) {
	i2cindex[0] = index>>8;
	i2cindex[1] = index & 0xFF;
	i2cbuff[0] = data >> 8;
	i2cbuff[1] = data & 0xFF;

	if (I2CM_Write(MXC_I2CM0, (uint8_t)dev, i2cindex, 2, i2cbuff, 2) != 2)
		return -1;
	return 0;
}

int8_t VL53L1_WrDWord(uint16_t dev, uint16_t index, uint32_t data) {
	i2cindex[0] = index>>8;
	i2cindex[1] = index & 0xFF;
	i2cbuff[0] = (data >> 24) & 0xFF;
	i2cbuff[1] = (data >> 16) & 0xFF;
	i2cbuff[2] = (data >> 8) & 0xFF;
	i2cbuff[3] = data & 0xFF;

	if (I2CM_Write(MXC_I2CM0, (uint8_t)dev, i2cindex, 2, i2cbuff, 4) != 4)
		return -1;
	return 0;
}

int8_t VL53L1_RdByte(uint16_t dev, uint16_t index, uint8_t *data) {
	i2cindex[0] = index>>8;
	i2cindex[1] = index & 0xFF;
	*data = i2cbuff[0];

	if (I2CM_Read(MXC_I2CM0, (uint8_t)dev, i2cindex, 2, i2cbuff, 1) != 1)
		return -1;
	return 0;
}

int8_t VL53L1_RdWord(uint16_t dev, uint16_t index, uint16_t *data) {
	i2cindex[0] = index>>8;
	i2cindex[1] = index & 0xFF;
	*data = i2cbuff[0] << 8 | i2cbuff[1];

	if (I2CM_Read(MXC_I2CM0, (uint8_t)dev, i2cindex, 2, i2cbuff, 2) != 2)
		return -1;
	return 0;
}

int8_t VL53L1_RdDWord(uint16_t dev, uint16_t index, uint32_t *data) {
	i2cindex[0] = index>>8;
	i2cindex[1] = index & 0xFF;
	*data = i2cbuff[0] << 24 | i2cbuff[1] << 16 | i2cbuff[2] << 8 | i2cbuff[3];

	if (I2CM_Read(MXC_I2CM0, (uint8_t)dev, i2cindex, 2, i2cbuff, 4) != 4)
		return -1;
	return 0;
}

int8_t VL53L1_WaitMs(uint16_t dev, int32_t wait_ms){
	TMR_Delay(MXC_TMR0, 1000*wait_ms);
	return 0;
}
