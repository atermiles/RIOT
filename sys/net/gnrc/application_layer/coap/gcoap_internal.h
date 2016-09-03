/*
 * Copyright (c) 2015-2016 Ken Bannister. All rights reserved.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     net_gcoap
 * @{
 *
 * @file
 * @brief       Private implementation of gcoap
 *
 * This header includes functions accessible only by the implementation of 
 * gcoap. See sys/include/net/gnrc/coap.h for the public interface.
 *
 * @author      Ken Bannister <kb2ma@runbox.com>
 * @}
 */

#ifndef GCOAP_INTERNAL_H_
#define GCOAP_INTERNAL_H_

#include "net/gnrc/coap.h"

/**
 * @brief Accessor for the thread for management of request/response messaging.
 */
kernel_pid_t gcoap_pid_get(void);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* GCOAP_INTERNAL_H_ */
