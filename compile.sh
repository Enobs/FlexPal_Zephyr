#!/bin/bash

if [ "$1" = "flash" ]; then
    st-flash --connect-under-reset write build/zephyr/zephyr.bin 0x8000000
elif [ "$1" = "all" ]; then
    west build -p always -b gd32f407v_start  ./pneumatic-control-device
elif [ "$1" = "menu" ]; then
    west build -t menuconfig
else
    west build -p auto -b gd32f407v_start  ./pneumatic-control-device
fi

