// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright(c) 2020 Intel Corporation. All rights reserved.
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 */

#include <sof/init.h>
#include <sof/lib/alloc.h>
#include <sof/drivers/idc.h>
#include <sof/drivers/interrupt.h>
#include <sof/drivers/interrupt-map.h>
#include <sof/lib/dma.h>
#include <sof/schedule/schedule.h>
#include <platform/drivers/interrupt.h>
#include <platform/lib/memory.h>
#include <sof/platform.h>
#include <sof/lib/notifier.h>
#include <sof/lib/pm_runtime.h>
#include <sof/audio/pipeline.h>
#include <sof/audio/component_ext.h>
#include <sof/trace/trace.h>

/* Zephyr includes */
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/__assert.h>
#include <version.h>
#include <soc.h>

#if !CONFIG_KERNEL_COHERENCE
#include <zephyr/arch/xtensa/cache.h>
#endif

LOG_MODULE_REGISTER(zephyr, CONFIG_SOF_LOG_LEVEL);

/* 300aaad4-45d2-8313-25d0-5e1d6086cdd1 */
DECLARE_SOF_RT_UUID("zephyr", zephyr_uuid, 0x300aaad4, 0x45d2, 0x8313,
		 0x25, 0xd0, 0x5e, 0x1d, 0x60, 0x86, 0xcd, 0xd1);

DECLARE_TR_CTX(zephyr_tr, SOF_UUID(zephyr_uuid), LOG_LEVEL_INFO);

/*
 * Interrupts.
 *
 * Mostly mapped. Still needs some linkage symbols that can be removed later.
 */

/* needed for linkage only */
const char irq_name_level2[] = "level2";
const char irq_name_level5[] = "level5";

int interrupt_register(uint32_t irq, void(*handler)(void *arg), void *arg)
{
#ifdef CONFIG_DYNAMIC_INTERRUPTS
	return arch_irq_connect_dynamic(irq, 0, (void (*)(const void *))handler,
					arg, 0);
#else
	tr_err(&zephyr_tr, "Cannot register handler for IRQ %u: dynamic IRQs are disabled",
		irq);
	return -EOPNOTSUPP;
#endif
}

#if !CONFIG_LIBRARY
/* unregister an IRQ handler - matches on IRQ number and data ptr */
void interrupt_unregister(uint32_t irq, const void *arg)
{
	/*
	 * There is no "unregister" (or "disconnect") for
	 * interrupts in Zephyr.
	 */
	z_soc_irq_disable(irq);
}

/* enable an interrupt source - IRQ needs mapped to Zephyr,
 * arg is used to match.
 */
uint32_t interrupt_enable(uint32_t irq, void *arg)
{
	z_soc_irq_enable(irq);

	return 0;
}

/* disable interrupt */
uint32_t interrupt_disable(uint32_t irq, void *arg)
{
	z_soc_irq_disable(irq);

	return 0;
}
#endif

/*
 * i.MX uses the IRQ_STEER
 */
#if !CONFIG_IMX
/*
 * CAVS IRQs are multilevel whereas BYT and BDW are DSP level only.
 */
int interrupt_get_irq(unsigned int irq, const char *cascade)
{
#if CONFIG_SOC_SERIES_INTEL_ADSP_BAYTRAIL ||\
	CONFIG_SOC_SERIES_INTEL_ADSP_BROADWELL || \
	CONFIG_LIBRARY
	return irq;
#else
	if (cascade == irq_name_level2)
		return SOC_AGGREGATE_IRQ(irq, IRQ_NUM_EXT_LEVEL2);
	if (cascade == irq_name_level5)
		return SOC_AGGREGATE_IRQ(irq, IRQ_NUM_EXT_LEVEL5);

	return SOC_AGGREGATE_IRQ(0, irq);
#endif
}

void interrupt_mask(uint32_t irq, unsigned int cpu)
{
	/* TODO: how do we mask on other cores with Zephyr APIs */
}

