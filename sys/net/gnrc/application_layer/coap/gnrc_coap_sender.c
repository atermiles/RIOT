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
 * @brief       Message sender for GNRC CoAP
 * 
 * Manages confirmable messaging.
 *
 * @author      Ken Bannister
 * @}
*/
 
#include <errno.h>
#include "net/gnrc/coap.h"
#include "gnrc_coap_internal.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

/** @brief List of registered listeners for responses */
static gnrc_coap_listener_t *_listener_list = NULL;

/* 
 * gnrc_coap internal functions
 */

gnrc_coap_listener_t *gnrc_coap_listener_find(uint16_t port)
{
    gnrc_coap_listener_t *listener;
    
    LL_SEARCH_SCALAR(_listener_list, listener, netreg.demux_ctx, port);
    return listener;
}

/* 
 * gnrc_coap interface functions
 */

size_t gnrc_coap_send(gnrc_coap_sender_t *sender, ipv6_addr_t *addr, uint16_t port, 
                                                         gnrc_coap_transfer_t *xfer)
{
    uint8_t srcport[2], dstport[2];             /* Bytes for UDP header function */
    gnrc_pktsnip_t *payload, *coap, *udp, *ip;
    size_t pktlen;
    
    /* register listener if necessary */
    gnrc_coap_register_listener(&sender->listener);

    /* allocate payload */
    if (xfer->datalen > 0) {
        payload = gnrc_pktbuf_add(NULL, xfer->data, xfer->datalen, GNRC_NETTYPE_UNDEF);
        if (payload == NULL) {
            DEBUG("coap: unable to copy data to packet buffer\n");
            return 0;
        }
    } else {
        payload = NULL;
    }
   
    /* allocate CoAP header */
    coap = gnrc_coap_hdr_build(&sender->msg_meta, xfer, payload);
    if (coap == NULL) {
        DEBUG("coap: unable to allocate CoAP header\n");
        gnrc_pktbuf_release(payload);
        return 0;
    }

    /* allocate UDP header */
    srcport[0] = (uint8_t)sender->listener.netreg.demux_ctx;
    srcport[1] = sender->listener.netreg.demux_ctx >> 8;
    dstport[0] = (uint8_t)port;
    dstport[1] = port >> 8;
    
    udp = gnrc_udp_hdr_build(coap, srcport, 2, dstport, 2);
    if (udp == NULL) {
        DEBUG("coap: unable to allocate UDP header\n");
        gnrc_pktbuf_release(payload);
        return 0;
    }
    /* allocate IPv6 header */
    ip = gnrc_ipv6_hdr_build(udp, NULL, 0, (uint8_t *)addr, sizeof(*addr));
    if (ip == NULL) {
        DEBUG("coap: unable to allocate IPv6 header\n");
        gnrc_pktbuf_release(udp);
        return 0;
    }
    pktlen = gnrc_pkt_len(ip);          /* count now; snips deallocated after send*/
    
    /* send packet */
    if (!gnrc_netapi_dispatch_send(GNRC_NETTYPE_UDP, GNRC_NETREG_DEMUX_CTX_ALL, ip)) {
        DEBUG("coap: unable to locate UDP thread\n");
        gnrc_pktbuf_release(ip);
        return 0;
    }
    DEBUG("coap: msg sent, %u bytes\n", pktlen);
    return pktlen;
}

int gnrc_coap_register_listener(gnrc_coap_listener_t *listener)
{
    if (listener->netreg.pid == gnrc_coap_pid_get()) {
        DEBUG("coap: listener already registered for port %" PRIu32 "\n",
               listener->netreg.demux_ctx);
        return -EALREADY;
    }

    /* TODO Handle reduced port range for UDP compression; and in that context, */
    /*      handle when no ports available */
    /* Find an unused ephemeral port and add to listener list */
    listener->netreg.demux_ctx = GNRC_COAP_EPHEMERAL_PORT_MIN;
    
    while (listener->netreg.pid != gnrc_coap_pid_get()) {
        gnrc_netreg_entry_t *lookup;
        lookup = gnrc_netreg_lookup(GNRC_NETTYPE_UDP, listener->netreg.demux_ctx);
        if (lookup == NULL) {
            LL_APPEND(_listener_list, listener);
            listener->netreg.pid = gnrc_coap_pid_get();
            gnrc_netreg_register(GNRC_NETTYPE_UDP, &listener->netreg);
            DEBUG("coap: registered listener to port %" PRIu32 "\n",
                   listener->netreg.demux_ctx);
        } else {
            /* Try next port number */
            listener->netreg.demux_ctx++;
        }
    }
    return 0;
}
