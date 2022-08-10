// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2022 Intel Corporation. All rights reserved.
//
// Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
//         Keyon Jie <yang.jie@linux.intel.com>
//         Rander Wang <rander.wang@intel.com>
//         Janusz Jankowski <janusz.jankowski@linux.intel.com>

#include <sof/common.h>
#include <sof/drivers/dw-dma.h>
#include <sof/drivers/hda-dma.h>
#include <sof/drivers/interrupt.h>
#include <sof/lib/dma.h>
#include <sof/lib/memory.h>
#include <sof/sof.h>
#include <sof/spinlock.h>
#ifdef __ZEPHYR__
#include <zephyr/device.h>
#endif

#define DMAC0_CLASS 6
#define DMAC1_CLASS 7
#define DMAC_HOST_IN_CHANNELS_COUNT 10
#define DMAC_HOST_OUT_CHANNELS_COUNT 9
#define DMAC_LINK_IN_CHANNELS_COUNT 10
#define DMAC_LINK_OUT_CHANNELS_COUNT 9

#if CONFIG_ZEPHYR_NATIVE_DRIVERS
static const char gpdma_dmac0_zephyr_id[] = "DMA_0";
static const char gpdma_dmac1_zephyr_id[] = "DMA_1";
static const char hda_host_in_zephyr_id[] = "HDA_HOST_IN";
static const char hda_host_out_zephyr_id[] = "HDA_HOST_OUT";
static const char hda_link_in_zephyr_id[] = "HDA_LINK_IN";
static const char hda_link_out_zephyr_id[] = "HDA_LINK_OUT";
#endif

const struct dw_drv_plat_data dmac0 = {
	.chan[0] = {
		.class	= DMAC0_CLASS,
		.weight = 0,
	},
	.chan[1] = {
		.class	= DMAC0_CLASS,
		.weight = 0,
	},
	.chan[2] = {
		.class	= DMAC0_CLASS,
		.weight = 0,
	},
	.chan[3] = {
		.class	= DMAC0_CLASS,
		.weight = 0,
	},
	.chan[4] = {
		.class	= DMAC0_CLASS,
		.weight = 0,
	},
	.chan[5] = {
		.class	= DMAC0_CLASS,
		.weight = 0,
	},
	.chan[6] = {
		.class	= DMAC0_CLASS,
		.weight = 0,
	},
	.chan[7] = {
		.class	= DMAC0_CLASS,
		.weight = 0,
	},
};

const struct dw_drv_plat_data dmac1 = {
	.chan[0] = {
		.class	= DMAC1_CLASS,
		.weight = 0,
	},
	.chan[1] = {
		.class	= DMAC1_CLASS,
		.weight = 0,
	},
	.chan[2] = {
		.class	= DMAC1_CLASS,
		.weight = 0,
	},
	.chan[3] = {
		.class	= DMAC1_CLASS,
		.weight = 0,
	},
	.chan[4] = {
		.class	= DMAC1_CLASS,
		.weight = 0,
	},
	.chan[5] = {
		.class	= DMAC1_CLASS,
		.weight = 0,
	},
	.chan[6] = {
		.class	= DMAC1_CLASS,
		.weight = 0,
	},
	.chan[7] = {
		.class	= DMAC1_CLASS,
		.weight = 0,
	},
};

