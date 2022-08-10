// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2022 Intel Corporation. All rights reserved.
//
// Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
//         Keyon Jie <yang.jie@linux.intel.com>
//         Rander Wang <rander.wang@intel.com>
//         Janusz Jankowski <janusz.jankowski@linux.intel.com>

#include <ace/version.h>
#if CONFIG_CAVS_LPS
#include <ace/lps_wait.h>
#endif /* CONFIG_CAVS_LPS */
#include <ace/mem_window.h>
#include <sof/common.h>
#include <sof/compiler_info.h>
#include <sof/debug/debug.h>
#include <sof/drivers/dw-dma.h>
#include <sof/drivers/idc.h>
#include <sof/drivers/interrupt.h>
#include <sof/ipc/common.h>
#include <sof/drivers/timer.h>
#include <sof/fw-ready-metadata.h>
#include <sof/lib/agent.h>
#include <sof/lib/cache.h>
#include <sof/lib/clk.h>
#include <sof/lib/cpu.h>
#include <sof/lib/dai.h>
#include <sof/lib/dma.h>
#include <sof/lib/io.h>
#include <sof/lib/mailbox.h>
#include <sof/lib/memory.h>
#include <sof/lib/mm_heap.h>
#include <sof/lib/notifier.h>
#include <sof/lib/pm_runtime.h>
#include <sof/lib/wait.h>
#include <sof/platform.h>
#include <sof/schedule/edf_schedule.h>
#include <sof/schedule/ll_schedule.h>
#include <sof/schedule/ll_schedule_domain.h>
#include <sof/trace/dma-trace.h>
#include <sof/trace/trace.h>
#include <ipc/header.h>
#include <ipc/info.h>
#include <kernel/abi.h>
#include <kernel/ext_manifest.h>

#include <sof_versions.h>
#include <errno.h>
#include <stdint.h>

static const struct sof_ipc_fw_ready ready
	__section(".fw_ready") = {
	.hdr = {
		.cmd = SOF_IPC_FW_READY,
		.size = sizeof(struct sof_ipc_fw_ready),
	},
	.version = {
		.hdr.size = sizeof(struct sof_ipc_fw_version),
		.micro = SOF_MICRO,
		.minor = SOF_MINOR,
		.major = SOF_MAJOR,
/* opt-in; reproducible build by default */
#if BLD_COUNTERS
		.build = SOF_BUILD, /* See version-build-counter.cmake */
		.date = __DATE__,
		.time = __TIME__,
#else /* BLD_COUNTERS */
		.build = -1,
		.date = "dtermin.\0",
		.time = "fwready.\0",
#endif /* BLD_COUNTERS */
		.tag = SOF_TAG,
		.abi_version = SOF_ABI_VERSION,
		.src_hash = SOF_SRC_HASH,
	},
#if CONFIG_ACE_IMR_D3_PERSISTENT
	.flags = DEBUG_SET_FW_READY_FLAGS | SOF_IPC_INFO_D3_PERSISTENT,
#else /* CONFIG_ACE_IMR_D3_PERSISTENT */
	.flags = DEBUG_SET_FW_READY_FLAGS,
#endif /* CONFIG_ACE_IMR_D3_PERSISTENT */
};

#if CONFIG_MEM_WND

#define SRAM_WINDOW_HOST_OFFSET(x) (0x80000 + x * 0x20000)
#define NUM_WINDOWS 7

