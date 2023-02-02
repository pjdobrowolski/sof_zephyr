/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2023 Intel Corporation. All rights reserved.
 *
 * Author: Pawel Dobrowolski <pawelx.dobrowolski@intel.com>
 */

#ifndef __NATIVE_SYSTEM_AGENT__
#define __NATIVE_SYSTEM_AGENT__

#include <sof/audio/module_adapter/module/module_interface.h>
#include <system_service.h>

struct NativeSystemAgent {
	AdspSystemService system_service;
	uint32_t log_handle;
	uint32_t core_id;
	uint32_t module_id;
	uint32_t instance_id;
	uint32_t module_size;
};

void *native_system_agent_start(uint32_t entry_point, uint32_t module_id, uint32_t instance_id,
				uint32_t core_id, uint32_t log_handle, void *mod_cfg,
				void *mod_interface, void *mod_uuid);

#endif /*  */
