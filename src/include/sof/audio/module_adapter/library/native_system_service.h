/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2023 Intel Corporation. All rights reserved.
 */
/*! \file native_system_service.h */
#include "logger.h"
#include "adsp_stddef.h"
#include "adsp_error_code.h"
#include <stdint.h>

struct native_system_service_api{
	void (*log_message)(AdspLogPriority log_priority, uint32_t log_entry,
		     AdspLogHandle const *log_handle, uint32_t param1,
		     uint32_t param2, uint32_t param3, uint32_t param4);

	AdspErrorCode (*safe_memcpy)(void *RESTRICT dst, size_t maxlen,
		      const void *RESTRICT src, size_t len);

	AdspErrorCode (*safe_memmove)(void *dst, size_t maxlen,
		       const void *src, size_t len);

	void (*vec_memset)(void *dst, int c, size_t len);

	AdspErrorCode (*notification_create)(NotificationParams *params,
						      uint8_t *notification_buffer,
						      uint32_t notification_buffer_size,
						      AdspNotificationHandle *handle);
	AdspErrorCode (*notification_send)(NotificationTarget notification_target,
							   AdspNotificationHandle message,
							   uint32_t actual_payload_size);

	AdspErrorCode (*get_interface)(AdspIfaceId id, SystemServiceIface **iface);
};