const struct ext_man_windows xsram_window
		__aligned(EXT_MAN_ALIGN) __section(".fw_metadata") __unused = {
	.hdr = {
		.type = EXT_MAN_ELEM_WINDOW,
		.elem_size = ALIGN_UP_COMPILE(sizeof(struct ext_man_windows), EXT_MAN_ALIGN),
	},
	.window = {
		.ext_hdr	= {
			.hdr.cmd = SOF_IPC_FW_READY,
			.hdr.size = sizeof(struct sof_ipc_window),
			.type	= SOF_IPC_EXT_WINDOW,
		},
		.num_windows	= NUM_WINDOWS,
		.window	= {
			{
				.type	= SOF_IPC_REGION_REGS,
				.id	= 0,	/* map to host window 0 */
				.flags	= 0, // TODO: set later
				.size	= MAILBOX_SW_REG_SIZE,
				.offset	= 0,
			},
			{
				.type	= SOF_IPC_REGION_UPBOX,
				.id	= 0,	/* map to host window 0 */
				.flags	= 0, // TODO: set later
				.size	= MAILBOX_DSPBOX_SIZE,
				.offset	= MAILBOX_SW_REG_SIZE,
			},
			{
				.type	= SOF_IPC_REGION_DOWNBOX,
				.id	= 1,	/* map to host window 1 */
				.flags	= 0, // TODO: set later
				.size	= MAILBOX_HOSTBOX_SIZE,
				.offset	= 0,
			},
			{
				.type	= SOF_IPC_REGION_DEBUG,
				.id	= 2,	/* map to host window 2 */
				.flags	= 0, // TODO: set later
				.size	= MAILBOX_DEBUG_SIZE,
				.offset	= 0,
			},
			{
				.type	= SOF_IPC_REGION_EXCEPTION,
				.id	= 2,	/* map to host window 2 */
				.flags	= 0, // TODO: set later
				.size	= MAILBOX_EXCEPTION_SIZE,
				.offset	= MAILBOX_EXCEPTION_OFFSET,
			},
			{
				.type	= SOF_IPC_REGION_STREAM,
				.id	= 2,	/* map to host window 2 */
				.flags	= 0, // TODO: set later
				.size	= MAILBOX_STREAM_SIZE,
				.offset	= MAILBOX_STREAM_OFFSET,
			},
			{
				.type	= SOF_IPC_REGION_TRACE,
				.id	= 3,	/* map to host window 3 */
				.flags	= 0, // TODO: set later
				.size	= MAILBOX_TRACE_SIZE,
				.offset	= 0,
			},
		},
	},
};

#endif /* CONFIG_MEM_WND */

#if CONFIG_CAVS_LPRO_ONLY
#define CAVS_DEFAULT_RO		SHIM_CLKCTL_RLROSCC
#define CAVS_DEFAULT_RO_FOR_MEM	SHIM_CLKCTL_OCS_LP_RING
#else /* CONFIG_CAVS_LPRO_ONLY */
#define CAVS_DEFAULT_RO		SHIM_CLKCTL_RHROSCC
#define CAVS_DEFAULT_RO_FOR_MEM	SHIM_CLKCTL_OCS_HP_RING
#endif /* CONFIG_CAVS_LPRO_ONLY */

#if CONFIG_DW_GPIO

#include <sof/drivers/gpio.h>

const struct gpio_pin_config gpio_data[] = {
	{	/* GPIO0 */
		.mux_id = 1,
		.mux_config = {.bit = 0, .mask = 3, .fn = 1},
	}, {	/* GPIO1 */
		.mux_id = 1,
		.mux_config = {.bit = 2, .mask = 3, .fn = 1},
	}, {	/* GPIO2 */
		.mux_id = 1,
		.mux_config = {.bit = 4, .mask = 3, .fn = 1},
	}, {	/* GPIO3 */
		.mux_id = 1,
		.mux_config = {.bit = 6, .mask = 3, .fn = 1},
	}, {	/* GPIO4 */
		.mux_id = 1,
		.mux_config = {.bit = 8, .mask = 3, .fn = 1},
	}, {	/* GPIO5 */
		.mux_id = 1,
		.mux_config = {.bit = 10, .mask = 3, .fn = 1},
	}, {	/* GPIO6 */
		.mux_id = 1,
		.mux_config = {.bit = 12, .mask = 3, .fn = 1},
	}, {	/* GPIO7 */
		.mux_id = 1,
		.mux_config = {.bit = 14, .mask = 3, .fn = 1},
	}, {	/* GPIO8 */
		.mux_id = 1,
		.mux_config = {.bit = 16, .mask = 1, .fn = 1},
	}, {	/* GPIO9 */
		.mux_id = 0,
		.mux_config = {.bit = 11, .mask = 1, .fn = 1},
	}, {	/* GPIO10 */
		.mux_id = 0,
		.mux_config = {.bit = 11, .mask = 1, .fn = 1},
	}, {	/* GPIO11 */
		.mux_id = 0,
		.mux_config = {.bit = 11, .mask = 1, .fn = 1},
	}, {	/* GPIO12 */
		.mux_id = 0,
		.mux_config = {.bit = 11, .mask = 1, .fn = 1},
	}, {	/* GPIO13 */
		.mux_id = 0,
		.mux_config = {.bit = 0, .mask = 1, .fn = 1},
	}, {	/* GPIO14 */
		.mux_id = 0,
		.mux_config = {.bit = 1, .mask = 1, .fn = 1},
	}, {	/* GPIO15 */
		.mux_id = 0,
		.mux_config = {.bit = 9, .mask = 1, .fn = 1},
	}, {	/* GPIO16 */
		.mux_id = 0,
		.mux_config = {.bit = 9, .mask = 1, .fn = 1},
	}, {	/* GPIO17 */
		.mux_id = 0,
		.mux_config = {.bit = 9, .mask = 1, .fn = 1},
	}, {	/* GPIO18 */
		.mux_id = 0,
		.mux_config = {.bit = 9, .mask = 1, .fn = 1},
	}, {	/* GPIO19 */
		.mux_id = 0,
		.mux_config = {.bit = 10, .mask = 1, .fn = 1},
	}, {	/* GPIO20 */
		.mux_id = 0,
		.mux_config = {.bit = 10, .mask = 1, .fn = 1},
	}, {	/* GPIO21 */
		.mux_id = 0,
		.mux_config = {.bit = 10, .mask = 1, .fn = 1},
	}, {	/* GPIO22 */
		.mux_id = 0,
		.mux_config = {.bit = 10, .mask = 1, .fn = 1},
	}, {	/* GPIO23 */
		.mux_id = 0,
		.mux_config = {.bit = 16, .mask = 1, .fn = 1},
	}, {	/* GPIO24 */
		.mux_id = 0,
		.mux_config = {.bit = 16, .mask = 1, .fn = 1},
	}, {	/* GPIO25 */
		.mux_id = 0,
		.mux_config = {.bit = 26, .mask = 1, .fn = 1},
	},
};

