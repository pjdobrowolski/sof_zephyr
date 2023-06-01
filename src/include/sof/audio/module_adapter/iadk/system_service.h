/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2022 Intel Corporation. All rights reserved.
 */
/*! \file system_service.h */

#ifndef _ADSP_SYSTEM_SERVICE_H_
#define _ADSP_SYSTEM_SERVICE_H_

#include "logger.h"
#include "adsp_stddef.h"
#include "adsp_error_code.h"
#include "native_system_service.h"
#include <stdint.h>


#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextern-c-compat"
#endif //__clang__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct native_system_service_api adsp_system_service;

#ifdef __cplusplus

namespace intel_adsp
{
/*! \brief Alias type of adsp_system_service which can be used in C++.
 */
struct SystemService : public adsp_system_service {};
}
#endif

#ifdef __cplusplus
}
#endif

#ifdef __clang__
#pragma clang diagnostic pop // ignored "-Wextern-c-compat"
#endif //__clang__

#endif /* _ADSP_SYSTEM_SERVICE_H_ */
