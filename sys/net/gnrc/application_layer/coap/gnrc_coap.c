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
 * @brief       GNRC's implementation of CoAP protocol
 *
 * Runs a thread (_pid) to manage request/response messaging.
 *
 * @author      Ken Bannister <kb2ma@runbox.com>
 */

#include <errno.h>
#include "net/nanocoap.h"
#include "net/gnrc/coap.h"
#include "gcoap_internal.h"
#include "thread.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

/** @brief Stack size for module thread */
#if ENABLE_DEBUG
#define GCOAP_STACK_SIZE (THREAD_STACKSIZE_DEFAULT + THREAD_EXTRA_STACKSIZE_PRINTF)
#else
#define GCOAP_STACK_SIZE (THREAD_STACKSIZE_DEFAULT)
#endif

/* Internal variables and functions */
static gcoap_module_t _coap_module = {
                                    .netreg_port = {NULL, 0, KERNEL_PID_UNDEF},
                                     };
static kernel_pid_t _pid = KERNEL_PID_UNDEF;
static char _msg_stack[GCOAP_STACK_SIZE];

static void _receive(gnrc_pktsnip_t *pkt, ipv6_addr_t *src, uint16_t port);

/**
 * @brief Event/Message loop for coap _pid thread.
 */
static void *_event_loop(void *arg)
{
    msg_t msg_rcvd, msg_queue[GCOAP_MSG_QUEUE_SIZE];
    gnrc_pktsnip_t *pkt, *udp_snip, *ipv6_snip;
    ipv6_addr_t *src_addr;
    uint16_t port;

    (void)arg;
    msg_init_queue(msg_queue, GCOAP_MSG_QUEUE_SIZE);

    while (1) {
        msg_receive(&msg_rcvd);

        switch (msg_rcvd.type) {
            case GNRC_NETAPI_MSG_TYPE_RCV:
                /* find client from UDP destination port */
                DEBUG("coap: GNRC_NETAPI_MSG_TYPE_RCV\n");
                pkt = (gnrc_pktsnip_t *)msg_rcvd.content.ptr;
                if (pkt->type != GNRC_NETTYPE_UNDEF) {
                    gnrc_pktbuf_release(pkt);
                    break;
                }
                udp_snip = pkt->next;
                if (udp_snip->type != GNRC_NETTYPE_UDP) {
                    gnrc_pktbuf_release(pkt);
                    break;
                }

                /* read source port and address */
                port = byteorder_ntohs(((udp_hdr_t *)udp_snip->data)->src_port);

                LL_SEARCH_SCALAR(udp_snip, ipv6_snip, type, GNRC_NETTYPE_IPV6);
                assert(ipv6_snip != NULL);
                src_addr = &((ipv6_hdr_t *)ipv6_snip->data)->src;

                _receive(pkt, src_addr, port);
                break;

            default:
                break;
        }
    }
    return 0;
}

static void _receive(gnrc_pktsnip_t *pkt, ipv6_addr_t *src, uint16_t port)
{
    coap_pkt_t coap_pkt;
    (void)src;
    (void)port;

    int result = coap_parse(&coap_pkt, pkt->data, pkt->size);
    if (result < 0) {
        DEBUG("coap: parse failure: %d\n", result);
        gnrc_pktbuf_release(pkt);
        return;
    }
/*
    if (listener->mode == GNRC_COAP_LISTEN_RESPONSE)
        _receive_response((gnrc_coap_sender_t *)listener->handler, &msg_meta, &xfer);

    else if (listener->mode == GNRC_COAP_LISTEN_REQUEST)
        _receive_request((gnrc_coap_server_t *)listener->handler, &msg_meta, &xfer, src, port);
*/
    gnrc_pktbuf_release(pkt);
}

static int _register_port(gnrc_netreg_entry_t *netreg_port, uint16_t port)
{
    if (gnrc_netreg_lookup(GNRC_NETTYPE_UDP, port) == NULL) {
        netreg_port->demux_ctx = port;
        netreg_port->pid       = gcoap_pid_get();
        gnrc_netreg_register(GNRC_NETTYPE_UDP, netreg_port);
        DEBUG("coap: registered UDP port %" PRIu32 "\n",
               netreg_port->demux_ctx);
        return 0;
    } else {
        return -EINVAL;
    }
}

/*
 * gcoap internal functions
 */

kernel_pid_t gcoap_pid_get(void) {
    return _pid;
}

/*
 * gcoap interface functions
 */

kernel_pid_t gcoap_init(void)
{
    if (_pid != KERNEL_PID_UNDEF)
        return -EEXIST;

    _pid = thread_create(_msg_stack, sizeof(_msg_stack), THREAD_PRIORITY_MAIN - 1,
                            THREAD_CREATE_STACKTEST, _event_loop, NULL, "coap");

    /* must establish pid first */
    if (_register_port(&_coap_module.netreg_port, GCOAP_PORT) < 0)
        return -EINVAL;

    return _pid;
}

/** @} */
