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
#include <soc.h>
#include <version.h>
#include <zephyr/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>

#if !CONFIG_KERNEL_COHERENCE
#include <arch/xtensa/cache.h>
#endif

extern struct tr_ctx zephyr_tr;
LOG_MODULE_REGISTER(libzephyr_alloc, CONFIG_SOF_LOG_LEVEL);

/*
 * Memory - Create Zephyr HEAP for SOF.
 *
 * Currently functional but some items still WIP.
 */

#ifndef HEAP_RUNTIME_SIZE
#define HEAP_RUNTIME_SIZE	0
#endif

/* system size not declared on some platforms */
#ifndef HEAP_SYSTEM_SIZE
#define HEAP_SYSTEM_SIZE	0
#endif

/* The Zephyr heap */

#ifdef CONFIG_IMX
#define HEAPMEM_SIZE		(HEAP_SYSTEM_SIZE + HEAP_RUNTIME_SIZE + HEAP_BUFFER_SIZE)

/*
 * Include heapmem variable in .heap_mem section, otherwise the HEAPMEM_SIZE is
 * duplicated in two sections and the sdram0 region overflows.
 */
__section(".heap_mem") static uint8_t __aligned(64) heapmem[HEAPMEM_SIZE];

#elif CONFIG_ACE

#define HEAPMEM_SIZE 0x40000

/*
 * System heap definition for ACE is defined below.
 * It needs to be explicitly packed into dedicated section
 * to allow memory management driver to control unused
 * memory pages.
 */
__section(".heap_mem") static uint8_t __aligned(PLATFORM_DCACHE_ALIGN) heapmem[HEAPMEM_SIZE];

#if CONFIG_L3_HEAP

#define L3_MEM_LABEL imr1
#define L3_MEM_BASE_ADDR (DT_REG_ADDR(DT_NODELABEL(L3_MEM_LABEL)))
#define L3_MEM_SIZE (DT_REG_SIZE(DT_NODELABEL(L3_MEM_LABEL)))
#define L3_MEM_PAGE_SIZE (DT_PROP(DT_NODELABEL(L3_MEM_LABEL), block_size))

/*
 * This is a poor man's method to identify start of unused
 * IMR memory. This should be done dynamically based on FW metadata
 * and manifest, but since bootloader hardcodes the IMR layout the
 * code here follows this concept.
 */
#define L3_HEAP_START ROUND_UP(IMR_BOOT_LDR_BSS_BASE + IMR_BOOT_LDR_BSS_SIZE, \
		L3_MEM_PAGE_SIZE)

#endif
#else

extern char _end[], _heap_sentry[];
#define heapmem ((uint8_t *)ALIGN_UP((uintptr_t)_end, PLATFORM_DCACHE_ALIGN))
#define HEAPMEM_SIZE ((uint8_t *)_heap_sentry - heapmem)

#endif

static struct k_heap sof_heap;

#if CONFIG_L3_HEAP
static struct k_heap l3_heap;

/**
 * Returns the start of L3 memory heap.
 * @return Pointer to the L3 memory location which can be used for L3 heap.
 */
static uintptr_t get_l3_heap_start(void)
{
	/*
	 * TODO: parse the actual offset using:
	 * - HfIMRIA1 register
	 * - rom_ext_load_offset
	 * - main_fw_load_offset
	 * - main fw size in manifest
	 */

	return (uintptr_t)L3_HEAP_START;
}

/**
 * Returns the size of L3 memory heap.
 * @return Size of the L3 memory region which can be used for L3 heap.
 */
static size_t get_l3_heap_size(void)
{
	size_t size;
	 /*
	  * Calculate the IMR heap size using:
	  * - total IMR size
	  * - IMR base address
	  * - actual IMR heap start
	  */
	size = L3_MEM_SIZE -
	       (get_l3_heap_start() - L3_MEM_BASE_ADDR);

	return ROUND_DOWN(size, L3_MEM_PAGE_SIZE);
}

/**
 * Checks whether pointer is from L3 heap memory range.
 * @param ptr Pointer to memory being checked.
 * @return True if pointer falls into L3 heap region, false otherwise.
 */
