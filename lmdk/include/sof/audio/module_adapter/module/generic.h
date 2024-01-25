/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright(c) 2020 - 2023 Intel Corporation. All rights reserved.
 *
 *
 * \file generic.h
 * \brief Generic Module API header file
 * \author Marcin Rajwa <marcin.rajwa@linux.intel.com>
 * \author Adrian Warecki <adrian.warecki@intel.com>
 *
 */

#ifndef __SOF_AUDIO_MODULE_GENERIC__
#define __SOF_AUDIO_MODULE_GENERIC__

//#include <sof/audio/component.h>
//#ifndef MODULE_PRIVAT
//#include <sof/ut.h>
//#include <sof/lib/memory.h>
//#include <sof/audio/dp_queue.h>
//#endif //MODULE_PRIVAT
//#include <sof/audio/sink_api.h>
//#include <sof/audio/source_api.h>
//
//#include "module_interface.h"
///*
// * helpers to determine processing type
// * Needed till all the modules use PROCESSING_MODE_SINK_SOURCE
// */
//#define IS_PROCESSING_MODE_AUDIO_STREAM(mod) ((mod)->proc_type == MODULE_PROCESS_TYPE_STREAM)
//#define IS_PROCESSING_MODE_RAW_DATA(mod) ((mod)->proc_type == MODULE_PROCESS_TYPE_RAW)
//#define IS_PROCESSING_MODE_SINK_SOURCE(mod) ((mod)->proc_type == MODULE_PROCESS_TYPE_SOURCE_SINK)
//
//#define MAX_BLOB_SIZE 8192
#define MODULE_MAX_SOURCES 8
#endif