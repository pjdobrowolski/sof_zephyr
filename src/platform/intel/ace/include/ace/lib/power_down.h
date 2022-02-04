/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2022 Intel Corporation. All rights reserved.
 *
 * Author: Lech Betlej <lech.betlej@linux.intel.com>
 */

#ifndef __ACE_LIB_POWER_DOWN_H__
#define __ACE_LIB_POWER_DOWN_H__

#include <stdbool.h>
#include <stdint.h>

/**
 * Power down procedure.
 * Locks its code in L1 cache and shuts down memories.
 * @param  disable_lpsram        flag if LPSRAM is to be disabled (whole)
 * @param  hpsram_pwrgating_mask pointer to memory segments power gating mask
 * (each bit corresponds to one ebb)
 * @return                       nothing returned - this function never quits
 */
void power_down(bool disable_lpsram, uint32_t *hpsram_pwrgating_mask);

#endif /* __ACE_LIB_POWER_DOWN_H__ */