void interrupt_unmask(uint32_t irq, unsigned int cpu)
{
	/* TODO: how do we unmask on other cores with Zephyr APIs */
}

void platform_interrupt_init(void)
{
	/* handled by zephyr - needed for linkage */
}

void platform_interrupt_set(uint32_t irq)
{
	/* handled by zephyr - needed for linkage */
}

void platform_interrupt_clear(uint32_t irq, uint32_t mask)
{
	/* handled by zephyr - needed for linkage */
}
#endif

/*
 * Notifier.
 *
 * Use SOF inter component messaging today. Zephyr has similar APIs that will
 * need some minor feature updates prior to merge. i.e. FW to host messages.
 * TODO: align with Zephyr API when ready.
 */

static struct notify *host_notify[CONFIG_CORE_COUNT];

struct notify **arch_notify_get(void)
{
	return host_notify + cpu_get_id();
}

/*
 * Debug
 */
void arch_dump_regs_a(void *dump_buf)
{
	/* needed for linkage only */
}

/*
 * Xtensa. TODO: this needs removed and fixed in SOF.
 */
unsigned int _xtos_ints_off(unsigned int mask)
{
	/* turn all local IRQs OFF */
	irq_lock();
	return 0;
}

void ipc_send_queued_msg(void);

static void ipc_send_queued_callback(void *private_data, enum notify_id event_type,
				     void *caller_data)
{
	if (!ipc_get()->pm_prepare_D3)
		ipc_send_queued_msg();
}

/*
 * Audio components.
 *
 * Integrated except for linkage so symbols are "used" here until linker
 * support is ready in Zephyr. TODO: fix component linkage in Zephyr.
 */

/* TODO: this is not yet working with Zephyr - section has been created but
 *  no symbols are being loaded into ELF file.
 */
extern intptr_t _module_init_start;
extern intptr_t _module_init_end;

static void sys_module_init(void)
{
#if !CONFIG_LIBRARY
	intptr_t *module_init = (intptr_t *)(&_module_init_start);

	for (; module_init < (intptr_t *)&_module_init_end; ++module_init)
		((void(*)(void))(*module_init))();
#endif
}

/*
 * TODO: all the audio processing components/modules constructor should be
 * linked to the module_init section, but this is not happening. Just call
 * constructors directly atm.
 */

void sys_comp_host_init(void);
void sys_comp_mixer_init(void);
void sys_comp_dai_init(void);
void sys_comp_src_init(void);
void sys_comp_mux_init(void);
void sys_comp_selector_init(void);
void sys_comp_switch_init(void);
void sys_comp_tone_init(void);
void sys_comp_eq_fir_init(void);
void sys_comp_keyword_init(void);
void sys_comp_asrc_init(void);
void sys_comp_dcblock_init(void);
void sys_comp_eq_iir_init(void);
void sys_comp_kpb_init(void);
void sys_comp_smart_amp_init(void);
void sys_comp_basefw_init(void);
void sys_comp_copier_init(void);
void sys_comp_module_cadence_interface_init(void);
void sys_comp_module_passthrough_interface_init(void);
#if CONFIG_COMP_LEGACY_INTERFACE
void sys_comp_volume_init(void);
#else
void sys_comp_module_volume_interface_init(void);
#endif
void sys_comp_module_gain_interface_init(void);
void sys_comp_mixin_init(void);
void sys_comp_aria_init(void);
void sys_comp_crossover_init(void);
void sys_comp_drc_init(void);
void sys_comp_multiband_drc_init(void);
void sys_comp_google_rtc_audio_processing_init(void);
void sys_comp_igo_nr_init(void);
void sys_comp_rtnr_init(void);
void sys_comp_up_down_mixer_init(void);
void sys_comp_tdfb_init(void);
void sys_comp_ghd_init(void);
void sys_comp_module_dts_interface_init(void);
void sys_comp_module_waves_interface_init(void);

