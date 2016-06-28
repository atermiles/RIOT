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
 * @brief       Request message handler for GNRC CoAP
 * 
 * Manages responses and listener callbacks.
 *
 * @author      Ken Bannister
 * @}
 */
 
#include <errno.h>
#include "net/gnrc/coap.h"
#include "gnrc_coap_internal.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

int gnrc_coap_start_server(gnrc_coap_server_t *server, uint16_t port)
{
    if (gnrc_coap_register_listener(&server->listener, port) < 0)
        return -EINVAL;

    return 0;
}
