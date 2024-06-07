#!/bin/sh
# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2018 Intel Corporation. All rights reserved.

# Runs a given script in the docker container you can generate from the
# docker_build directory.
# Example:
#  ./scripts/docker-qemu.sh ../sof.git/scripts/qemu-check.sh


docker run -i --shm-size=512m --privileged -v `pwd`:/home/sof/sof.git \
	   --user `id -u` sofqemu $@
