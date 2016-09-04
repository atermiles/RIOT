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

/* Internal functions */
static void _receive(gnrc_pktsnip_t *pkt, ipv6_addr_t *src, uint16_t port);
static size_t _send(ipv6_addr_t *addr, uint16_t port, gnrc_pktsnip_t *coap_snip);
static ssize_t _well_known_core_handler(coap_pkt_t* pkt, uint8_t *buf, size_t len);
static size_t _finalize_response(coap_pkt_t *pkt, uint8_t *buf, size_t len);

/* Internal variables */
const coap_endpoint_t _default_endpoints[] = {
    { "/.well-known/core", COAP_METHOD_GET, _well_known_core_handler },
};
static gcoap_listener_t _default_listener = {
    (coap_endpoint_t *)&_default_endpoints[0], 
    sizeof(_default_endpoints) / sizeof(_default_endpoints[0]), 
    NULL
};
static gcoap_state_t _coap_state = {
    .netreg_port = {NULL, 0, KERNEL_PID_UNDEF},
    .listeners   = &_default_listener,
};
static kernel_pid_t _pid = KERNEL_PID_UNDEF;
static char _msg_stack[GCOAP_STACK_SIZE];


/* Event/Message loop for gcoap _pid thread. */
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

/* Handles incoming network IPC message. */
static void _receive(gnrc_pktsnip_t *pkt, ipv6_addr_t *src, uint16_t port)
{
    coap_pkt_t coap_pkt;
    uint8_t buf[GCOAP_RESPONSE_BUF_SIZE];
    size_t hdr_len;
    gnrc_pktsnip_t *resp_snip;

    /* Copy request into temporary buffer, and parse it as CoAP. */
    memcpy(buf, pkt->data, pkt->size);

    int result = coap_parse(&coap_pkt, buf, pkt->size);
    if (result < 0) {
        DEBUG("gcoap: parse failure: %d\n", result);
        goto exit;
    }

    if (coap_get_code_class(&coap_pkt) != COAP_CLASS_REQ) {
        puts("gcoap: not a CoAP request.");
        goto exit;
    }

    /* Find path for CoAP pkt among listener endpoints and execute callback. */
    gcoap_listener_t *listener = _coap_state.listeners;
    while (listener) {
        coap_endpoint_t *endpoint = listener->endpoints;
        for (size_t i = 0; i < listener->endpoints_len; i++) {
            if (i)
                endpoint++;
            
            int res = strcmp((char*)&coap_pkt.url, endpoint->path);
            if (res < 0) {
                continue;
            } else if (res > 0) {
                /* endpoints expected in alphabetical order */
                break;
            } else {
                if (endpoint->handler(&coap_pkt, &buf[0], pkt->size) < 0)
                    gcoap_resp_header(&coap_pkt, &buf[0], 
                                                 GCOAP_RESPONSE_BUF_SIZE, 
                                                 COAP_CODE_INTERNAL_SERVER_ERROR);
                goto send;
            }
        }
        listener = listener->next;
    }
    /* endpoint not found */
    gcoap_resp_header(&coap_pkt, &buf[0], GCOAP_RESPONSE_BUF_SIZE, 
                                          COAP_CODE_PATH_NOT_FOUND);

send:
    hdr_len   = _finalize_response(&coap_pkt, &buf[0], GCOAP_RESPONSE_BUF_SIZE);
    
    /* Allocate GNRC response packet, fill in contents, and send it. */
    resp_snip = gnrc_pktbuf_add(NULL, NULL, hdr_len + coap_pkt.payload_len, 
                                            GNRC_NETTYPE_UNDEF);
    memcpy(resp_snip->data, coap_pkt.hdr, hdr_len);
    if (coap_pkt.payload_len)
        memcpy((uint8_t *)resp_snip->data + hdr_len, coap_pkt.payload, 
                                                     coap_pkt.payload_len);
    _send(src, port, resp_snip);

exit:
    gnrc_pktbuf_release(pkt);
}

