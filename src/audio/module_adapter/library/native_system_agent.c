// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2023 Intel Corporation. All rights reserved.
//
// Author: Pawel Dobrowolski <pawelx.dobrowolski@intel.com>

#include <stdbool.h>
#include <stdint.h>
#include <utilities/array.h>
#include <native_system_agent.h>

/* The create_instance_f is a function call type known in module. The module entry_point
 * points to this type of function which starts module creation.
 */

typedef void* (*native_create_instance_f)(uint32_t module_id, uint32_t instance_id,
					  uint32_t core_id, void *mod_cfg, void *parent_ppl,
					  void **mod_ptr, void **mod_ops, void **mod_uuid);

void *native_system_agent_start(uint32_t entry_point, uint32_t module_id, uint32_t instance_id,
				uint32_t core_id, uint32_t log_handle, void *mod_cfg,
				void *mod_interface, void *mod_uuid)
{
	struct NativeSystemAgent native_system_agent;

	native_system_agent.module_id = module_id;
	native_system_agent.instance_id = instance_id;
	native_system_agent.core_id = core_id;
	native_system_agent.log_handle = log_handle;
	void *system_agent_p = (void *)(&native_system_agent);

	native_create_instance_f ci = (native_create_instance_f)(entry_point);

	return ci(module_id, instance_id, core_id, mod_cfg, NULL, &system_agent_p, &mod_interface,
		  &mod_uuid);
}
