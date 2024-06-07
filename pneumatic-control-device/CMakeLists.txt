# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -save-temps=obj")
set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pump-control)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/include/zephyr)

target_include_directories(app PRIVATE src/sensor)
target_include_directories(app PRIVATE src/tach)
target_include_directories(app PRIVATE src/uart_handle)
target_include_directories(app PRIVATE src/shell)
target_include_directories(app PRIVATE src/lib/motor_ctrl)
target_include_directories(app PRIVATE src/lib/pid_ctrl)

FILE(GLOB sensor src/sensor/*.c)
FILE(GLOB tach src/tach/*.c)
FILE(GLOB uart_handle src/uart_handle/*.c)
FILE(GLOB shell src/shell/*.c)
FILE(GLOB lib src/lib/motor_ctrl/*.c src/lib/pid_ctrl/*.c)

target_sources(app PRIVATE ${sensor})
target_sources(app PRIVATE ${tach})
target_sources(app PRIVATE ${uart_handle})
target_sources(app PRIVATE ${shell})
target_sources(app PRIVATE ${lib})
target_sources(app PRIVATE src/main.c)
