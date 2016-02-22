/*
 * Copyright (c) 2015 Ken Bannister. All rights reserved.
 *  
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     nethead
 * @{
 *
 * @file
 * @brief       Nethead implementation
 *
 * @author      Ken Bannister
 */
 
#include "net/gnrc/coap.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/ipv6/netif.h"
#include "nethead.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static void _handle_response(gnrc_coap_transfer_t *xfer);

static gnrc_coap_client_t _coap = { {NULL, 0, KERNEL_PID_UNDEF}, {0}, _handle_response, NULL};
static ipv6_addr_t _mgr_addr;
static uint16_t _mgr_port;
/* operational state */
static nethead_state_t _op_state  = NETHEAD_STATE_INIT;
static nethead_client_t *_client;
static nethead_nbr_stats_t _neighbors[NETHEAD_NBR_CAPACITY];

/* CoAP request callback */
static void _handle_response(gnrc_coap_transfer_t *xfer)
{
    if (xfer->xfer_code & GNRC_COAP_CLASS_SUCCESS)
        _op_state = NETHEAD_STATE_HELLO_ACK;
    else
        _op_state = NETHEAD_STATE_HELLO_FAIL;
        
    /* In turn, notify our client. */
    _client->state_cbf(_op_state);
}

static void _send_hello(void)
{
    gnrc_coap_transfer_t xfer = {0, NULL, 0, NULL, 0, 0};
    int bytes_sent;
    ipv6_addr_t local_prefix, *local_addr;

    xfer.xfer_code = GNRC_COAP_CODE_POST;
    xfer.path      = NETHEAD_PATH_HELLO;
    xfer.pathlen   = strlen(NETHEAD_PATH_HELLO);
    
    // Set IID for link local address
    ipv6_addr_set_link_local_prefix(&local_prefix);
    ipv6_addr_set_iid(&local_prefix, 0x0000000000000000);
    local_addr = gnrc_ipv6_netif_match_prefix(_client->iface_pid, &local_prefix);
    if (local_addr == NULL) {
        _op_state = NETHEAD_STATE_HELLO_FAIL;
        return;
    }
    xfer.data        = (uint8_t *)&local_addr->u64[1];
    xfer.datalen     = 8;
    xfer.data_format = GNRC_COAP_FORMAT_OCTET;

    bytes_sent = gnrc_coap_send(&_coap, &_mgr_addr, _mgr_port, &xfer);
    if (bytes_sent > 0)
        _op_state = NETHEAD_STATE_HELLO_REQ;
    else
        _op_state = NETHEAD_STATE_HELLO_FAIL;
}

/* 
 * Nethead interface functions
 */

int nethead_init(nethead_client_t *client, const char *addr_str, const char *port_str)
{
    /* read server address and port */
    if (ipv6_addr_from_str(&_mgr_addr, addr_str) == NULL)
        return -1;

    _mgr_port = (uint16_t)atoi(port_str);
    if (_mgr_port == 0)
        return -1;

    if (_coap.netreg.pid == KERNEL_PID_UNDEF)
        if (gnrc_coap_register_client(&_coap) < 0)
            return -2;
            
    _client = client;

    _send_hello();
    return (_op_state == NETHEAD_STATE_HELLO_FAIL) ? -3 : 0;
}

nethead_state_t nethead_op_state(void)
{
    return _op_state;
}

void nethead_post_l2(gnrc_netif_hdr_t *l2hdr, size_t hdrsize)
{
    nethead_nbr_stats_t nbr;
    
    nbr = _neighbors[0];
    /* assume a single neighbor for now; always write invariants */
    nbr.nbr_id = 1;
    memcpy(nbr.l2addr, gnrc_netif_hdr_get_src_addr(l2hdr),
           l2hdr->src_l2addr_len);
    
    nbr.lqi = l2hdr->lqi;
    nbr.rss = l2hdr->rss;
    DEBUG("nethead: L2 post from nbr: %u; rss,lqi: %d, %u\n", nbr.nbr_id, nbr.rss, nbr.lqi);
}

/** @} */
