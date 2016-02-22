/*
 * Copyright (c) 2015 Ken Bannister. All rights reserved.
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

int gnrc_coap_start_server(gnrc_coap_server_t *server)
{
    /* check if server is already started */
    if (server->netreg.pid == gnrc_coap_pid_get()) {
        DEBUG("coap: server already started on port %" PRIu32 "\n",
               server->netreg.demux_ctx);
        return -EINVAL;
    }

    if (server->netreg.demux_ctx > UINT16_MAX) {
        DEBUG("coap: invalid port: %" PRIu32 "\n", server->netreg.demux_ctx);
        return -EINVAL;
    }
        
    server->netreg.pid = gnrc_coap_pid_get();
    gnrc_netreg_register(GNRC_NETTYPE_UDP, &server->netreg);
    return 0;
}
