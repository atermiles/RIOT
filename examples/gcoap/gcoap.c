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

static void handle_request(gnrc_coap_server_t *server, gnrc_coap_meta_t *msg_meta, 
                                                        gnrc_coap_transfer_t *xfer);
static void handle_response(gnrc_coap_sender_t *sender, gnrc_coap_meta_t *msg_meta, 
                                                        gnrc_coap_transfer_t *xfer);

static gnrc_coap_server_t server = { .listener     = {{NULL, 0, KERNEL_PID_UNDEF}, 
                                                      GNRC_COAP_LISTEN_REQUEST, &server, NULL}, 
                                     .request_cbf  = handle_request };
static gnrc_coap_sender_t sender = { .xfer_state   = 0, 
                                     .msg_meta     = {GNRC_COAP_TYPE_NON, 0, 0, {0}, 0}, 
                                     .xfer         = NULL, 
                                     .listener     = {{NULL, 0, KERNEL_PID_UNDEF}, 
                                                      GNRC_COAP_LISTEN_RESPONSE, &sender, NULL},
                                     .response_cbf = handle_response};

static void handle_request(gnrc_coap_server_t *server, gnrc_coap_meta_t *msg_meta, 
                                                       gnrc_coap_transfer_t *xfer)
{
    char *path_seg = NULL;
    uint8_t seglen, i = 0;
    
    if (msg_meta->xfer_code == GNRC_COAP_CODE_GET) {
        /* create empty response -- no resources */
        printf("gcoap: request for ");
        do {
            seglen = gnrc_coap_get_pathseg(xfer, i, path_seg);
            if (seglen > 0) {
                printf("/%.*s", seglen, path_seg);
                i++;
            } else {
                printf("%s\n", i==0 ? "/" : "");
            }
        } while (seglen > 0);
    }
}

static void handle_response(gnrc_coap_sender_t *sender, gnrc_coap_meta_t *msg_meta, 
                                                        gnrc_coap_transfer_t *xfer)
{
    char *class_str = (gnrc_coap_is_class(msg_meta->xfer_code, GNRC_COAP_CLASS_SUCCESS)) 
                            ? "success" : "error";
    printf("gcoap: %s, code %1u.%02u", class_str, 
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
        printf("\n");
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
    /* start server */
    server.listener.netreg.demux_ctx = (uint32_t)port;
    gnrc_coap_start_server(&server);
    printf("gcoap: started CoAP server on port %" PRIu16 "\n", port);
}

int gcoap_cmd(int argc, char **argv)
{
    /* Ordered as in the gnrc_coap_code_t enum, for easier processing */
    char *methods[] = {"get", "post", "put"}; 
    size_t i;
    gnrc_coap_transfer_t xfer = {0, NULL, 0, NULL, 0, 0};
    
    if (argc == 1)
        goto end;

    if (strcmp(argv[1], "c") == 0) {
        for (i = 0; i < sizeof(methods); i++) {
            if (strcmp(argv[2], methods[i]) == 0) {
                if (argc == 6 || argc == 7) {
                    sender.msg_meta.xfer_code = (gnrc_coap_code_t)(i+1);
                    xfer.path_source          = GNRC_COAP_PATHSOURCE_STRING;
                    xfer.path                 = argv[5];
                    xfer.pathlen              = strlen(argv[5]);
                    if (argc == 7) {
                        xfer.data        = (uint8_t *)argv[6];
                        xfer.datalen     = strlen(argv[6]);
                        xfer.data_format = GNRC_COAP_FORMAT_TEXT;
                    }
                    send(argv[3], argv[4], &xfer);
                    return 0;
                } else {
                    printf("usage: %s c <get|put|post> <addr> <port> <path> [data]\n", argv[0]);
                    return 1;
                }
            }
        }

    } else if (strcmp(argv[1], "s") == 0) {
        if (argc == 3) {
            start_server(argv[2]);
            return 0;
        } else {
            printf("usage: %s s <port>\n", argv[0]);
            return 1;
        }

    } else if (strcmp(argv[1], "t") == 0) {
        if (argc == 3 && atoi(argv[2]) <= 8) {
            sender.msg_meta.tokenlen = (uint8_t)atoi(argv[2]);
            return 0;
        } else {
            printf("usage: %s t <length>; default 0, to 8\n", argv[0]);
            return 1;
        }
    }
    
    end:
    /* handles goto and fall through */
    printf("usage: %s <(c)lient|(t)okenlen|(s)erver>\n", argv[0]);
    return 1;
}