const int n_gpios = ARRAY_SIZE(gpio_data);

#if CONFIG_INTEL_IOMUX

#include <sof/drivers/iomux.h>

struct iomux iomux_data[] = {
	{.base = EXT_CTRL_BASE + 0x30,},
	{.base = EXT_CTRL_BASE + 0x34,},
	{.base = EXT_CTRL_BASE + 0x38,},
};

const int n_iomux = ARRAY_SIZE(iomux_data);

#endif /* CONFIG_INTEL_IOMUX */

#endif /* CONFIG_DW_GPIO */

#ifndef __ZEPHYR__
static SHARED_DATA struct timer timer = {
	.id = TIMER3, /* external timer */
	.irq = IRQ_EXT_TSTAMP0_LVL2,
	.irq_name = irq_name_level2,
};

static SHARED_DATA struct timer arch_timers[CONFIG_CORE_COUNT];
#endif

#if CONFIG_DW_SPI

#include <sof/drivers/spi.h>

static struct spi_platform_data spi = {
	.base		= DW_SPI_SLAVE_BASE,
	.type		= SOF_SPI_INTEL_SLAVE,
	.fifo[SPI_DIR_RX] = {
		.handshake	= DMA_HANDSHAKE_SSI_RX,
	},
	.fifo[SPI_DIR_TX] = {
		.handshake	= DMA_HANDSHAKE_SSI_TX,
	}
};

int platform_boot_complete(uint32_t boot_message)
{
	return spi_push(spi_get(SOF_SPI_INTEL_SLAVE), &ready, sizeof(ready));
}

#else /* CONFIG_DW_SPI */

int platform_boot_complete(uint32_t boot_message)
{
	struct ipc_cmd_hdr header;

	mailbox_dspbox_write(0, &ready, sizeof(ready));

	/* get any IPC specific boot message and optional data */
	ipc_boot_complete_msg(&header, SRAM_WINDOW_HOST_OFFSET(0) >> 12);

	/* tell host we are ready */
	ipc_write(DfIPCxIDDy_REG, header.ext);
	ipc_write(DfIPCxIDR_REG, IPC_DIPCIDR_BUSY | header.pri);

	return 0;
}

#endif /* CONFIG_DW_SPI */

/* init HW  */
static void platform_init_hw(void)
{
	io_reg_write(DSP_INIT_GENO,
		GENO_MDIVOSEL | GENO_DIOPTOSEL);

	io_reg_write(DSP_INIT_IOPO,
		IOPO_DMIC_FLAG | IOPO_I2S_FLAG);

	io_reg_write(DSP_INIT_LPGPDMA(0),
		LPGPDMA_CHOSEL_FLAG | LPGPDMA_CTLOSEL_FLAG);
	io_reg_write(DSP_INIT_LPGPDMA(1),
		LPGPDMA_CHOSEL_FLAG | LPGPDMA_CTLOSEL_FLAG);
}

