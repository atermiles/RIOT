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

#define COAP_OPT_URL             (11)
#define COAP_OPT_CONTENT_FORMAT  (12)

/* Message types -- confirmable, non-confirmable, etc. */
#define COAP_TYPE_CON       (0)
#define COAP_TYPE_NON       (1)
#define COAP_TYPE_ACK       (2)
#define COAP_TYPE_RST       (3)

/* Request message codes. Using "COAP_METHOD..." for clarity. */
#define COAP_CLASS_REQ      (0)
#define COAP_METHOD_GET     (1)
#define COAP_METHOD_POST    (2)
#define COAP_METHOD_PUT     (3)
#define COAP_METHOD_DELETE  (4)

/* Response messages codes; success */
#define COAP_CLASS_SUCCESS  (2<<5)
#define COAP_CODE_CREATED   ((2<<5) | 1)
#define COAP_CODE_DELETED   ((2<<5) | 2)
#define COAP_CODE_VALID     ((2<<5) | 3)
#define COAP_CODE_CHANGED   ((2<<5) | 4)
#define COAP_CODE_CONTENT   ((2<<5) | 5)
#define COAP_CODE_205       ((2<<5) | 5)
/* client error codes */
#define COAP_CLASS_CLIENT_FAILURE            ((4<<5) | 0)
#define COAP_CODE_BAD_REQUEST                ((4<<5) | 0)
#define COAP_CODE_UNAUTHORIZED               ((4<<5) | 1)
#define COAP_CODE_BAD_OPTION                 ((4<<5) | 2)
#define COAP_CODE_FORBIDDEN                  ((4<<5) | 3)
#define COAP_CODE_PATH_NOT_FOUND             ((4<<5) | 4)
#define COAP_CODE_404                        ((4<<5) | 4)
#define COAP_CODE_METHOD_NOT_ALLOWED         ((4<<5) | 5)
#define COAP_CODE_NOT_ACCEPTABLE             ((4<<5) | 6)
#define COAP_CODE_PRECONDITION_FAILED        ((4<<5) | 0xC)
#define COAP_CODE_REQUEST_ENTITY_TOO_LARGE   ((4<<5) | 0xD)
#define COAP_CODE_UNSUPPORTED_CONTENT_FORMAT ((4<<5) | 0xF)
/* server error codes */
#define COAP_CLASS_SERVER_FAILURE        ((5<<5) | 0)
#define COAP_CODE_INTERNAL_SERVER_ERROR  ((5<<5) | 0)
#define COAP_CODE_NOT_IMPLEMENTED        ((5<<5) | 1)
#define COAP_CODE_BAD_GATEWAY            ((5<<5) | 2)
#define COAP_CODE_SERVICE_UNAVAILABLE    ((5<<5) | 3)
#define COAP_CODE_GATEWAY_TIMEOUT        ((5<<5) | 4)
#define COAP_CODE_PROXYING_NOT_SUPPORTED ((5<<5) | 5)

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

/**
 * @brief   Parses buffer into a coap packet data structure.
 *
 * @param[out] pkt  Packet data structure
 * @param[in] buf   Buffer containing CoAP header and payload
 * @param[in] len   Length of buffer
 *
 * @return 0       on success
 * @return -BADMSG on a format error in the buffer
 */
int coap_parse(coap_pkt_t* pkt, uint8_t *buf, size_t len);

ssize_t coap_build_reply(coap_pkt_t *pkt, unsigned code,
        uint8_t *rbuf, unsigned rlen,
        uint8_t *payload, unsigned payload_len);

ssize_t coap_handle_req(coap_pkt_t *pkt, uint8_t *resp_buf, unsigned resp_buf_len);

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

/**
 *  @brief Provides the length of the CoAP header plus any token.
 */
static inline unsigned coap_get_total_hdr_len(coap_pkt_t *pkt)
{
    return sizeof(coap_hdr_t) + coap_get_token_len(pkt);
}

static inline uint8_t coap_code(unsigned class, unsigned detail)
{
   return (class << 5) | detail;
}

/**
 * @brief   Sets the method for a request, or the code for a response.
 */
static inline void coap_hdr_set_code(coap_hdr_t *hdr, uint8_t code)
{
    hdr->code = code;
}

static inline void coap_hdr_set_type(coap_hdr_t *hdr, unsigned type)
{
    /* assert correct range of type */
    assert(!(type & ~0x3));

    hdr->ver_t_tkl &= ~0x30;
    hdr->ver_t_tkl |= type << 4;
}

#endif /* NANOCOAP_H */
