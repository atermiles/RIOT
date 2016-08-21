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
 * Uses a single UDP port for communication to support RFC 6282 compression.
 *
 * @{
 *
 * @file
 * @brief       gcoap definition
 *
 * @author      Ken Bannister <kb2ma@runbox.com>
 */

#ifndef GNRC_COAP_H_
#define GNRC_COAP_H_

#include "net/gnrc.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/udp.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Size for module message queue */
#define GCOAP_MSG_QUEUE_SIZE (4)

/** @brief Server port; use RFC 7252 default if not defined */
#ifndef GCOAP_PORT
#define GCOAP_PORT  (5683)
#endif

/**
 * @brief   State for the gnrc coap module itself
 */
typedef struct {
    gnrc_netreg_entry_t netreg_port;   /**< Registration for IP port */
} gcoap_module_t;

/**
 * @brief   Initializes the gcoap thread and device.
 *
 * Must call once before first use.
 *
 * @return  PID of the gcoap thread on success.
 * @return  -EEXIST, if thread already has been created.
 * @return  -EINVAL, if the IP port already is in use.
 */
kernel_pid_t gcoap_init(void);

#ifdef __cplusplus
}
#endif

#endif /* GNRC_COAP_H_ */
/** @} */