/* Runs on the primary core only */
int platform_init(struct sof *sof)
{
#if CONFIG_DW_SPI
	struct spi *spi_dev;
#endif /* CONFIG_DW_SPI */
	int ret;
#ifndef __ZEPHYR__
	int i;

	sof->platform_timer = cache_to_uncache(&timer);
	sof->cpu_timers = (struct timer *)cache_to_uncache(&arch_timers);

	for (i = 0; i < CONFIG_CORE_COUNT; i++)
		sof->cpu_timers[i] = (struct timer) {
			.id = TIMER1, /* internal timer */
			.irq = IRQ_NUM_TIMER2,
		};
#endif

	/* Turn off memory for all unused cores */
	// TODO: to be enabled for MTL
#if !CONFIG_METEORLAKE
	for (i = 0; i < CONFIG_CORE_COUNT; i++)
		if (i != PLATFORM_PRIMARY_CORE_ID)
			pm_runtime_put(CORE_MEMORY_POW, i);
#endif /* !CONFIG_METEORLAKE */

	/* pm runtime already initialized, request the DSP to stay in D0
	 * until we are allowed to do full power gating (by the IPC req).
	 */
	pm_runtime_disable(PM_RUNTIME_DSP, 0);

	trace_point(TRACE_BOOT_PLATFORM_ENTRY);
	platform_init_hw();

	trace_point(TRACE_BOOT_PLATFORM_IRQ);
	platform_interrupt_init();

#if CONFIG_MEM_WND
	trace_point(TRACE_BOOT_PLATFORM_MBOX);
	platform_memory_windows_init(MEM_WND_INIT_CLEAR);
#endif /* CONFIG_MEM_WND */

#ifndef __ZEPHYR__
	/* init timers, clocks and schedulers */
	trace_point(TRACE_BOOT_PLATFORM_TIMER);
	platform_timer_start(sof->platform_timer);
#endif /* !__ZEPHYR__ */

	trace_point(TRACE_BOOT_PLATFORM_CLOCK);
	platform_clock_init(sof);

	trace_point(TRACE_BOOT_PLATFORM_SCHED);
	scheduler_init_edf();

	/* init low latency timer domain and scheduler */
	sof->platform_timer_domain =
		timer_domain_init(sof->platform_timer, PLATFORM_DEFAULT_CLOCK);
	scheduler_init_ll(sof->platform_timer_domain);

	/* init the system agent */
	trace_point(TRACE_BOOT_PLATFORM_AGENT);
	sa_init(sof, CONFIG_SYSTICK_PERIOD);

	/* Set CPU to max frequency for booting (single shim_write below) */
	trace_point(TRACE_BOOT_PLATFORM_CPU_FREQ);

	shim_write(SHIM_GPDMA_CLKCTL(0), SHIM_CLKCTL_LPGPDMA_SPA);
	shim_write(SHIM_GPDMA_CLKCTL(1), SHIM_CLKCTL_LPGPDMA_SPA);
	while (!(shim_read(SHIM_GPDMA_CLKCTL(0)) & SHIM_CLKCTL_LPGPDMA_CPA))
		idelay(16);

	/* init DMACs */
	trace_point(TRACE_BOOT_PLATFORM_DMA);
	ret = dmac_init(sof);
	if (ret < 0)
		return ret;

	/* init low latency single channel DW-DMA domain and scheduler */
	sof->platform_dma_domain =
		dma_single_chan_domain_init
			(&sof->dma_info->dma_array[PLATFORM_DW_DMA_INDEX],
			 PLATFORM_NUM_DW_DMACS,
			 PLATFORM_DEFAULT_CLOCK);
	scheduler_init_ll(sof->platform_dma_domain);

	/* initialize the host IPC mechanisms */
	trace_point(TRACE_BOOT_PLATFORM_IPC);
	ipc_init(sof);

	/* initialize IDC mechanism */
	trace_point(TRACE_BOOT_PLATFORM_IDC);
	ret = idc_init();
	if (ret < 0)
		return ret;

	/* init DAIs */
	trace_point(TRACE_BOOT_PLATFORM_DAI);
	ret = dai_init(sof);
	if (ret < 0)
		return ret;

#if CONFIG_DW_SPI
	/* initialize the SPI slave */
	trace_point(TRACE_BOOT_PLATFORM_SPI);
	spi_init();
	ret = spi_install(&spi, 1);
	if (ret < 0)
		return ret;

	spi_dev = spi_get(SOF_SPI_INTEL_SLAVE);
	if (!spi_dev)
		return -ENODEV;

	/* initialize the SPI-SLave module */
	ret = spi_probe(spi_dev);
	if (ret < 0)
		return ret;
#elif CONFIG_TRACE
	/* Initialize DMA for Trace*/
	trace_point(TRACE_BOOT_PLATFORM_DMA_TRACE);
	dma_trace_init_complete(sof->dmat);
#endif /* CONFIG_DW_SPI */

	/* show heap status */
	heap_trace_all(1);

	return 0;
}

