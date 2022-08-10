// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2022 Intel Corporation. All rights reserved.
//
// Author: Tomasz Leman <tomasz.m.leman@intel.com>

/**
 * \file
 * \brief Zephyr RTOS CPU implementation file
 * \authors Tomasz Leman <tomasz.m.leman@intel.com>
 */

#include <sof/init.h>
#include <sof/lib/cpu.h>
#include <sof/lib/pm_runtime.h>
#include <sof/lib/wait.h>
#include <sof/trace/trace.h>

/* Zephyr includes */
#include <soc.h>
#include <version.h>
#include <zephyr/pm/pm.h>
#include <zephyr/kernel.h>

extern struct tr_ctx zephyr_tr;

#if CONFIG_MULTICORE && CONFIG_SMP

extern K_KERNEL_STACK_ARRAY_DEFINE(z_interrupt_stacks, CONFIG_MP_NUM_CPUS,
				   CONFIG_ISR_STACK_SIZE);

static atomic_t start_flag;
static atomic_t ready_flag;

/* Zephyr kernel_internal.h interface */
extern void smp_timer_init(void);

static FUNC_NORETURN void secondary_init(void *arg)
{
	struct k_thread dummy_thread;

	/*
	 * This is an open-coded version of zephyr/kernel/smp.c
	 * smp_init_top(). We do this so that we can call SOF
	 * secondary_core_init() for each core.
	 */

	atomic_set(&ready_flag, 1);
	z_smp_thread_init(arg, &dummy_thread);
	smp_timer_init();

	secondary_core_init(sof_get());

#ifdef CONFIG_THREAD_STACK_INFO
	dummy_thread.stack_info.start = (uintptr_t)z_interrupt_stacks +
		arch_curr_cpu()->id * Z_KERNEL_STACK_LEN(CONFIG_ISR_STACK_SIZE);
	dummy_thread.stack_info.size = Z_KERNEL_STACK_LEN(CONFIG_ISR_STACK_SIZE);
#endif

	z_smp_thread_swap();

	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}

int cpu_enable_core(int id)
{
	/* only called from single core, no RMW lock */
	__ASSERT_NO_MSG(cpu_get_id() == PLATFORM_PRIMARY_CORE_ID);
	/*
	 * This is an open-coded version of zephyr/kernel/smp.c
	 * z_smp_start_cpu(). We do this, so we can use a customized
	 * secondary_init() for SOF.
	 */

	if (arch_cpu_active(id))
		return 0;

#if ZEPHYR_VERSION(3, 0, 99) <= ZEPHYR_VERSION_CODE
	z_init_cpu(id);
#endif

	atomic_clear(&start_flag);
	atomic_clear(&ready_flag);

	arch_start_cpu(id, z_interrupt_stacks[id], CONFIG_ISR_STACK_SIZE,
		       secondary_init, &start_flag);

	while (!atomic_get(&ready_flag))
		k_busy_wait(100);

	atomic_set(&start_flag, 1);

	return 0;
}

void cpu_disable_core(int id)
{
	uint64_t timeout = 0;

	if (!arch_cpu_active(id)) {
		tr_warn(&zephyr_tr, "core %d is already disabled", id);
		return;
	}
#if defined(CONFIG_PM)
	/* only called from single core, no RMW lock */
	__ASSERT_NO_MSG(cpu_get_id() == PLATFORM_PRIMARY_CORE_ID);

	/* TODO: before requesting core shut down check if it's not actively used */
	if (!pm_state_force(id, &(struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0})) {
		tr_err(&zephyr_tr, "failed to set PM_STATE_SOFT_OFF on core %d", id);
		return;
	}

	/* Primary core will be turn off by the host after it enter SOFT_OFF state */
	if (cpu_is_primary(id))
		return;

	timeout = k_cycle_get_64() + k_ms_to_cyc_ceil64(CONFIG_SECONDARY_CORE_DISABLING_TIMEOUT);

	/* Waiting for secondary core to enter idle state */
	while (arch_cpu_active(id) && (k_cycle_get_64() < timeout))
		idelay(PLATFORM_DEFAULT_DELAY);

	if (arch_cpu_active(id)) {
		tr_err(&zephyr_tr, "core %d did not enter idle state", id);
		return;
	}

	if (soc_adsp_halt_cpu(id) != 0)
		tr_err(&zephyr_tr, "failed to disable core %d", id);
#endif /* CONFIG_PM */
}

int cpu_enabled_cores(void)
{
	unsigned int i;
	int mask = 0;

	for (i = 0; i < CONFIG_MP_NUM_CPUS; i++)
		if (arch_cpu_active(i))
			mask |= BIT(i);

	return mask;
}

int cpu_restore_secondary_cores(void)
{
	/* TODO: use Zephyr API */
	return 0;
}

int cpu_secondary_cores_prepare_d0ix(void)
{
	/* TODO: use Zephyr API */
	return 0;
}

#endif /* CONFIG_MULTICORE && CONFIG_SMP */
