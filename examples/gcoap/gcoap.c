/*
 * Copyright (c) 2015-2016 Ken Bannister. All rights reserved.
 *  
 * Released under the Mozilla Public License 2.0, as published at the link below.
 * http://opensource.org/licenses/MPL-2.0
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       GNRC CoAP CLI support
 *
 * @author      Ken Bannister
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
#include "net/gnrc/ipv6.h"
#include "net/gnrc/udp.h"
#include "net/gnrc/coap.h"
#include "od.h"
#include "fmt.h"

static void handle_request(gnrc_coap_meta_t *msg_meta, gnrc_coap_transfer_t *xfer,
                                                       ipv6_addr_t *src, uint16_t port);
static void handle_response(gnrc_coap_sender_t *sender, gnrc_coap_meta_t *msg_meta, 
                                                        gnrc_coap_transfer_t *xfer);

static gnrc_coap_server_t server = { .listener     = {{NULL, 0, KERNEL_PID_UNDEF}, 
                                                      GNRC_COAP_LISTEN_REQUEST, &server, NULL}, 
                                     .request_cbf  = handle_request };
/* For client request or server response */                                     
static gnrc_coap_sender_t sender = { .xfer_state   = 0, 
                                     .msg_meta     = {GNRC_COAP_TYPE_NON, 0, 0, {0}, 0}, 
                                     .xfer         = NULL, 
                                     .listener     = {{NULL, 0, KERNEL_PID_UNDEF}, 
                                                      GNRC_COAP_LISTEN_RESPONSE, &sender, NULL},
                                     .response_cbf = handle_response};

/* Request handling for server */
static void handle_request(gnrc_coap_meta_t *msg_meta, gnrc_coap_transfer_t *xfer,
                                                       ipv6_addr_t *src, uint16_t port)
{
    char *path_seg = NULL;
    uint8_t seglen, i = 0;
    gnrc_coap_transfer_t rsp_xfer = {0, NULL, 0, NULL, 0, 0};
    size_t bytes_sent;
    char token[GNRC_COAP_MAX_TKLEN * 2];

    /* print request path and token */
    printf("gcoap: request for path...\n");
    do {
        seglen = gnrc_coap_get_pathseg(xfer, i, &path_seg);
        if (seglen > 0) {
            printf("[%u] /%.*s\n", i, seglen, path_seg);
            i++;
        } else if (i == 0) {
            printf("[0] /\n");
        }
    } while (seglen > 0);
    
    for (i=0; i < msg_meta->tokenlen; i++) {
        fmt_byte_hex(&token[i*2], msg_meta->token[i]);
    }
    printf("gcoap: token %.*s\n", 
            msg_meta->tokenlen == 0 ? 6 : msg_meta->tokenlen * 2, 
            msg_meta->tokenlen == 0 ? "<none>" : token);

    /* create empty response -- no resources */
    if (msg_meta->xfer_code == GNRC_COAP_CODE_GET
            && gnrc_coap_pathcmp(xfer, "/.well-known/core") == 0) {
        sender.msg_meta.xfer_code = GNRC_COAP_CODE_CONTENT;
    } else {
        sender.msg_meta.xfer_code = GNRC_COAP_CODE_NOT_FOUND;
    }
    /* reflect request token */
    sender.msg_meta.msg_type  = GNRC_COAP_TYPE_NON;
    sender.msg_meta.tokenlen  = msg_meta->tokenlen;
    memcpy(&sender.msg_meta.token[0], &msg_meta->token[0], msg_meta->tokenlen);

    bytes_sent = gnrc_coap_send(&sender, src, port, &rsp_xfer);
    if (bytes_sent > 0)
        printf("gcoap: msg sent, %u bytes\n", bytes_sent);
    else
        puts("gcoap: msg send failed");
}

