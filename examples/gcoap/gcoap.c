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

static void handle_response(gnrc_coap_transfer_t *xfer);

static gnrc_coap_server_t server = { {NULL, 0, KERNEL_PID_UNDEF}, NULL, NULL};
static gnrc_coap_sender_t sender = { 0, 0, {0}, 0, NULL, 
                                     {KERNEL_PID_UNDEF, handle_response, NULL} };


static void handle_response(gnrc_coap_transfer_t *xfer)
{
    char *class_str = (xfer->xfer_code & GNRC_COAP_CLASS_SUCCESS) ? "success" : "error";
    printf("gcoap: %s, code %1" PRIu8 ".%02" PRIu8, class_str, 
                                                   (xfer->xfer_code & 0xE0) >> 5,
                                                    xfer->xfer_code & 0x1F);
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
    server.netreg.demux_ctx = (uint32_t)port;
    gnrc_coap_start_server(&server);
    printf("gcoap: started CoAP server on port %" PRIu16 "\n", port);
}

int gcoap_cmd(int argc, char **argv)
{
    /* Ordered as in the gnrc_coap_code_t enum, for easier processing */
    char *methods[] = {"get", "post", "put"}; 
    size_t i;
    gnrc_coap_transfer_t xfer = {0, 0, NULL, 0, NULL, 0};
    
    if (argc == 1)
        goto end;

    /* client methods */
    for (i = 0; i < sizeof(methods); i++) {
        if (strcmp(argv[1], methods[i]) == 0) {
            if (argc == 5 || argc == 6) {
                xfer.xfer_code = (gnrc_coap_code_t)(i+1);
                xfer.path      = argv[4];
                xfer.pathlen   = strlen(argv[4]);
                if (argc == 6) {
                    xfer.data        = (uint8_t *)argv[5];
                    xfer.datalen     = strlen(argv[5]);
                    xfer.data_format = GNRC_COAP_FORMAT_TEXT;
                }
                send(argv[2], argv[3], &xfer);
                return 0;
            } else {
                printf("usage: %s <get|put|post> <addr> <port> <path> [data]\n", argv[0]);
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
    }
    
    end:
    /* handles goto and fall through */
    printf("usage: %s <get|put|post|server>\n", argv[0]);
    return 1;
}