static bool is_l3_heap_pointer(void *ptr)
{
	uintptr_t l3_heap_start = get_l3_heap_start();
	uintptr_t l3_heap_end = l3_heap_start + get_l3_heap_size();

	if (POINTER_TO_UINT(ptr) >= l3_heap_start &&
	    POINTER_TO_UINT(ptr) < l3_heap_end) {
		return true;
	}
	return false;
}

/**
 * Allocate buffer from the L3 memory heap.
 * Return pointer is always aligned to page size.
 * @param size Size of memory to be allocated
 * @return Pointer to allocated L3 memory block.
 */
void *l3_alloc(size_t size)
{
	k_spinlock_key_t key;
	void *ret;

	key = k_spin_lock(&l3_heap.lock);
	ret = sys_heap_aligned_alloc(&l3_heap.heap, L3_MEM_PAGE_SIZE, size);
	k_spin_unlock(&l3_heap.lock, key);
	return ret;
}

/**
 * Allocate buffer from the L3 memory heap.
 * @param mem SPointer to memory block to be freed.
 * @return Pointer to allocated L3 memory block.
 */
void l3_free(void *mem)
{
	/*
	 * first check to which heap the mem argument belongs to
	 */
	uintptr_t l3_heap_start = get_l3_heap_start();
	uintptr_t l3_heap_end = l3_heap_start + get_l3_heap_size();

	if (mem < l3_heap_start && mem >= l3_heap_end)
		return;

	k_spinlock_key_t key = k_spin_lock(&l3_heap.lock);

	sys_heap_free(&l3_heap.heap, mem);

	k_spin_unlock(&l3_heap.lock, key);
}
#endif

static int statics_init(const struct device *unused)
{
	ARG_UNUSED(unused);

	sys_heap_init(&sof_heap.heap, heapmem, HEAPMEM_SIZE);

#if CONFIG_L3_HEAP
	sys_heap_init(&l3_heap.heap, UINT_TO_POINTER(get_l3_heap_start()), get_l3_heap_size());
#endif

	return 0;
}