/* Zephyr redefines log_message() and mtrace_printf() which leaves
 * totally empty the .static_log_entries ELF sections for the
 * sof-logger. This makes smex fail. Define at least one such section to
 * fix the build when sof-logger is not used.
 */
static inline const void *smex_placeholder_f(void)
{
	_DECLARE_LOG_ENTRY(LOG_LEVEL_DEBUG,
			   "placeholder so .static_log.X are not all empty",
			   _TRACE_INV_CLASS, 0);

	return &log_entry;
}

/* Need to actually use the function and export something otherwise the
 * compiler optimizes everything away.
 */
const void *_smex_placeholder;

int task_main_start(struct sof *sof)
{
	_smex_placeholder = smex_placeholder_f();

	int ret;

	/* init default audio components */
	sys_comp_init(sof);

	/* init self-registered modules */
	sys_module_init();

	/* host is mandatory */
	sys_comp_host_init();

	if (IS_ENABLED(CONFIG_COMP_VOLUME)) {
#if CONFIG_COMP_LEGACY_INTERFACE
		sys_comp_volume_init();
#else
		sys_comp_module_volume_interface_init();
#endif

		if (IS_ENABLED(CONFIG_IPC_MAJOR_4))
			sys_comp_module_gain_interface_init();
	}

	if (IS_ENABLED(CONFIG_COMP_MIXER)) {
		sys_comp_mixer_init();

		if (IS_ENABLED(CONFIG_IPC_MAJOR_4))
			sys_comp_mixin_init();
	}

	if (IS_ENABLED(CONFIG_COMP_DAI))
		sys_comp_dai_init();

	if (IS_ENABLED(CONFIG_COMP_SRC))
		sys_comp_src_init();

	if (IS_ENABLED(CONFIG_COMP_SEL))
		sys_comp_selector_init();

	if (IS_ENABLED(CONFIG_COMP_SWITCH))
		sys_comp_switch_init();

	if (IS_ENABLED(CONFIG_COMP_TONE))
		sys_comp_tone_init();

	if (IS_ENABLED(CONFIG_COMP_FIR))
		sys_comp_eq_fir_init();

	if (IS_ENABLED(CONFIG_COMP_IIR))
		sys_comp_eq_iir_init();

	if (IS_ENABLED(CONFIG_SAMPLE_KEYPHRASE))
		sys_comp_keyword_init();

	if (IS_ENABLED(CONFIG_COMP_KPB))
		sys_comp_kpb_init();

	if (IS_ENABLED(CONFIG_SAMPLE_SMART_AMP) ||
	    IS_ENABLED(CONFIG_MAXIM_DSM))
		sys_comp_smart_amp_init();

	if (IS_ENABLED(CONFIG_COMP_ASRC))
		sys_comp_asrc_init();

	if (IS_ENABLED(CONFIG_COMP_DCBLOCK))
		sys_comp_dcblock_init();

	if (IS_ENABLED(CONFIG_COMP_MUX))
		sys_comp_mux_init();

	if (IS_ENABLED(CONFIG_COMP_BASEFW_IPC4))
		sys_comp_basefw_init();

	if (IS_ENABLED(CONFIG_COMP_COPIER))
		sys_comp_copier_init();

	if (IS_ENABLED(CONFIG_CADENCE_CODEC))
		sys_comp_module_cadence_interface_init();

	if (IS_ENABLED(CONFIG_PASSTHROUGH_CODEC))
		sys_comp_module_passthrough_interface_init();

	if (IS_ENABLED(CONFIG_COMP_ARIA))
		sys_comp_aria_init();

	if (IS_ENABLED(CONFIG_COMP_CROSSOVER))
		sys_comp_crossover_init();

	if (IS_ENABLED(CONFIG_COMP_DRC))
		sys_comp_drc_init();

	if (IS_ENABLED(CONFIG_COMP_MULTIBAND_DRC))
		sys_comp_multiband_drc_init();

	if (IS_ENABLED(CONFIG_COMP_GOOGLE_RTC_AUDIO_PROCESSING))
		sys_comp_google_rtc_audio_processing_init();

	if (IS_ENABLED(CONFIG_COMP_IGO_NR))
		sys_comp_igo_nr_init();

	if (IS_ENABLED(CONFIG_COMP_RTNR))
		sys_comp_rtnr_init();

	if (IS_ENABLED(CONFIG_COMP_UP_DOWN_MIXER))
		sys_comp_up_down_mixer_init();

	if (IS_ENABLED(CONFIG_COMP_TDFB))
		sys_comp_tdfb_init();

	if (IS_ENABLED(CONFIG_COMP_GOOGLE_HOTWORD_DETECT))
		sys_comp_ghd_init();

	if (IS_ENABLED(CONFIG_DTS_CODEC))
		sys_comp_module_dts_interface_init();

	if (IS_ENABLED(CONFIG_WAVES_CODEC))
		sys_comp_module_waves_interface_init();

	/* init pipeline position offsets */
	pipeline_posn_init(sof);

#if defined(CONFIG_IMX)
#define SOF_IPC_QUEUED_DOMAIN SOF_SCHEDULE_LL_DMA
#else
#define SOF_IPC_QUEUED_DOMAIN SOF_SCHEDULE_LL_TIMER
#endif

	/*
	 * called from primary_core_init(), track state here
	 * (only called from single core, no RMW lock)
	 */
	__ASSERT_NO_MSG(cpu_get_id() == PLATFORM_PRIMARY_CORE_ID);

	/* Temporary fix for issue #4356 */
	(void)notifier_register(NULL, scheduler_get_data(SOF_IPC_QUEUED_DOMAIN),
				NOTIFIER_ID_LL_POST_RUN,
				ipc_send_queued_callback, 0);
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_SOFT_OFF, PM_ALL_SUBSTATES);

	/* let host know DSP boot is complete */
	ret = platform_boot_complete(0);

	return ret;
}

