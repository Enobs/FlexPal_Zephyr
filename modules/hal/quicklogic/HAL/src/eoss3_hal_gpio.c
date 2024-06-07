/*
 * ==========================================================
 *
 *    Copyright (C) 2020 QuickLogic Corporation             
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 		http://www.apache.org/licenses/LICENSE-2.0
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 *    File      : eoss3_hal_gpio.c
 *    Purpose   : This file contains macros, structures and APIs to
 *             read/write GPIO
 *                                                          
 *                                                          
 * ===========================================================
 *
 */

#include <stdint.h>
#include <errno.h>
#include "eoss3_hal_gpio.h"
#include "eoss3_hal_def.h"
#include "eoss3_dev.h"

/*!
 * \brief array containing reg offset (wrt S_INTR_SEL_BASE) and correponding register value to be selected for each PAD
    Bit 7:4 - register offset from S_INTR_SEL_BASE (0x40004D3C)
    Bit 3:0 - register value to be written for selecting the particular PAD
*/
const uint8_t gpio_intr_cfg[PAD_COUNT] = {0x0,0x0,0x11,0x01,0x21,0x31,
					   0x12,0x41,0x22,0x32,0x42,0x51,
					   0x61,0x71,0x52,0x62,0x0,0x0,
					   0x13,0x0,0x0,0x23,0x32,0x72,
					   0x14,0x24,0x43,0x53,0x34,0x44,
					   0x54,0x63,0x64,0x73,0x74,0x15,
					   0x16,0x25,0x26,0x35,0x36,0x65,
					   0x75,0x0,0x45,0x55
  
};

void HAL_GPIO_Read(uint8_t ucGpioIndex, uint8_t *ucGpioVal)
{
	if(ucGpioIndex <= GPIO_7)
	{
		*ucGpioVal = ((MISC_CTRL->IO_INPUT >> ucGpioIndex) & 1);
	}
}

void HAL_GPIO_Write(uint8_t ucGpioIndex, uint8_t ucGpioVal)
{
	if((ucGpioIndex <= GPIO_7) && (ucGpioVal <= 1) )
	{
		if(ucGpioVal)
		{
			MISC_CTRL->IO_OUTPUT |=  (1 << ucGpioIndex);
		}
		else
		{
			MISC_CTRL->IO_OUTPUT &= ~(1 << ucGpioIndex);
		}
	}
}

int HAL_GPIO_IntrCfg(GPIOCfgTypeDef *hGpioCfg)
{
	int gpio_irq_num;

	switch (hGpioCfg->xPadConf->ucPin) {
	case PAD_6:
		IO_MUX->S_INTR_1_SEL_REG = 2;
		gpio_irq_num = 1;
		break;
	case PAD_9:
		IO_MUX->S_INTR_3_SEL_REG = 2;
		gpio_irq_num = 3;
		break;
	case PAD_11:
		IO_MUX->S_INTR_5_SEL_REG = 1;
		gpio_irq_num = 5;
		break;
	case PAD_14:
		IO_MUX->S_INTR_5_SEL_REG = 2;
		gpio_irq_num = 5;
		break;
	case PAD_18:
		IO_MUX->S_INTR_1_SEL_REG = 3;
		gpio_irq_num = 1;
		break;
	case PAD_21:
		IO_MUX->S_INTR_2_SEL_REG = 3;
		gpio_irq_num = 2;
		break;
	case PAD_22:
		IO_MUX->S_INTR_3_SEL_REG = 3;
		gpio_irq_num = 3;
		break;
	case PAD_23:
		IO_MUX->S_INTR_7_SEL_REG = 2;
		gpio_irq_num = 7;
		break;
	case PAD_24:
		IO_MUX->S_INTR_1_SEL_REG = 4;
		gpio_irq_num = 1;
		break;
	case PAD_26:
		IO_MUX->S_INTR_4_SEL_REG = 3;
		gpio_irq_num = 4;
		break;
	case PAD_28:
		IO_MUX->S_INTR_3_SEL_REG = 4;
		gpio_irq_num = 3;
		break;
	case PAD_30:
		IO_MUX->S_INTR_5_SEL_REG = 4;
		gpio_irq_num = 5;
		break;
	case PAD_31:
		IO_MUX->S_INTR_6_SEL_REG = 3;
		gpio_irq_num = 6;
		break;
	case PAD_36:
		IO_MUX->S_INTR_1_SEL_REG = 6;
		gpio_irq_num = 1;
		break;
	case PAD_38:
		IO_MUX->S_INTR_2_SEL_REG = 6;
		gpio_irq_num = 2;
		break;
	case PAD_45:
		IO_MUX->S_INTR_5_SEL_REG = 5;
		gpio_irq_num = 5;
		break;
	default:
		return -EINVAL;

	};

	INTR_CTRL->GPIO_INTR_TYPE &= (~((UINT32_t) (0x1) << gpio_irq_num)); //zero the bit position
	INTR_CTRL->GPIO_INTR_TYPE |= (hGpioCfg->intr_type << gpio_irq_num); //update the bit with desired value

	INTR_CTRL->GPIO_INTR_POL &= (~((UINT32_t) (0x1) << gpio_irq_num)); //zero the bit position
	INTR_CTRL->GPIO_INTR_POL |= (hGpioCfg->pol_type << gpio_irq_num); //update the bit with desired value

	return gpio_irq_num;
}