#ifndef __ZEPHYR__
void platform_wait_for_interrupt(int level)
{
	platform_clock_on_waiti();

#ifdef CONFIG_MULTICORE
	int cpu_id = cpu_get_id();

	/* for secondary cores, if prepare_d0ix_core_mask flag is set for
	 * specific core, we should prepare for power down before going to wait
	 * - it is required by D0->D0ix flow.
	 */
	if (cpu_id != PLATFORM_PRIMARY_CORE_ID &&
	    platform_pm_runtime_prepare_d0ix_is_req(cpu_id))
		cpu_power_down_core(CPU_POWER_DOWN_MEMORY_ON);
#endif /* CONFIG_MULTICORE */

#if CONFIG_CAVS_LPS
	if (pm_runtime_is_active(PM_RUNTIME_DSP, PLATFORM_PRIMARY_CORE_ID) ||
	    cpu_get_id() != PLATFORM_PRIMARY_CORE_ID)
		arch_wait_for_interrupt(level);
	else
		lps_wait_for_interrupt(level);
#else /* CONFIG_CAVS_LPS */
	arch_wait_for_interrupt(level);
#endif /* CONFIG_CAVS_LPS */
}
#endif /* !__ZEPHYR__ */

#if CONFIG_ACE_IMR_D3_PERSISTENT
/* These structs and macros are from from the ROM code header
 * on cAVS platforms, please keep them immutable
 */

#define ADSP_IMR_MAGIC_VALUE		0x02468ACE
/* FIXME: IMR address should be taken from zephyr DTS */
#define IMR_L1_CACHE_ADDRESS		0xA1000000
#define IMR_LAYOUT_OFFSET               0x20000
#define IMR_LAYOUT_ADDRESS              (IMR_L1_CACHE_ADDRESS + IMR_LAYOUT_OFFSET)

struct imr_header {
	uint32_t adsp_imr_magic;
	uint32_t structure_version;
	uint32_t structure_size;
	uint32_t imr_state;
	uint32_t imr_size;
	void *imr_restore_vector;
	void *imr_auth_api_vector;
};

struct imr_state {
	struct imr_header header;
	uint8_t reserved[0x1000 - sizeof(struct imr_header)];
};

struct imr_layout {
	struct imr_state    imr_state;
};

/* cAVS ROM structs and macros end */

static void imr_layout_update(void *vector)
{
	struct imr_layout *imr_layout = (struct imr_layout *)(IMR_LAYOUT_ADDRESS);

	/* update the IMR layout and write it back to uncache memory
	 * for ROM code usage. The ROM code will read this from IMR
	 * at the subsequent run and decide (e.g. combine with checking
	 * if FW_PURGE IPC from host got) if it can use the previous
	 * IMR FW directly. So this here is only a host->FW->ROM one way
	 * configuration, no symmetric task need to done in any
	 * platform_resume() to clear the configuration.
	 */
	dcache_invalidate_region(imr_layout, sizeof(*imr_layout));
	imr_layout->imr_state.header.adsp_imr_magic = ADSP_IMR_MAGIC_VALUE;
	imr_layout->imr_state.header.imr_restore_vector = vector;
	dcache_writeback_region(imr_layout, sizeof(*imr_layout));
}
#endif /* CONFIG_ACE_IMR_D3_PERSISTENT */

int platform_context_save(struct sof *sof)
{
#if CONFIG_ACE_IMR_D3_PERSISTENT
	/*
	 * Both runtime PM and S2Idle suspend works on APL, while S3 ([deep])
	 * doesn't. Only support IMR restoring on cAVS 1.8 and onward at the
	 * moment.
	 * TODO: Root cause of why IMR restore doesn't work on APL during S3
	 * cycle.
	 */
	imr_layout_update((void *)IMR_BOOT_LDR_TEXT_ENTRY_BASE);
#endif /* CONFIG_ACE_IMR_D3_PERSISTENT */
	return 0;
}