SHARED_DATA struct dma dma[PLATFORM_NUM_DMACS] = {
{	/* Low Power GP DMAC 0 */
	.plat_data = {
		.id		= DMA_GP_LP_DMAC0,
		.dir		= DMA_DIR_MEM_TO_MEM | DMA_DIR_MEM_TO_DEV |
				  DMA_DIR_DEV_TO_MEM | DMA_DIR_DEV_TO_DEV,
		.caps		= DMA_CAP_GP_LP,
		.devs		= DMA_DEV_SSP | DMA_DEV_DMIC |
				  DMA_DEV_ALH,
		.base		= LP_GP_DMA_BASE(0),
		.channels	= 8,
		.irq		= IRQ_EXT_LP_GPDMA0_LVL5(0),
		.irq_name	= irq_name_level5,
		.drv_plat_data	= &dmac0,
#if CONFIG_ZEPHYR_NATIVE_DRIVERS
		.zephyr_id = gpdma_dmac0_zephyr_id
#endif
	},
	.ops		= &dw_dma_ops,
},
{	/* Low Power GP DMAC 1 */
	.plat_data = {
		.id		= DMA_GP_LP_DMAC1,
		.dir		= DMA_DIR_MEM_TO_MEM | DMA_DIR_MEM_TO_DEV |
				  DMA_DIR_DEV_TO_MEM | DMA_DIR_DEV_TO_DEV,
		.caps		= DMA_CAP_GP_LP,
		.devs		= DMA_DEV_SSP | DMA_DEV_DMIC |
				  DMA_DEV_ALH,
		.base		= LP_GP_DMA_BASE(1),
		.channels	= 8,
		.irq		= IRQ_EXT_LP_GPDMA1_LVL5(0),
		.irq_name	= irq_name_level5,
		.drv_plat_data	= &dmac1,
#if CONFIG_ZEPHYR_NATIVE_DRIVERS
		.zephyr_id = gpdma_dmac1_zephyr_id
#endif
	},
	.ops		= &dw_dma_ops,
},
{	/* Host In DMAC */
	.plat_data = {
		.id		= DMA_HOST_IN_DMAC,
		.dir		= DMA_DIR_LMEM_TO_HMEM,
		.caps		= DMA_CAP_HDA,
		.devs		= DMA_DEV_HOST,
		.base		= GTW_HOST_IN_STREAM_BASE(0),
		.channels	= DMAC_HOST_IN_CHANNELS_COUNT,
		.chan_size	= GTW_HOST_IN_STREAM_SIZE,
#if CONFIG_ZEPHYR_NATIVE_DRIVERS
		.zephyr_id = hda_host_in_zephyr_id
#endif
	},
	.ops		= &hda_host_dma_ops,
},
{	/* Host out DMAC */
	.plat_data = {
		.id		= DMA_HOST_OUT_DMAC,
		.dir		= DMA_DIR_HMEM_TO_LMEM,
		.caps		= DMA_CAP_HDA,
		.devs		= DMA_DEV_HOST,
		.base		= GTW_HOST_OUT_STREAM_BASE(0),
		.channels	= DMAC_HOST_OUT_CHANNELS_COUNT,
		.chan_size	= GTW_HOST_OUT_STREAM_SIZE,
#if CONFIG_ZEPHYR_NATIVE_DRIVERS
		.zephyr_id = hda_host_out_zephyr_id
#endif
	},
	.ops		= &hda_host_dma_ops,
},
{	/* Link In DMAC */
	.plat_data = {
		.id		= DMA_LINK_IN_DMAC,
		.dir		= DMA_DIR_DEV_TO_MEM,
		.caps		= DMA_CAP_HDA,
		.devs		= DMA_DEV_HDA,
		.base		= GTW_LINK_IN_STREAM_BASE(0),
		.channels	= DMAC_LINK_IN_CHANNELS_COUNT,
		.chan_size	= GTW_LINK_IN_STREAM_SIZE,
#if CONFIG_ZEPHYR_NATIVE_DRIVERS
		.zephyr_id = hda_link_in_zephyr_id
#endif
	},
	.ops		= &hda_link_dma_ops,
},
{	/* Link out DMAC */
	.plat_data = {
		.id		= DMA_LINK_OUT_DMAC,
		.dir		= DMA_DIR_MEM_TO_DEV,
		.caps		= DMA_CAP_HDA,
		.devs		= DMA_DEV_HDA,
		.base		= GTW_LINK_OUT_STREAM_BASE(0),
		.channels	= DMAC_LINK_OUT_CHANNELS_COUNT,
		.chan_size	= GTW_LINK_OUT_STREAM_SIZE,
#if CONFIG_ZEPHYR_NATIVE_DRIVERS
		.zephyr_id = hda_link_out_zephyr_id
#endif
	},
	.ops		= &hda_link_dma_ops,
},};

const struct dma_info lib_dma = {
	.dma_array = cache_to_uncache_init((struct dma *)dma),
	.num_dmas = ARRAY_SIZE(dma)
};

/* Initialize all platform DMAC's */
int dmac_init(struct sof *sof)
{
#if CONFIG_ZEPHYR_NATIVE_DRIVERS
	const struct device *z_dev = NULL;
#endif
	int i;
	/* no probing before first use */

	/* TODO: dynamic init based on platform settings */

	sof->dma_info = &lib_dma;

	/* early lock initialization for ref counting */
	for (i = 0; i < sof->dma_info->num_dmas; i++) {
		k_spinlock_init(&sof->dma_info->dma_array[i].lock);
#if CONFIG_ZEPHYR_NATIVE_DRIVERS
		z_dev = device_get_binding(sof->dma_info->dma_array[i].plat_data.zephyr_id);
		sof->dma_info->dma_array[i].z_dev = z_dev;
#endif
	}
	return 0;
}