/*
 * Timestamps.
 *
 * TODO: move to generic code in SOF, currently platform code.
 */

/* get timestamp for host stream DMA position */
void platform_host_timestamp(struct comp_dev *host,
			     struct sof_ipc_stream_posn *posn)
{
	int err;

	/* get host position */
	err = comp_position(host, posn);
	if (err == 0)
		posn->flags |= SOF_TIME_HOST_VALID;
}

/* get timestamp for DAI stream DMA position */
void platform_dai_timestamp(struct comp_dev *dai,
			    struct sof_ipc_stream_posn *posn)
{
	int err;

	/* get DAI position */
	err = comp_position(dai, posn);
	if (err == 0)
		posn->flags |= SOF_TIME_DAI_VALID;

	/* get SSP wallclock - DAI sets this to stream start value */
	posn->wallclock = sof_cycle_get_64() - posn->wallclock;
	posn->wallclock_hz = clock_get_freq(PLATFORM_DEFAULT_CLOCK);
	posn->flags |= SOF_TIME_WALL_VALID;
}

/* get current wallclock for componnent */
void platform_dai_wallclock(struct comp_dev *dai, uint64_t *wallclock)
{
	*wallclock = sof_cycle_get_64();
}

/*
 * Multicore
 *
 * Mostly empty today waiting pending Zephyr CAVS SMP integration.
 */
#if CONFIG_MULTICORE && CONFIG_SMP

static struct idc idc[CONFIG_MP_NUM_CPUS];
static struct idc *p_idc[CONFIG_MP_NUM_CPUS];

struct idc **idc_get(void)
{
	int cpu = cpu_get_id();

	p_idc[cpu] = idc + cpu;

	return p_idc + cpu;
}
#endif

