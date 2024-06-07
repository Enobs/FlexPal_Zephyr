#!/bin/bash

cp ./0001-delete-dts-config.patch ../../zephyr/
cd ../../zephyr/
git apply 0001-delete-dts-config.patch

cd -

cp ./0001-modify-hardware-clock.patch  ../../modules/hal/gigadevice
cd ../../modules/hal/gigadevice
git apply 0001-modify-hardware-clock.patch

cd -

cp ./0001-modify-timer0-pinctrl.patch  ../../modules/hal/gigadevice
cd ../../modules/hal/gigadevice
git apply 0001-modify-timer0-pinctrl.patch