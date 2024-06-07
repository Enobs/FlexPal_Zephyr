/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef __DA1469X_OTP_H_
#define __DA1469X_OTP_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OTP_ERR_INVALID_SIZE_ALIGNMENT -1
#define OTP_ERR_INVALID_ADDRESS -2
#define OTP_ERR_PROGRAM_VERIFY_FAILED -3

#define OTP_SEGMENT_CONFIG          0xc00
#define OTP_SEGMENT_QSPI_FW_KEYS    0xb00
#define OTP_SEGMENT_USER_DATA_KEYS  0xa00
#define OTP_SEGMENT_SIGNATURE_KEYS  0x8c0
#define OTP_SEGMENT_USER_DATA_LEN   0x100

#define OTP_ADDRESS_RANGE_USER_DATA_KEYS(x) \
    (((uint32_t)(x) >= (uint32_t)MCU_OTPM_BASE + OTP_SEGMENT_USER_DATA_KEYS) && \
     ((uint32_t)(x) < (uint32_t)MCU_OTPM_BASE + OTP_SEGMENT_USER_DATA_KEYS + OTP_SEGMENT_USER_DATA_LEN))

enum otpc_mode_val {
    OTPC_MODE_PDOWN = 0,
    OTPC_MODE_DSTBY,
    OTPC_MODE_STBY,
    OTPC_MODE_READ,
    OTPC_MODE_PROG,
    OTPC_MODE_PVFY,
    OTPC_MODE_RINI,
};

static inline void
da1469x_otp_set_mode(enum otpc_mode_val mode)
{
    OTPC->OTPC_MODE_REG = (OTPC->OTPC_MODE_REG &
                           ~OTPC_OTPC_MODE_REG_OTPC_MODE_MODE_Msk) |
                          (mode << OTPC_OTPC_MODE_REG_OTPC_MODE_MODE_Pos);
    while (!(OTPC->OTPC_STAT_REG & OTPC_OTPC_STAT_REG_OTPC_STAT_MRDY_Msk));
}

int da1469x_otp_write(uint32_t address, const void *src,
                             uint32_t num_bytes);

int da1469x_otp_read(uint32_t address, void *dst, uint32_t num_bytes);

void da1469x_otp_init(void);

void da1469x_otp_set_speed(uint32_t clk_speed);


#ifdef __cplusplus
}
#endif

#endif /* __MCU_DA1469X_OTP_H_ */
