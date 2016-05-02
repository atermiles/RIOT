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
 * @author      Ken Bannister
 */
 
#include <errno.h>
#include "utlist.h"
#include "net/gnrc/coap.h"
#include "gnrc_coap_internal.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

/** @brief Stack size for module thread */
#if ENABLE_DEBUG
#define GNRC_COAP_STACK_SIZE (THREAD_STACKSIZE_DEFAULT + THREAD_EXTRA_STACKSIZE_PRINTF)
#else
#define GNRC_COAP_STACK_SIZE (THREAD_STACKSIZE_DEFAULT)
#endif

/* Internal variables and functions */
static kernel_pid_t _pid = KERNEL_PID_UNDEF;
static char _msg_stack[GNRC_COAP_STACK_SIZE];

static int _coap_parse(gnrc_pktsnip_t *snip, gnrc_coap_transfer_t *xfer);
static void *_event_loop(void *arg);
static int _do_options(uint8_t *optsfld, gnrc_coap_transfer_t *xfer);
static int _parse_format_option(gnrc_coap_transfer_t *xfer, uint8_t *optval, 
                                                            uint8_t optlen);
/* static void _coap_hdr_print(gnrc_coap_hdr_t *hdr); */

/**
 * @brief Event/Message loop for gnrc_coap _pid thread.
 */
static void *_event_loop(void *arg)
{
    msg_t msg_rcvd, msg_queue[GNRC_COAP_MSG_QUEUE_SIZE];
    gnrc_pktsnip_t *pkt, *udp_snip;
    uint16_t port;
    gnrc_coap_client_t *client;
    gnrc_coap_transfer_t xfer;
    int result;

    (void)arg;
    msg_init_queue(msg_queue, GNRC_COAP_MSG_QUEUE_SIZE);

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
                udp_snip  = pkt->next;
                if (udp_snip->type != GNRC_NETTYPE_UDP) {
                    gnrc_pktbuf_release(pkt);
                    break;
                }
                port   = byteorder_ntohs(((udp_hdr_t *)udp_snip->data)->dst_port);
                client = gnrc_coap_client_find(port);
                if (!client) {
                    DEBUG("coap: client not found for port: %u\n", port);
                    gnrc_pktbuf_release(pkt);
                    break;
                }
                /* _coap_hdr_print((gnrc_coap_hdr_t *)pkt->data); */
                
                /* parse the message and pass to the client */
                result =_coap_parse(pkt, &xfer);
                if (result < 0) {
                    DEBUG("coap: parse failure: %d\n", result);
                    gnrc_pktbuf_release(pkt);
                    break;
                }
                if (client->response_cbf)
                    client->response_cbf(&xfer);
                gnrc_pktbuf_release(pkt);
                break;
                

            default:
                break;
        }
    }
    return 0;
}

/**
 * @brief Calculates the length in bytes required to write all of the CoAP options,
 *        and optionally writes the options to the provided field.
 * 
 * @param[in] optsfld  Options field (in pktsnip header) to fill in; if NULL, only 
 *                     calculates required length; otherwise, field is updated
 * @param[in] xfer     Resource transfer record containing option data
 * 
 * @return Count of bytes in options field
 * @return -EINVAL if path does not begin with '/'
 */
