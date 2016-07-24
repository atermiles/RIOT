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
#include "net/gnrc/netapi.h"
#include "net/netopt.h"
#include "net/netstats.h"
#include "nethead.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static void _handle_response(gnrc_coap_sender_t *sender, gnrc_coap_meta_t *msg_meta, 
                                                        gnrc_coap_transfer_t *xfer);

static ipv6_addr_t _mgr_addr;
static uint16_t _mgr_port;
/* operational state */
static nethead_state_t _op_state  = NETHEAD_STATE_INIT;
static nethead_client_t *_client;
static gnrc_coap_sender_t _coap = { .xfer_state   = 0, 
                                    .msg_meta     = {GNRC_COAP_TYPE_NON, 0, 0, {0}, 0}, 
                                    .xfer         = NULL, 
                                    .listener     = {{NULL, 0, KERNEL_PID_UNDEF}, 
                                                     GNRC_COAP_LISTEN_RESPONSE, &_coap, NULL},
                                    .timeout_msg  = {0, GNRC_COAP_MSG_TYPE_TIMEOUT, {0}},
                                    .response_cbf = _handle_response};

/* CoAP request callback */
static void _handle_response(gnrc_coap_sender_t *sender, gnrc_coap_meta_t *msg_meta, 
                                                        gnrc_coap_transfer_t *xfer)
{
    (void) xfer;
    
    if (sender->xfer_state == GNRC_COAP_XFER_REQ_TIMEOUT) {
        _op_state = NETHEAD_STATE_HELLO_FAIL;
    } else if (sender->xfer_state == GNRC_COAP_XFER_FAIL) {
        _op_state = NETHEAD_STATE_HELLO_FAIL;
    } else  if (gnrc_coap_is_class(msg_meta->xfer_code, GNRC_COAP_CLASS_SUCCESS))
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

    _coap.msg_meta.xfer_code = GNRC_COAP_CODE_POST;
    xfer.path_source         = GNRC_COAP_PATHSOURCE_STRING;
    xfer.path                = NETHEAD_PATH_HELLO;
    xfer.pathlen             = strlen(NETHEAD_PATH_HELLO);
    
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
            
    _client = client;

    _send_hello();
    return (_op_state == NETHEAD_STATE_HELLO_FAIL) ? -3 : 0;
}

nethead_state_t nethead_op_state(void)
{
    return _op_state;
}

void nethead_push_stats(void)
{
    netstats_t *stats;
    int res = -ENOTSUP;

    res = gnrc_netapi_get(_client->iface_pid, NETOPT_STATS, 0, &stats, sizeof(&stats));
    if (res > 0) {
        DEBUG("Stats RX count %" PRIu32 "\n", stats->rx_count);
    }
}
/** @} */
