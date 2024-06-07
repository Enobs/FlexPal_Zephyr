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
 *    File      : eoss3_hal_gpio.h
 *    Purpose   : This file contains macros, structures and APIs to
 *             read/write GPIO 
 *                                                          
 *                                                          
 * ===========================================================
 *
 */

#ifndef __EOSS3_HAL_GPIO_H
#define __EOSS3_HAL_GPIO_H

#include "eoss3_dev.h"
#include "eoss3_hal_pad_config.h"

#include "eoss3_hal_def.h"

#ifdef __cplusplus
 extern "C" {
#endif

/*! \def GPIO_0
    \brief GPIO index 0.
*/
#define GPIO_0	0
   
/*! \def GPIO_1
    \brief GPIO index 1.
*/   
#define GPIO_1	1
   
/*! \def GPIO_2
    \brief GPIO index 3.
*/   
#define GPIO_2	2
   
/*! \def GPIO_3
    \brief GPIO index 3.
*/   
#define GPIO_3	3
   
/*! \def GPIO_4
    \brief GPIO index 4.
*/   
#define GPIO_4	4
   
/*! \def GPIO_5
    \brief GPIO index 5.
*/   
#define GPIO_5	5
   
/*! \def GPIO_6
    \brief GPIO index 6.
*/   
#define GPIO_6	6
   
/*! \def GPIO_7
    \brief GPIO index 7.
*/   
#define GPIO_7	7

#define PAD_COUNT	46

#define S_INTR_SEL_BASE		((volatile uint32_t *)0x40004D3C)

typedef enum
{
	LEVEL_TRIGGERED = 0,
	EDGE_TRIGGERED = 1
}GPIO_INTR_TYPE;

typedef enum
{
	FALL_LOW = 0,
	RISE_HIGH = 1
}GPIO_POL_TYPE;

/*!
 * \brief GPIO Configuration Structure definition
 */
typedef struct __GPIOCfgTypeDef
{
	UINT8_t   ucGpioNum;
	PadConfig *xPadConf;
	GPIO_INTR_TYPE	intr_type;
	GPIO_POL_TYPE	pol_type;	
	
}GPIOCfgTypeDef;


/*! \fn HAL_GPIO_Read(uint8_t ucGpioIndex, uint8_t *ucGpioVal)
    \brief GPIO read function to read pad status. Before using this function 
      given pad needs to be initalized.
    \param ucGpioIndex - GPIO index that needs to be read.
    \param *ucGpioVal - GPIO status read, return value pointer.
    \return None
*/   
void HAL_GPIO_Read(uint8_t ucGpioIndex, uint8_t *ucGpioVal);


 /*! \fn HAL_GPIO_Write(uint8_t ucGpioIndex, uint8_t ucGpioVal)
    \brief GPIO write function to write to pad . Before using this function 
      given pad needs to be initalized.
    \param ucGpioIndex - GPIO index that needs to be written.
    \param ucGpioVal - GPIO status that needs to be written.
    \return None
*/
void HAL_GPIO_Write(uint8_t ucGpioIndex, uint8_t ucGpioVal);

 /*! \fn HAL_StatusTypeDef  HAL_GPIO_IntrCfg(GPIOCfgTypeDef *hGpioCfg)
    \brief Function to write to configure pad as GPIO interrupt in INTR_CTRL and IO_MUX.
    \param hGpioCfg - pointer to GPIO configuration structure.
    \return gpio IRQ number
*/
int HAL_GPIO_IntrCfg(GPIOCfgTypeDef *hGpioCfg);

#ifdef __cplusplus
}
#endif

#endif /* __EOSS3_HAL_PADS_H */
