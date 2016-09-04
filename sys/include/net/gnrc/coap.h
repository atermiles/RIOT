/*
 * Copyright (c) 2015-2016 Ken Bannister. All rights reserved.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    net_gnrc_coap  CoAP
 * @ingroup     net_gnrc
 * @brief       GNRC implementation of CoAP protocol, RFC 7252
 *
 * ## Architecture ##
 * Requests and responses are exchanged via an asynchronous RIOT message 
 * processing thread. Depends on nanocoap for base level structs and 
 * functionality.
 * 
 * Uses a single UDP port for communication to support RFC 6282 compression.
 * 
 * ## Server Operation ##
 * 
 * gcoap listens for requests on GCOAP_PORT, 5683 by default. You can redefine
 * this by uncommenting the appropriate lines in gcoap's make file.
 * 
 * gcoap allows an application to specify a collection ofrequest endpoint paths 
 * it wants to be notified about. Create an array of endpoints, coap_endpoint_t
 * structs. Use gcoap_register_listener() at application startup to pass in 
 * these endpoints, wrapped in a gcoap_listener_t.
 * 
 * gcoap itself defines an endpoint for /.well-known/core discovery, which lists
 * all of the registered paths.
 * 
 * ### Creating a response ###
 * An application endpoint includes a callback function, a coap_handler_t, which
 * accepts three parameters: 
 * 
 * 1. the CoAP packet, a coap_pkt_t, for the request
 * 2. the buffer pointer for the response
 * 3. the length of the buffer
 * 
 * After reading the request, the callback must use one or two functions 
 * provided by gcoap to format the response. The callback *must* read the 
 * request throughly before calling the functions, because the response buffer 
 * likely reuses the request buffer. So, here is the expected sequence for a
 * callback function:
 * 
 * 1. Read request completely, parse request payload, if any. Use the coap_pkt_t
 *    'payload' and 'payload_len' attributes.
 * 2. Call gcoap_resp_header(), which initializes the response, and sets the 
 *    response code and the position for the payload. If no response payload, 
 *    stop here.
 * 3. Write the response payload, using the updated 'payload' pointer attribute.
 * 4. Call gcoap_resp_content(), which updates the packet with the length 
 *    and format used.
 * 
 * Implementation note: In the callback, the response buffer is written out in 
 * two separate sections, for header and payload. Then, after the callback 
 * returns, we may need to insert one or more CoAP option tags in a particular 
 * order between these sections.
 *
 * @{
 *
 * @file
 * @brief       gcoap definition
 *
 * @author      Ken Bannister <kb2ma@runbox.com>
 */

#ifndef GCOAP_H_
#define GCOAP_H_

#include "net/gnrc.h"
#include "net/nanocoap.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/udp.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Size for module message queue */
#define GCOAP_MSG_QUEUE_SIZE (4)

/** @brief Server port; use RFC 7252 default if not defined */
#ifndef GCOAP_PORT
#define GCOAP_PORT  (5683)
#endif

/** @brief Size of the buffer used to build the response. */
#define GCOAP_RESPONSE_BUF_SIZE  (128)

/* Content-Format option codes */
#define GCOAP_FORMAT_TEXT    (0)
#define GCOAP_FORMAT_LINK   (40)
#define GCOAP_FORMAT_OCTET  (42)
#define GCOAP_FORMAT_JSON   (50)
#define GCOAP_FORMAT_CBOR   (60)

/** @brief gcoap-specific sentinel value to indicate no format specified. */
#define GCOAP_FORMAT_NONE   (65535)

/** @brief  Marks the boundary between header and payload */
#define GCOAP_PAYLOAD_MARKER (0xFF)

/**
 * @brief  A modular collection of endpoints for a server
 */
typedef struct gcoap_listener {
    coap_endpoint_t *endpoints;   /**< First element in the array of endpoints;
                                       must order alphabetically */
    size_t endpoints_len;         /**< Length of array */
    struct gcoap_listener *next;         /**< Next listener in list */
} gcoap_listener_t;

/**
 * @brief  Container for the state of gcoap itself
 */
typedef struct {
    gnrc_netreg_entry_t netreg_port;   /**< Registration for IP port */
    gcoap_listener_t *listeners;       /**< List of registered listeners */
} gcoap_state_t;

/**
 * @brief   Initializes the gcoap thread and device.
 *
 * Must call once before first use.
 *
 * @return  PID of the gcoap thread on success.
 * @return  -EEXIST, if thread already has been created.
 * @return  -EINVAL, if the IP port already is in use.
 */
kernel_pid_t gcoap_init(void);

/**
 * @brief   Starts listening for endpoint paths.
 *
 * @param listener listener containing the endpoints.
 */
void gcoap_register_listener(gcoap_listener_t *listener);

/**
 * @brief  Initializes a CoAP response packet on a buffer.
 *
 * Initializes payload location within the buffer based on packet setup.
 *
 * @param[in] pkt request/response packet
 * @param[in] buf buffer containing the packet
 * @param[in] len length of the buffer
 * @param[in] code response code
 */
void gcoap_resp_header(coap_pkt_t *pkt, uint8_t *buf, size_t len, unsigned code);

/**
 * @brief  Updates a CoAP response packet with the payload content.
 * 
 * Requires that the packet first has been initialized.
 * 
 * @param[in] pkt target packet info
 * @param[in] payload_len length of payload
 * @param[in] format format code for the payload
 */
void gcoap_resp_content(coap_pkt_t *pkt, size_t payload_len, unsigned format);


#ifdef __cplusplus
}
#endif

#endif /* GCOAP_H_ */
/** @} */