static int _register_port(gnrc_netreg_entry_t *netreg_port, uint16_t port)
{
    if (!gnrc_netreg_lookup(GNRC_NETTYPE_UDP, port)) {
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
 * Sends a CoAP message to the provided host/port.
 * 
 * @return Length of the packet
 * @return 0 if cannot send
 */
static size_t _send(ipv6_addr_t *addr, uint16_t port, gnrc_pktsnip_t *coap_snip)
{
    gnrc_pktsnip_t *udp, *ip;
    size_t pktlen;

    /* allocate UDP header */
    udp = gnrc_udp_hdr_build(coap_snip, (uint16_t)_coap_state.netreg_port.demux_ctx, 
                                                                               port);
    if (udp == NULL) {
        DEBUG("gcoap: unable to allocate UDP header\n");
        gnrc_pktbuf_release(coap_snip);
        return 0;
    }
    /* allocate IPv6 header */
    ip = gnrc_ipv6_hdr_build(udp, NULL, addr);
    if (ip == NULL) {
        DEBUG("gcoap: unable to allocate IPv6 header\n");
        gnrc_pktbuf_release(udp);
        return 0;
    }
    pktlen = gnrc_pkt_len(ip);          /* count length now; snips deallocated after send */

    /* send message */
    if (!gnrc_netapi_dispatch_send(GNRC_NETTYPE_UDP, GNRC_NETREG_DEMUX_CTX_ALL, ip)) {
        DEBUG("coap: unable to locate UDP thread\n");
        gnrc_pktbuf_release(ip);
        return 0;
    }
    return pktlen;
}

/* 
 * Handler for /.well-known/core. Lists registered handlers, except for 
 * /.well-known/core itself.
 */
static ssize_t _well_known_core_handler(coap_pkt_t* pkt, uint8_t *buf, size_t len)
{
   /* write header */
    gcoap_resp_header(pkt, buf, len, COAP_CODE_CONTENT);

    /* skip the first listener, gcoap itself */
    gcoap_listener_t *listener = _coap_state.listeners->next;
    
    /* write payload */
    uint8_t *bufpos            = pkt->payload;
    
    /* TODO handle response too long */
    while (listener) {
        coap_endpoint_t *endpoint = listener->endpoints;
        for (size_t i = 0; i < listener->endpoints_len; i++) {
            if (i) {
                *bufpos++ = ',';
            }
            *bufpos++ = '<';
            unsigned url_len = strlen(endpoint->path);
            memcpy(bufpos, endpoint->path, url_len);
            bufpos   += url_len;
            *bufpos++ = '>';
        }
        listener = listener->next;
    }
    
    /* response content */
    gcoap_resp_content(pkt, bufpos - pkt->payload, GCOAP_FORMAT_LINK);
    return 0;
}

/* 
 * Validates packet for response. Creates CoAP option for content format and
 * sets payload marker, if any. Packet now is ready to send.
 * 
 * Returns length of header + options.
 */
static size_t _finalize_response(coap_pkt_t *pkt, uint8_t *buf, size_t len) 
{
    int8_t format_len;
    (void)len;

    if (pkt->content_type == GCOAP_FORMAT_NONE)
        format_len = -1;
    else if (pkt->content_type == 0)
        format_len = 0;
    else if (pkt->content_type > UINT8_MAX)
        format_len = 2;
    else
        format_len = 1;
 
    uint8_t *pos = buf + coap_get_total_hdr_len(pkt);
    
    if (format_len >= 0) {
        /* write option header, as the first option */
        *pos++ = (uint8_t)((COAP_OPT_CONTENT_FORMAT << 4) + format_len);
        
        /* write option value */
        if (format_len) {
            if (format_len == 1) {
                *pos++ = (uint8_t)pkt->content_type;
            } else {
                network_uint16_t format = byteorder_htons((uint16_t)pkt->content_type);
                memcpy(pos, &format, format_len);
                pos += format_len;
            }
        }
    }

    /* write payload marker */
    if (pkt->payload_len) {
        *pos++ = GCOAP_PAYLOAD_MARKER;
    }
    return pos - buf;
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
    if (_register_port(&_coap_state.netreg_port, GCOAP_PORT) < 0)
        return -EINVAL;

    return _pid;
}

void gcoap_register_listener(gcoap_listener_t *listener)
{
    gcoap_listener_t *_last = _coap_state.listeners;
    while (_last->next)
        _last = _last->next;
        
    listener->next = NULL;
    _last->next = listener;
}

void gcoap_resp_header(coap_pkt_t *pkt, uint8_t *buf, size_t buflen, unsigned code)
{
    (void)buflen;
    
    /* Assume NON type request, so response type is the same. */
    coap_hdr_set_code(pkt->hdr, code);
    /* Create message ID since NON? */
    pkt->content_type = GCOAP_FORMAT_NONE;
    
    /* Reserve some space between the header and payload to write options, which 
     * is done after writing the payload. */
    pkt->payload     = buf + coap_get_total_hdr_len(pkt) + 10;
    pkt->payload_len = 0;
}

void gcoap_resp_content(coap_pkt_t *pkt, size_t payload_len, unsigned format)
{
    pkt->content_type = format;
    pkt->payload_len  = payload_len;
}
/** @} */
