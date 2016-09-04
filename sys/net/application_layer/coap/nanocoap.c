/*
 * Copyright and license as defined at
 * https://github.com/kaspar030/sock/blob/master/nanocoap/nanocoap.c.
 */

/**
 * @ingroup     net_nanocoap
 * @{
 *
 * @file
 * @brief       Implementation of CoAP protocol
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "net/nanocoap.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

static int _decode_value(unsigned val, uint8_t **pkt_pos_ptr, uint8_t *pkt_end);

extern ssize_t _test_handler(coap_pkt_t* pkt, uint8_t *buf, size_t len);
extern ssize_t _well_known_core_handler(coap_pkt_t* pkt, uint8_t *buf, size_t len);

const coap_endpoint_t endpoints[] = {
    { "/test", COAP_METHOD_GET, _test_handler },
    { "/.well-known/core", COAP_METHOD_GET, _well_known_core_handler },
};

const unsigned nanocoap_endpoints_numof = sizeof(endpoints) / sizeof(endpoints[0]);

/* http://tools.ietf.org/html/rfc7252#section-3
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |Ver| T |  TKL  |      Code     |          Message ID           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   Token (if any, TKL bytes) ...
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   Options (if any) ...
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |1 1 1 1 1 1 1 1|    Payload (if any) ...
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
int coap_parse(coap_pkt_t *pkt, uint8_t *buf, size_t len)
{
    uint8_t *urlpos = pkt->url;
    coap_hdr_t *hdr = (coap_hdr_t *)buf;
    pkt->hdr = hdr;

    uint8_t *pkt_pos = hdr->data;
    uint8_t *pkt_end = buf + len;

    memset(pkt->url, '\0', NANOCOAP_URL_MAX);
    pkt->payload_len = 0;

    /* token value (tkl bytes) */
    pkt_pos += coap_get_token_len(pkt);

    /* parse options */
    int option_nr = 0;
    while (pkt_pos != pkt_end) {
        uint8_t option_byte = *pkt_pos++;
        if (option_byte == 0xff) {
            pkt->payload = pkt_pos;
            pkt->payload_len = buf + len - pkt_pos ;
            DEBUG("nanocoap: payload len = %u\n", pkt->payload_len);
            break;
        }
        else {
            int option_delta = _decode_value(option_byte >> 4, &pkt_pos, pkt_end);
            if (option_delta < 0) {
                DEBUG("nanocoap: bad op delta");
                return -EBADMSG;
            }
            int option_len = _decode_value(option_byte & 0xf, &pkt_pos, pkt_end);
            if (option_len < 0) {
                DEBUG("nanocoap: bad op len");
                return -EBADMSG;
            }
            option_nr += option_delta;
            DEBUG("nanocoap: option nr=%i len=%i\n", option_nr, option_len);

            switch (option_nr) {
                case COAP_OPT_URL:
                    {
                        *urlpos++ = '/';
                        memcpy(urlpos, pkt_pos, option_len);
                        urlpos += option_len;
                        break;
                    }
                default:
                    DEBUG("nanocoap: unhandled option nr=%i len=%i\n", option_nr, option_len);
            }

            pkt_pos += option_len;
        }
    }

    DEBUG("nanocoap: parsed pkt, code=%u.%u id=%" PRIu16 " payload_len=%u\n",
            coap_get_code_class(pkt), coap_get_code_detail(pkt),
            coap_get_id(pkt),
            pkt->payload_len);

    return 0;
}

ssize_t coap_handle_req(coap_pkt_t *pkt, uint8_t *resp_buf, unsigned resp_buf_len)
{
    if (coap_get_code_class(pkt) != COAP_CLASS_REQ) {
        puts("coap_handle_req(): not a request.");
        return -EBADMSG;
    }
    if (coap_get_type(pkt) != COAP_TYPE_NON) {
        puts("coap_handle_req(): not non-confirmable.");
        return -ENOTSUP;
    }

//    unsigned url_len = strlen((char*)pkt->url);
//    unsigned endpoint_len;

    for (size_t i = 0; i < nanocoap_endpoints_numof; i++) {
        int res = strcmp((char*)pkt->url, endpoints[i].path);
        if (res < 0) {
            continue;
        }
        else if (res > 0) {
            break;
        }
        else {
            return endpoints[i].handler(pkt, resp_buf, resp_buf_len);
        }
    }

    return coap_build_reply(pkt, COAP_CODE_404, resp_buf, resp_buf_len, NULL, 0);
}

ssize_t coap_build_reply(coap_pkt_t *pkt, unsigned code,
        uint8_t *rbuf, unsigned rlen,
        uint8_t *payload, unsigned payload_len)
{
    unsigned len = sizeof(coap_hdr_t) + coap_get_token_len(pkt);
    if ((len + payload_len + 1) > rlen) {
        return -ENOSPC;
    }

    memcpy(rbuf, pkt->hdr, len);

    coap_hdr_set_type((coap_hdr_t*)rbuf, COAP_TYPE_NON);
    coap_hdr_set_code((coap_hdr_t*)rbuf, code);

    if (payload_len) {
        rbuf += len;
        /* insert end of option marker */
        *rbuf = 0xFF;
        if (payload) {
            rbuf++;
            memcpy(rbuf, payload, payload_len);
        }
        len += payload_len +1;
    }

    return len;
}

ssize_t coap_build_hdr(coap_hdr_t *hdr, unsigned type, uint8_t *token, size_t token_len, unsigned code, uint16_t id)
{
    assert(!(type & ~0x3));
    assert(!(token_len & ~0x1f));

    memset(hdr, 0, sizeof(coap_hdr_t));
    hdr->ver_t_tkl = (0x1 << 6) | (type << 4) | token_len;
    hdr->code = code;
    hdr->id = htons(id);

    if (token_len) {
        memcpy(hdr->data, token, token_len);
    }

    return sizeof(coap_hdr_t) + token_len;
}

static int _decode_value(unsigned val, uint8_t **pkt_pos_ptr, uint8_t *pkt_end)
{
    uint8_t *pkt_pos = *pkt_pos_ptr;
    size_t left = pkt_end - pkt_pos;
    int res;
    switch (val) {
        case 13:
            {
            /* An 8-bit unsigned integer follows the initial byte and
               indicates the Option Delta minus 13. */
            if (left < 1) {
                return -ENOSPC;
            }
            uint8_t delta = *pkt_pos++;
            res = delta + 13;
            break;
            }
        case 14:
            {
            /* A 16-bit unsigned integer in network byte order follows
             * the initial byte and indicates the Option Delta minus
             * 269. */
            if (left < 2) {
                return -ENOSPC;
            }
            uint16_t delta;
            uint8_t *_tmp = (uint8_t*)&delta;
            *_tmp++= *pkt_pos++;
            *_tmp++= *pkt_pos++;
            res = ntohs(delta) + 269;
            break;
            }
        case 15:
            /* Reserved for the Payload Marker.  If the field is set to
             * this value but the entire byte is not the payload
             * marker, this MUST be processed as a message format
             * error. */
            return -EBADMSG;
        default:
            res = val;
    }

    *pkt_pos_ptr = pkt_pos;
    return res;
}
