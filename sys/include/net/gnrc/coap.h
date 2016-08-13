/*
 * Copyright (c) 2015-2016 Ken Bannister. All rights reserved.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    net_gnrc_coap  CoAP
 * @ingroup     net_gnrc
 * @brief       GNRC implementation of CoAP protocol, RFC 7252
 *
 * ## Architecture ##
 * Requests and responses are exchanged via an asynchronous RIOT message processing 
 * thread. Depends on nanocoap for base level structs and functionality.
 *
 * @{
 *
 * @file
 * @brief       GNRC CoAP definition
 *
 * @author      Ken Bannister <kb2ma@runbox.com>
 */

#ifndef GNRC_COAP_H_
#define GNRC_COAP_H_

#include "net/gnrc.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Size for module message queue */
#define GNRC_COAP_MSG_QUEUE_SIZE (4)


/**
 * @brief   Initializes the gnrc_coap thread and device.
 *
 * Must call once before first use.
 *
 * @return  PID of the gnrc_coap thread on success.
 * @return  -EEXIST, if thread already has been created.
 */
kernel_pid_t gnrc_coap_init(void);

#ifdef __cplusplus
}
#endif

#endif /* GNRC_COAP_H_ */
/** @} */