static int _do_options(uint8_t *optsfld, gnrc_coap_transfer_t *xfer)
{
    size_t optslen      = 0;
    uint8_t last_optnum = 0;

    /* Uri-Path: write each segment*/
    if ((xfer->path != NULL && xfer->pathlen > 0)
            || (xfer->pathlen == 1 && xfer->path[0] != '/')) {
        size_t seg_pos;         /* position of current segment in full path */
        if (xfer->path[0] != '/')
            return -EINVAL;     /* must be an absolute path */
        else
            seg_pos = 1;        /* skip the initial '/' */

        for (size_t i = seg_pos; i <= xfer->pathlen; i++) {
            if (i == xfer->pathlen || xfer->path[i] == '/') {
                size_t seglen = i - seg_pos;
                if (seglen > 0) {               /* 0 if previous char was '/' */
                    optslen += seglen + 1;      /* add 1 for option header */
                    if (optsfld != NULL) {
                        char *path_seg = (char *)xfer->path + seg_pos;
                        uint8_t delta  = GNRC_COAP_OPT_URI_PATH - last_optnum;
                        *optsfld       = (delta << 4) + seglen;     /* assume seglen < 13 */
                        memcpy(++optsfld, path_seg, seglen);
                        optsfld       += seglen;
                        last_optnum    = GNRC_COAP_OPT_URI_PATH;
                    }
                }
                seg_pos = i + 1;
            }
        }
    }
    /* Content-Format */
    if (xfer->datalen > 0) {
        optslen++;              /* account for option header */
        if (optsfld != NULL)
            *optsfld  = (uint8_t)(GNRC_COAP_OPT_CONTENT_FORMAT - last_optnum) << 4;
            
        /* Option has a value only when the format is not encoded as 0 */
        if (xfer->data_format > 0) {
            uint8_t format_len = (xfer->data_format > UINT8_MAX) ? 2 : 1;
            optslen += format_len;
            if (optsfld != NULL) {
                *optsfld += format_len;
                if (format_len == 1) {
                    *(++optsfld) = (uint8_t)xfer->data_format;
                } else {
                    network_uint16_t format = byteorder_htons((uint16_t)xfer->data_format);
                    memcpy(++optsfld, &format, format_len);
                    optsfld += format_len;
                }
                last_optnum = GNRC_COAP_OPT_CONTENT_FORMAT;
            }
        }
    }
    return optslen;
}

/*
static void _coap_hdr_print(gnrc_coap_hdr_t *hdr)
{
    uint8_t coap_ver, coap_type, token_len;
    
    coap_ver  = (hdr->ver_type_tkl & 0xC0) >> 6;
    coap_type = (hdr->ver_type_tkl & 0x30) >> 4;
    token_len = (hdr->ver_type_tkl & 0x0F);
    DEBUG("coap_hdr: Ver: %1" PRIu8 ", Type: %1" PRIu8 ", TKL: %1" PRIu8 "\n", 
                                            coap_ver, coap_type, token_len);
    DEBUG("coap_hdr: Code: %#2" PRIx8, hdr->code_class);
    DEBUG(", Message ID: %5" PRIu16 "\n", byteorder_ntohs(hdr->message_id));
}
*/

/**
 * @brief Parse CoAP parameters from header+payload bytes
 * 
 * @param[in] snip      Contents of CoAP header and payload
 * @param[inout] xfer   Target for parsed information
 * 
 * @return    0 on success; negative errno on failure
 */
static int _coap_parse(gnrc_pktsnip_t *snip, gnrc_coap_transfer_t *xfer)
{
    gnrc_coap_hdr_t *hdr4;    /* 4 byte static portion of header */
    uint8_t coap_type, token_len, *data, hdrlen = 0;
    uint16_t opt_delta = 0, optnum = 0;
    
    /* TODO Reject if not a CoAP packet. May be intended for some other app .*/
    hdr4      = (gnrc_coap_hdr_t *)snip->data;
    coap_type = (hdr4->ver_type_tkl & 0x30) >> 4;
    token_len =  hdr4->ver_type_tkl & 0x0F;
    if (coap_type != GNRC_COAP_TYPE_NON)
        return -EINVAL;

    xfer->xfer_code = hdr4->code_class;
    
    /* read/skip token and options */
    hdrlen = sizeof(gnrc_coap_hdr_t) + token_len;
    data   = (uint8_t *)snip->data + hdrlen;
    while (hdrlen < snip->size && opt_delta != GNRC_COAP_PAYLOAD_DELTA) {
        opt_delta = (*data & 0xF0) >> 4;        /* assume not extended (13/14) */
        optnum   += opt_delta;
        if (opt_delta == GNRC_COAP_PAYLOAD_DELTA) {
            hdrlen++;
        } else {
            hdrlen += (*data & 0xF) + 1;
            if (optnum == GNRC_COAP_OPT_CONTENT_FORMAT) {
                if (_parse_format_option(xfer, (uint8_t *)(data + 1), *data & 0xF) < 0)
                    return -EINVAL;
            }
        }
        data = (uint8_t *)snip->data + hdrlen;
    }

    /* record data location */
    if (snip->size > hdrlen) {
        if (opt_delta == GNRC_COAP_PAYLOAD_DELTA) {
            xfer->datalen = snip->size - hdrlen;
            xfer->data    = data;
        } else {
            return -EINVAL;  /* garbage beyond header; no payload indicated */
        }
    } else {
        xfer->datalen = 0;
        xfer->data    = NULL;
    }
    return 0;
}

