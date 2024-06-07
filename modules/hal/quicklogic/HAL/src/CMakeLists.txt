# QuickLogic HAL
#
# Copyright (c) 2020 Antmicro Ltd <www.antmicro.com>
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_GPIO_EOS_S3)
	zephyr_sources(eoss3_hal_gpio.c)
	zephyr_sources(eoss3_hal_pad_config.c)
endif()
