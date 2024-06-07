#!/bin/sh

# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2017 Intel Corporation. All rights reserved.
# Author: Seppo Ingalsuo <seppo.ingalsuo@linux.intel.com>

octave --no-gui src_test_top.m
if [ $? -eq 0 ]; then
    echo "Test passed."
    exit 0
else
    echo "Test failed." >&2
    exit 1
fi
