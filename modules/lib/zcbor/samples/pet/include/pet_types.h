/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generated using zcbor version 0.7.0
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 3
 */

#ifndef PET_TYPES_H__
#define PET_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <zcbor_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Which value for --default-max-qty this file was created with.
 *
 *  The define is used in the other generated file to do a build-time
 *  compatibility check.
 *
 *  See `zcbor --help` for more information about --default-max-qty
 */
#define DEFAULT_MAX_QTY 3

struct Pet {
	struct zcbor_string names[3];
	size_t names_count;
	struct zcbor_string birthday;
	enum {
		_Pet_species_cat = 1,
		_Pet_species_dog = 2,
		_Pet_species_other = 3,
	} species_choice;
};

#ifdef __cplusplus
}
#endif

#endif /* PET_TYPES_H__ */
