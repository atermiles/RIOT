/*
 * Copyright and license as defined at
 * https://github.com/kaspar030/sock/blob/master/nanocoap/nanocoap.h.
 */

/**
 * @defgroup    net_nanocoap  nanocoap
 * @ingroup     net
 * @brief       Minimal implementation of CoAP protocol, RFC 7252
 *
 * @{
 *
 * @file
 * @brief       nanocoap definition
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 */

#ifndef NANOCOAP_H
#define NANOCOAP_H

#include <assert.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stddef.h>

#define COAP_PORT           (5683)
#define NANOCOAP_URL_MAX    (64)

#define COAP_OPT_URL        (11)

#define COAP_REQ            (0)
#define COAP_RESP           (2)

#define COAP_METHOD_GET     (1)
#define COAP_METHOD_POST    (2)
#define COAP_METHOD_PUT     (3)
#define COAP_METHOD_DELETE  (4)

#define COAP_CODE_205       ((2<<5) | 5)
#define COAP_CODE_404       ((4<<5) | 4)

typedef struct {
    uint8_t ver_t_tkl;
    uint8_t code;
    uint16_t id;
    uint8_t data[];
} coap_hdr_t;

typedef struct {
    coap_hdr_t *hdr;
    uint8_t url[NANOCOAP_URL_MAX];
    uint8_t *token;
    uint8_t *payload;
    unsigned payload_len;
    unsigned content_type;
} coap_pkt_t;

typedef ssize_t (*coap_handler_t)(coap_pkt_t* pkt, uint8_t *buf, size_t len);

typedef struct {
    const char *path;
    unsigned method;
    coap_handler_t handler;
} coap_endpoint_t;

extern const coap_endpoint_t endpoints[];
extern const unsigned nanocoap_endpoints_numof;

int coap_parse(coap_pkt_t* pkt, uint8_t *buf, size_t len);
ssize_t coap_build_reply(coap_pkt_t *pkt, unsigned code,
        uint8_t *rbuf, unsigned rlen,
        uint8_t *payload, unsigned payload_len);

static inline unsigned coap_get_ver(coap_pkt_t *pkt)
{
    return (pkt->hdr->ver_t_tkl & 0x60) >> 6;
}

static inline unsigned coap_get_type(coap_pkt_t *pkt)
{
    return (pkt->hdr->ver_t_tkl & 0x30) >> 4;
}

static inline unsigned coap_get_token_len(coap_pkt_t *pkt)
{
    return (pkt->hdr->ver_t_tkl & 0xf);
}

static inline unsigned coap_get_code_class(coap_pkt_t *pkt)
{
    return pkt->hdr->code >> 5;
}

static inline unsigned coap_get_code_detail(coap_pkt_t *pkt)
{
    return pkt->hdr->code & 0x1f;
}

static inline unsigned coap_get_id(coap_pkt_t *pkt)
{
    return ntohs(pkt->hdr->id);
}

static inline unsigned coap_get_total_hdr_len(coap_pkt_t *pkt)
{
    return sizeof(coap_hdr_t) + coap_get_token_len(pkt);
}

static inline uint8_t coap_code(unsigned class, unsigned detail)
{
   return (class << 5) | detail;
}

static inline void coap_hdr_set_code(coap_hdr_t *hdr, uint8_t code)
{
    hdr->code = code;
}

static inline void coap_hdr_set_type(coap_hdr_t *hdr, unsigned type)
{
    assert(!(type & ~0x3));
    hdr->ver_t_tkl &= ~0x30;
    hdr->ver_t_tkl |= type << 4;
}

#endif /* NANOCOAP_H */
