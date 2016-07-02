/*
 * Copyright (c) 2015-2016 Ken Bannister. All rights reserved.
 *  
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     net_gnrc_coap
 * @{
 *
 * @file
 * @brief       Private implementation of GNRC CoAP
 * 
 * This header includes functions accessible only by the implementation of GNRC
 * COAP. See sys/include/net/gnrc/coap.h for the public interface.
 *
 * @author      Ken Bannister
 * @}
 */

#ifndef GNRC_COAP_INTERNAL_H_
#define GNRC_COAP_INTERNAL_H_

#include "net/gnrc/coap.h"

/** 
 * @brief Accessor for the thread for management of request/response messaging.
 */
kernel_pid_t gnrc_coap_pid_get(void);

/** 
 * @brief Finds a registered listener.
 * 
 * @param[in] port    Epermeral source port used by the listener
 * @return            The matching listener, or NULL if not found
 */
gnrc_coap_listener_t *gnrc_coap_listener_find(uint16_t port);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* GNRC_COAP_INTERNAL_H_ */