/* Response handling for client */
static void handle_response(gnrc_coap_sender_t *sender, gnrc_coap_meta_t *msg_meta, 
                                                        gnrc_coap_transfer_t *xfer)
{
    char *class_str;
    (void)sender;
    
    class_str = (gnrc_coap_is_class(msg_meta->xfer_code, GNRC_COAP_CLASS_SUCCESS)) 
                            ? "Success" : "Error";
    printf("gcoap: response %s, code %1u.%02u", class_str, 
                                                   (msg_meta->xfer_code & 0xE0) >> 5,
                                                    msg_meta->xfer_code & 0x1F);
    if (xfer->datalen > 0) {
        if (xfer->data_format == GNRC_COAP_FORMAT_TEXT
                || xfer->data_format == GNRC_COAP_FORMAT_LINK) {
            printf(", %u bytes\n%.*s\n", xfer->datalen, xfer->datalen,
                                                            (char *)xfer->data);
        } else {
            printf(", %u bytes\n", xfer->datalen);
            od_hex_dump(xfer->data, xfer->datalen, OD_WIDTH_DEFAULT);
        }
    } else {
        printf(", empty payload\n");
    }
}

static void send(char *addr_str, char *port_str, gnrc_coap_transfer_t *xfer)
{
    ipv6_addr_t addr;
    uint16_t port;
    size_t bytes_sent;

    /* parse destination address */
    if (ipv6_addr_from_str(&addr, addr_str) == NULL) {
        puts("Error: unable to parse destination address");
        return;
    }
    /* parse port */
    port = (uint16_t)atoi(port_str);
    if (port == 0) {
        puts("Error: unable to parse destination port");
        return;
    }

    puts("gcoap: about to send");
    bytes_sent = gnrc_coap_send(&sender, &addr, port, xfer);
    if (bytes_sent > 0)
        printf("gcoap: msg sent, %u bytes\n", bytes_sent);
    else
        puts("gcoap: msg send failed");
}

static void start_server(char *port_str)
{
    uint16_t port;

    /* parse port */
    port = (uint16_t)atoi(port_str);
    if (port == 0) {
        puts("Error: invalid port specified");
        return;
    }

    if (gnrc_coap_start_server(&server, port) == 0) {
        printf("gcoap: started CoAP server on port %" PRIu16 "\n", port);
        /* Used as source port in responses; however, not registered as a listener */
        sender.listener.netreg.demux_ctx = server.listener.netreg.demux_ctx;
    } else {
        printf("gcoap: failed to start CoAP server on port %" PRIu16 "\n", port);
    }
}

int gcoap_cmd(int argc, char **argv)
{
    /* Ordered as in the gnrc_coap_code_t enum, for easier processing */
    char *client_methods[] = {"get", "post", "put"}; 
    uint8_t i;
    gnrc_coap_transfer_t xfer = {0, NULL, 0, NULL, 0, 0};
    
    if (argc == 1) {
        /* show help for main commands */
        goto end;
    }

    for (i = 0; i < sizeof(client_methods) / sizeof(char*); i++) {
        if (strcmp(argv[1], client_methods[i]) == 0) {
            if (argc == 5 || argc == 6) {
                sender.msg_meta.xfer_code = (gnrc_coap_code_t)(i+1);
                xfer.path_source          = GNRC_COAP_PATHSOURCE_STRING;
                xfer.path                 = argv[4];
                xfer.pathlen              = strlen(argv[4]);
                if (argc == 6) {
                    xfer.data        = (uint8_t *)argv[5];
                    xfer.datalen     = strlen(argv[5]);
                    xfer.data_format = GNRC_COAP_FORMAT_TEXT;
                }
                send(argv[2], argv[3], &xfer);
                return 0;
            } else {
                printf("usage: %s <get|post|put> <addr> <port> <path> [data]\n", argv[0]);
                return 1;
            }
        }
    }

    if (strcmp(argv[1], "server") == 0) {
        if (argc == 3) {
            start_server(argv[2]);
            return 0;
        } else {
            printf("usage: %s server <port>\n", argv[0]);
            return 1;
        }

    } else if (strcmp(argv[1], "token") == 0) {
        if (argc == 3 && atoi(argv[2]) <= 8) {
            sender.msg_meta.tokenlen = (uint8_t)atoi(argv[2]);
            return 0;
        } else {
            printf("usage: %s token <length>; default 0, to 8\n", argv[0]);
            return 1;
        }
    }
    
    end:
    printf("usage: %s <get|post|put|server|token>\n", argv[0]);
    return 1;
}