SYS_INIT(statics_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

static void *heap_alloc_aligned(struct k_heap *h, size_t min_align, size_t bytes)
{
	k_spinlock_key_t key;
	void *ret;

	key = k_spin_lock(&h->lock);
	ret = sys_heap_aligned_alloc(&h->heap, min_align, bytes);
	k_spin_unlock(&h->lock, key);

	return ret;
}

static void __sparse_cache *heap_alloc_aligned_cached(struct k_heap *h, size_t min_align,
						      size_t bytes)
{
	void __sparse_cache *ptr;

	/*
	 * Zephyr sys_heap stores metadata at start of each
	 * heap allocation. To ensure no allocated cached buffer
	 * overlaps the same cacheline with the metadata chunk,
	 * align both allocation start and size of allocation
	 * to cacheline. As cached and non-cached allocations are
	 * mixed, same rules need to be followed for both type of
	 * allocations.
	 */
#ifdef CONFIG_SOF_ZEPHYR_HEAP_CACHED
	min_align = MAX(PLATFORM_DCACHE_ALIGN, min_align);
	bytes = ALIGN_UP(bytes, min_align);
#endif

	ptr = (__sparse_force void __sparse_cache *)heap_alloc_aligned(h, min_align, bytes);

#ifdef CONFIG_SOF_ZEPHYR_HEAP_CACHED
	if (ptr)
		ptr = z_soc_cached_ptr((__sparse_force void *)ptr);
#endif

	return ptr;
}

static void heap_free(struct k_heap *h, void *mem)
{
	k_spinlock_key_t key = k_spin_lock(&h->lock);
#ifdef CONFIG_SOF_ZEPHYR_HEAP_CACHED
	void *mem_uncached;

	if (is_cached(mem)) {
		mem_uncached = z_soc_uncached_ptr((__sparse_force void __sparse_cache *)mem);
		z_xtensa_cache_flush_inv(mem, sys_heap_usable_size(&h->heap, mem_uncached));

		mem = mem_uncached;
	}
#endif

	sys_heap_free(&h->heap, mem);

	k_spin_unlock(&h->lock, key);
}

static inline bool zone_is_cached(enum mem_zone zone)
{
#ifdef CONFIG_SOF_ZEPHYR_HEAP_CACHED
	switch (zone) {
	case SOF_MEM_ZONE_SYS:
	case SOF_MEM_ZONE_SYS_RUNTIME:
	case SOF_MEM_ZONE_RUNTIME:
	case SOF_MEM_ZONE_BUFFER:
		return true;
	default:
		break;
	}
#endif

	return false;
}

void *rmalloc(enum mem_zone zone, uint32_t flags, uint32_t caps, size_t bytes)
{
	void *ptr;

#if CONFIG_L3_HEAP
	if (caps & SOF_MEM_CAPS_L3)
		return l3_alloc(bytes);
#endif

	if (zone_is_cached(zone) && !(flags & SOF_MEM_FLAG_COHERENT)) {
		ptr = (__sparse_force void *)heap_alloc_aligned_cached(&sof_heap, 0, bytes);
	} else {
		/*
		 * XTOS alloc implementation has used dcache alignment,
		 * so SOF application code is expecting this behaviour.
		 */
		ptr = heap_alloc_aligned(&sof_heap, PLATFORM_DCACHE_ALIGN, bytes);
	}

	if (!ptr && zone == SOF_MEM_ZONE_SYS)
		k_panic();

	return ptr;
}

/* Use SOF_MEM_ZONE_BUFFER at the moment */
void *rbrealloc_align(void *ptr, uint32_t flags, uint32_t caps, size_t bytes,
		      size_t old_bytes, uint32_t alignment)
{
	void *new_ptr;

	if (!ptr) {
		/* TODO: Use correct zone */
		return rballoc_align(flags, caps, bytes, alignment);
	}

	/* Original version returns NULL without freeing this memory */
	if (!bytes) {
		/* TODO: Should we call rfree(ptr); */
		tr_err(&zephyr_tr, "realloc failed for 0 bytes");
		return NULL;
	}

	new_ptr = rballoc_align(flags, caps, bytes, alignment);
	if (!new_ptr)
		return NULL;

	if (!(flags & SOF_MEM_FLAG_NO_COPY))
		memcpy(new_ptr, ptr, MIN(bytes, old_bytes));

	rfree(ptr);

	tr_info(&zephyr_tr, "rbealloc: new ptr %p", new_ptr);

	return new_ptr;
}

/**
 * Similar to rmalloc(), guarantees that returned block is zeroed.
 *
 * @note Do not use  for buffers (SOF_MEM_ZONE_BUFFER zone).
 *       rballoc(), rballoc_align() to allocate memory for buffers.
 */
void *rzalloc(enum mem_zone zone, uint32_t flags, uint32_t caps, size_t bytes)
{
	void *ptr = rmalloc(zone, flags, caps, bytes);

	memset(ptr, 0, bytes);

	return ptr;
}

/**
 * Allocates memory block from SOF_MEM_ZONE_BUFFER.
 * @param flags Flags, see SOF_MEM_FLAG_...
 * @param caps Capabilities, see SOF_MEM_CAPS_...
 * @param bytes Size in bytes.
 * @param alignment Alignment in bytes.
 * @return Pointer to the allocated memory or NULL if failed.
 */
void *rballoc_align(uint32_t flags, uint32_t caps, size_t bytes,
		    uint32_t alignment)
{
	if (flags & SOF_MEM_FLAG_COHERENT)
		return heap_alloc_aligned(&sof_heap, alignment, bytes);

	return (__sparse_force void *)heap_alloc_aligned_cached(&sof_heap, alignment, bytes);
}

/*
 * Free's memory allocated by above alloc calls.
 */
void rfree(void *ptr)
{
	if (!ptr)
		return;

#if CONFIG_L3_HEAP
	if (is_l3_heap_pointer(ptr)) {
		l3_free(ptr);
		return;
	}
#endif

	heap_free(&sof_heap, ptr);
}

/* debug only - only needed for linking */
void heap_trace_all(int force)
{
}
