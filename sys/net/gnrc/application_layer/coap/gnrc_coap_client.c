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

/** @brief List of registered clients */
static gnrc_coap_client_t *_client_list = NULL;

/* 
 * gnrc_coap internal functions
 */

gnrc_coap_client_t *gnrc_coap_client_find(uint16_t port)
{
    gnrc_coap_client_t *client;
    
    LL_SEARCH_SCALAR(_client_list, client, netreg.demux_ctx, port);
    return client;
}

/* 
 * gnrc_coap interface functions
 */

size_t gnrc_coap_send(gnrc_coap_client_t *client, ipv6_addr_t *addr, uint16_t port, 
                                                                gnrc_coap_transfer_t *xfer)
{
    uint8_t srcport[2], dstport[2];             /* Bytes for UDP header function */
    gnrc_pktsnip_t *payload, *coap, *udp, *ip;
    size_t pktlen;

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
    coap = gnrc_coap_hdr_build(payload, xfer);
    if (coap == NULL) {
        DEBUG("coap: unable to allocate CoAP header\n");
        gnrc_pktbuf_release(payload);
        return 0;
    }

    /* allocate UDP header */
    srcport[0] = (uint8_t)client->netreg.demux_ctx;
    srcport[1] = client->netreg.demux_ctx >> 8;
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

int gnrc_coap_register_client(gnrc_coap_client_t *client) {
    if (client->netreg.pid == gnrc_coap_pid_get()) {
        DEBUG("coap: client already started on port %" PRIu32 "\n",
               client->netreg.demux_ctx);
        return -EINVAL;
    }
    /* To be safe, verify not already in client list. */
    DEBUG("coap: searching client list\n");
    gnrc_coap_client_t *i_client;
    LL_FOREACH(_client_list, i_client) {
        if (i_client == client) {
            DEBUG("coap: client already in list for port %" PRIu32 "\n",
                   client->netreg.demux_ctx);
            return -EINVAL;
        }
    }

    /* Find an unused ephemeral port and add to client list */
    client->netreg.demux_ctx = GNRC_COAP_EPHEMERAL_PORT_MIN;
    
    while (client->netreg.pid != gnrc_coap_pid_get()) {
        gnrc_netreg_entry_t *lookup;
        lookup = gnrc_netreg_lookup(GNRC_NETTYPE_UDP, client->netreg.demux_ctx);
        if (lookup == NULL) {
            LL_APPEND(_client_list, client);
            client->netreg.pid = gnrc_coap_pid_get();
            gnrc_netreg_register(GNRC_NETTYPE_UDP, &client->netreg);
            DEBUG("coap: registered client to port %" PRIu32 "\n",
                   client->netreg.demux_ctx);
        } else {
            client->netreg.demux_ctx++;
        }
    }
    return 0;
}
