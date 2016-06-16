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
 * @brief       GNRC implementation of CoAP protocol
 * 
 * Provides a CoAP message processing thread, and explicit support for client 
 * and server based use of CoAP.
 * 
 * ### Client Use ###
 * Use gnrc_coap_register_client() on a client struct, which assigns an ephemeral 
 * source port for requests. This port then allows matching a (non-confirmable) 
 * response on this port to the client. Use of a per-client source port reduces 
 * the need for a CoAP token to demux responses.
 * 
 * @{
 *
 * @file
 * @brief       GNRC CoAP definition
 *
 * @author      Ken Bannister
 */

#ifndef GNRC_COAP_H_
#define GNRC_COAP_H_

#include "byteorder.h"
#include "net/gnrc.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/udp.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Size for module message queue */
#define GNRC_COAP_MSG_QUEUE_SIZE (4)

/** @brief Port on which server listens */
#define GNRC_COAP_DEFAULT_PORT (5683)

/** @brief Minimum value for the high numbered port for request source */
#define GNRC_COAP_EPHEMERAL_PORT_MIN (20000)

/** @brief Maximum value for the high numbered port for request source */
#define GNRC_COAP_EPHEMERAL_PORT_MAX (21000)

/** @brief CoAP version embedded in each message */
#define GNRC_COAP_VERSION (1)

/** @brief Maximum length of a message token */
#define GNRC_COAP_MAX_TKLEN (8)

/** @brief Byte marker to separate header from payload */
#define GNRC_COAP_PAYLOAD_MARKER (0xFF)

/**
 *  @brief Message type enum -- confirmable, non-confirmable, etc.
 */
typedef enum
{
    GNRC_COAP_TYPE_CON = 0,
    GNRC_COAP_TYPE_NON = 1,
    GNRC_COAP_TYPE_ACK = 2,
    GNRC_COAP_TYPE_RST = 3
}
coap_msg_type_t;

/**
 *  @brief Enum for full message request/response codes.
 */
typedef enum
{
    GNRC_COAP_CODE_EMPTY   = 0x00,
    // request
    GNRC_COAP_CLASS_REQUEST = 0x00,
    GNRC_COAP_CODE_GET      = 0x01,
    GNRC_COAP_CODE_POST     = 0x02,
    GNRC_COAP_CODE_PUT      = 0x03,
    GNRC_COAP_CODE_DELETE   = 0x04,
    // success response
    GNRC_COAP_CLASS_SUCCESS = 0x40,
    GNRC_COAP_CODE_CREATED  = 0x41,
    GNRC_COAP_CODE_DELETED  = 0x42,
    GNRC_COAP_CODE_VALID    = 0x43,
    GNRC_COAP_CODE_CHANGED  = 0x44,
    GNRC_COAP_CODE_CONTENT  = 0x45,
    // client error response
    GNRC_COAP_CLASS_CLIENT_FAILURE            = 0x80,
    GNRC_COAP_CODE_BAD_REQUEST                = 0x80,
    GNRC_COAP_CODE_UNAUTHORIZED               = 0x81,
    GNRC_COAP_CODE_BAD_OPTION                 = 0x82,
    GNRC_COAP_CODE_FORBIDDEN                  = 0x83,
    GNRC_COAP_CODE_NOT_FOUND                  = 0x84,
    GNRC_COAP_CODE_METHOD_NOT_ALLOWED         = 0x85,
    GNRC_COAP_CODE_NOT_ACCEPTABLE             = 0x86,
    GNRC_COAP_CODE_PRECONDITION_FAILED        = 0x8C,
    GNRC_COAP_CODE_REQUEST_ENTITY_TOO_LARGE   = 0x8D,
    GNRC_COAP_CODE_UNSUPPORTED_CONTENT_FORMAT = 0x8F,
    // server error response
    GNRC_COAP_CLASS_SERVER_FAILURE        = 0xA0,
    GNRC_COAP_CODE_INTERNAL_SERVER_ERROR  = 0xA0,
    GNRC_COAP_CODE_NOT_IMPLEMENTED        = 0xA1,
    GNRC_COAP_CODE_BAD_GATEWAY            = 0xA2,
    GNRC_COAP_CODE_SERVICE_UNAVAILABLE    = 0xA3,
    GNRC_COAP_CODE_GATEWAY_TIMEOUT        = 0xA4,
    GNRC_COAP_CODE_PROXYING_NOT_SUPPORTED = 0xA5
}
gnrc_coap_code_t;

/**
 *  @brief Enum for option codes
 */
typedef enum
{
    GNRC_COAP_OPT_URI_PATH       = 11,
    GNRC_COAP_OPT_CONTENT_FORMAT = 12
}
gnrc_coap_option_code_t;

/**
 *  @brief Enum for media types used to identify content (payload) format
 */
typedef enum
{
    GNRC_COAP_FORMAT_TEXT  = 0,
    GNRC_COAP_FORMAT_LINK  = 40,
    GNRC_COAP_FORMAT_XML   = 41,
    GNRC_COAP_FORMAT_OCTET = 42,
    GNRC_COAP_FORMAT_EXI   = 47,
    GNRC_COAP_FORMAT_JSON  = 50,
    GNRC_COAP_FORMAT_CBOR  = 60
}
gnrc_coap_media_type_t;

/**
 *  @brief Enum for the source of options information in a transfer struct
 */
typedef enum
{
    GNRC_COAP_PATHSOURCE_STRING = 0,
    GNRC_COAP_PATHSOURCE_OPTIONS = 1
}
gnrc_coap_path_source_t;

/**
 *  @brief Enum for mode of listening for incoming messages
 */
typedef enum
{
    GNRC_COAP_LISTEN_REQUEST,      /**< Listening for requests (server) */
    GNRC_COAP_LISTEN_RESPONSE      /**< Listening for responses (client) */
}
gnrc_coap_listen_mode_t;

/**
 *  @brief Enum for state of resource transfer via exchange of messages
 */
typedef enum
{
    GNRC_COAP_XFER_INIT,      /**< No  messaging yet */
    GNRC_COAP_XFER_REQ,       /**< Request sent */
    GNRC_COAP_XFER_FAIL,      /**< Request failed */
    GNRC_COAP_XFER_SUCCESS    /**< Got response; acknowledged */
}
gnrc_coap_xfer_state_t;

/**
 * @brief   State for the gnrc coap module itself
 */
typedef struct {
    uint16_t last_message_id;    /**< Last message ID used */
} gnrc_coap_module_t;

/**
 * @brief   Initial fixed fields in a CoAP message header (4 bytes)
 *
 * @details Header structure shown below. Includes only static fields through 
 * Message ID, to support use within a pktsnip_t:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.unparsed}
 *    0                   1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Ver| T |  TKL  |      Code     |          Message ID           |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |   Token (if any, TKL bytes) ...
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |   Options (if any) ...
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |1 1 1 1 1 1 1 1|    Payload (if any) ...
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @see <a href="http://datatracker.ietf.org/doc/rfc7252/">
 *          RFC 7252, section 3
 *      </a>
 */
typedef struct __attribute__((packed)) {
    uint8_t ver_type_tkl;                /**< CoAP version, message type, token length */
    uint8_t code;                        /**< Message code, code detail */
    network_uint16_t message_id;         /**< Message ID */
} gnrc_coap_hdr4_t;

/**
 * @brief   Message metadata used in a CoAP header.
 */
typedef struct {
    coap_msg_type_t msg_type;            /**< Type of message: confirmable, ack, etc */
    gnrc_coap_code_t xfer_code;          /**< Transfer code: GET, POST, etc, or response */
    uint16_t message_id;                 /**< Message ID */
    uint8_t token[GNRC_COAP_MAX_TKLEN];  /**< Conversation token */
    uint8_t tokenlen;                    /**< Length of token */
} gnrc_coap_meta_t;

/**
 * @brief   Transfer of a resource to/from some host; separate from the resource
 *          itself
 * 
 * Useful for sending to a client, or as received from a server.
 */
typedef struct {
    gnrc_coap_path_source_t path_source; /**< Source of path data: string or options data; if
                                              options, path attribute points to the first option */
    char *path;                          /**< Path to resource */
    size_t pathlen;                      /**< Length of path, to make it safer to read path */
    uint8_t *data;                       /**< Data for resource representation */
    size_t datalen;                      /**< Length of data */
    gnrc_coap_media_type_t data_format;  /**< Format for data, defaults to octet */
} gnrc_coap_transfer_t;

/**
 * @brief   Listener for incoming messages, whether unsolicited to a server or 
 *          an expected response to a sender.
 * 
 * The network registration allows demuxing among listeners by supporting use 
 * of a unique source port per listener.
 */
typedef struct coap_listener {
    gnrc_netreg_entry_t netreg;          /**< Network registration for port */
    gnrc_coap_listen_mode_t mode;        /**< Mode of listening -- request, response, etc. */
    void *handler;                       /**< Handler appropriate for the listen mode */
    struct coap_listener *next;          /**< Next member in list; allows for registrar */
} gnrc_coap_listener_t;

/**
 * @brief   A message sender, which also listens for a response.
 * 
 * A sender may be a client sending a request, or a server sending a confirmable
 * response.
 */
typedef struct coap_sender {
    gnrc_coap_xfer_state_t xfer_state;   /**< State of messaging to complete the transfer */
    gnrc_coap_meta_t msg_meta;           /**< Request metadata for header */
    gnrc_coap_transfer_t *xfer;          /**< Request transfer details (optional, for retries) */
    gnrc_coap_listener_t listener;       /**< Listens for a response from the server */
    void (*response_cbf)(struct coap_sender*, gnrc_coap_meta_t*, gnrc_coap_transfer_t*);    
                                         /**< Callback for server response */
} gnrc_coap_sender_t;

/**
 * @brief   Setup for listening for requests as a CoAP server.
 */
typedef struct coap_server {
    gnrc_coap_listener_t listener;       /**< Listens for client requests */
    void (*request_cbf)(struct coap_server*, gnrc_coap_meta_t*, gnrc_coap_transfer_t*);
                                         /**< Request callback */
} gnrc_coap_server_t;


/**
 * @brief   Allocates and initializes a fresh CoAP header in the packet buffer.
 * 
 * Assumes a non-confirmable message with no token.
 *
 * @param[in] msg_meta Message metadata, including code
 * @param[in] xfer     Resource transfer path, and value
 * @param[in] payload  Payload contained in the CoAP message
 * 
 * @return  pointer to the newly created (and allocated) header
 * @return  NULL on allocation error or invalid CoAP option
 */
gnrc_pktsnip_t *gnrc_coap_hdr_build(gnrc_coap_meta_t *msg_meta, gnrc_coap_transfer_t *xfer, 
                                                                gnrc_pktsnip_t *payload);

/**
 * @brief   Sends a resource, or sends a request for a resource to a host.
 *
 * @param[in] client  CoAP sender for resource
 * @param[in] addr    Destination host address
 * @param[in] port    Destination port
 * @param[in] xfer    Resource transfer type (GET,POST,etc), path, and data

 * @return  Size of packet sent on success, or zero on failure.
 */
size_t gnrc_coap_send(gnrc_coap_sender_t *sender, ipv6_addr_t *addr, uint16_t port, 
                                                  gnrc_coap_transfer_t *xfer);

/**
 * @brief   Registers a listener for responses on an ephemeral port.
 * 
 * @param[inout] listener  Listener parameters; updates gnrc_coap pid and port
 * 
 * @return 0 on success
 * @return -EALREADY if listener already is registered
 */                                                             
int gnrc_coap_register_listener(gnrc_coap_listener_t *listener);

/**
 * @brief   Starts a server for listening for CoAP messages on a port.
 * 
 * @param[inout] server  Server parameters; protocol registration is updated when
 *                       started
 * @return 0 on success
 * @return -EINVAL if a server already is listening, or port number not valid
 */                                                             
int gnrc_coap_start_server(gnrc_coap_server_t *server);

/**
 * @brief  Verifies a method/response code as a member of a class of these codes.
 * 
 * @param[in]  code  Method/Response code to be verified
 * @param[in]  class Class to test for membership
 * 
 * @return true on success
 */
inline bool gnrc_coap_is_class(gnrc_coap_code_t code, gnrc_coap_code_t class)
{
    return code >= class && code <= (class + 0xF);
}

/**
 * @brief Provides a URI path segment from a Uri-Path option in a CoAP header
 * 
 * @param[in] xfer Resource transfer with pointer to first option header
 * @param[in] seg_index Index of the desired segmented
 * @param[out] path_seg Pointer to the segment if found; otherwise NULL
 * 
 * @return Length of requested segment, or 0 if not found
 */
uint8_t gnrc_coap_get_pathseg(gnrc_coap_transfer_t *xfer, uint8_t seg_index, char *path_seg);

/**
 * @brief strcmp against the path for a resource transfer
 * 
 * This function allows comparison of an option-based path, which may span multiple
 * options, like a single string. This way we need not allocate memory to
 * temporarily assemble the string.
 * 
 * @param[in] xfer Transfer for path
 * @param[in] path String for comparison
 * 
 * @return 0 if strings are equal; otherwise same sign as the first differing
 *         characters
 */
int gnrc_coap_pathcmp(gnrc_coap_transfer_t *xfer, char *path);

/**
 * @brief   Initializes the gnrc_coap thread and device.
 * 
 * Must call once before first use.
 *
 * @return  PID of the gnrc_coap thread on success.
 * @return  -EEXIST, if thread already has been created.
 */
kernel_pid_t gnrc_coap_init(void);

#ifdef __cplusplus
}
#endif

#endif /* GNRC_COAP_H_ */
/** @} */
