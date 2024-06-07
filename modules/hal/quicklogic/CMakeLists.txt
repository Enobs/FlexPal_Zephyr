# QuickLogic HAL
#
# Copyright (c) 2020 Antmicro Ltd <www.antmicro.com>
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_EOS_S3_HAL)
  zephyr_include_directories(
    HAL/inc
  )
  add_subdirectory(HAL/src)
endif()