/**
 * @brief Determines payload format from header option
 * 
 * @param[inout] xfer   Resource record for format
 * @param[in] optval    option value
 * @param[in] optlen    value length
 * 
 * @return    0 on success; negative errno on failure
 */
static int _parse_format_option(gnrc_coap_transfer_t *xfer, uint8_t *optval, 
                                                            uint8_t optlen)
{
    int result = 0;
    switch (optlen) {
        case 0:
            xfer->data_format = GNRC_COAP_FORMAT_TEXT;
            break;
        case 1:
            xfer->data_format = *optval;
            break;
        case 2:
            xfer->data_format = byteorder_ntohs(*((network_uint16_t *)optval));
            break;
        default:
            result = -EINVAL;
            break;
    }
    return result;
}

/* 
 * gnrc_coap internal functions
 */

kernel_pid_t gnrc_coap_pid_get(void) {
    return _pid;
}

/* 
 * gnrc_coap interface functions
 */
 
gnrc_pktsnip_t *gnrc_coap_hdr_build(gnrc_pktsnip_t *payload, gnrc_coap_transfer_t *xfer)
{
    gnrc_pktsnip_t *hdr;
    size_t hdr_len;
    gnrc_coap_hdr_t *hdr4;   /* 4 byte static portion of header */
    uint8_t *varflds;
    int optlen;
    
    /* Find options length so we can allocate the header snip. */
    optlen = _do_options(NULL, xfer);
    if (optlen < 0) {
        DEBUG("coap: invalid option: %d\n", optlen);
        return NULL;
    }

    /* allocate header */
    hdr_len = sizeof(gnrc_coap_hdr_t) + optlen + (payload == NULL ? 0 : 1);
    hdr     = gnrc_pktbuf_add(payload, NULL, hdr_len, GNRC_NETTYPE_UNDEF);
    if (hdr == NULL)
        return NULL;

    /* write initial static fields */
    hdr4               = (gnrc_coap_hdr_t *)hdr->data;
    hdr4->ver_type_tkl = (GNRC_COAP_VERSION << 6) + (GNRC_COAP_TYPE_NON << 4) + 0;
    hdr4->code_class   = xfer->xfer_code;
    hdr4->message_id   = byteorder_htons(1);
    
    /* write variable fields, starting with options */
    varflds = (uint8_t *)hdr->data + sizeof(gnrc_coap_hdr_t);
    _do_options(varflds, xfer);
    
    if (payload != NULL)
        varflds[optlen] = GNRC_COAP_PAYLOAD_MARKER;

    return hdr;
}

kernel_pid_t gnrc_coap_init(void)
{
    if (_pid != KERNEL_PID_UNDEF)
        return -EEXIST;

    _pid = thread_create(_msg_stack, sizeof(_msg_stack), THREAD_PRIORITY_MAIN - 1,
                            THREAD_CREATE_STACKTEST, _event_loop, NULL, "coap");

    return _pid;
}

/** @} */
