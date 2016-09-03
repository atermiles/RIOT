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
 * @brief       gcoap CLI support
 *
 * @author      Ken Bannister <kb2ma@runbox.com>
 *
 * @}
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "net/gnrc/coap.h"

static ssize_t _stats_handler(coap_pkt_t* pkt, uint8_t *buf, size_t len);

/* CoAP endpoints */
static const coap_endpoint_t _endpoints[] = {
    { "/cli/stats", COAP_METHOD_GET, _stats_handler },
};
static gcoap_listener_t _listener = {
    (coap_endpoint_t *)&_endpoints[0], 
    sizeof(_endpoints) / sizeof(_endpoints[0]), 
    NULL
};

/* 
 * Counts requests sent by CLI (not implemented yet). Uses a single byte to
 * simplify sending stats. 
 */
static uint8_t req_count = 0;

/* 
 * Response callback for /cli/stats. Returns the count of packets sent by the 
 * CLI.
 */
static ssize_t _stats_handler(coap_pkt_t* pkt, uint8_t *buf, size_t len)
{
    gcoap_resp_header(pkt, buf, len, COAP_CODE_CONTENT);

    *pkt->payload = req_count;
    
    gcoap_resp_content(pkt, 1, GCOAP_FORMAT_OCTET);
    return 0;
}

int gcoap_cli_cmd(int argc, char **argv)
{
    if (argc == 1) {
        /* show help for main commands */
        goto end;
    }

    if (strcmp(argv[1], "info") == 0) {
        if (argc == 2) {
            printf("CoAP server is listening on port %u\n", GCOAP_PORT);
            printf("CLI requests sent: %u\n", req_count);
            return 0;
        } else {
            printf("usage: %s info\n", argv[0]);
            return 1;
        }
    }

    end:
    printf("usage: %s <info>\n", argv[0]);
    return 1;
}

void gcoap_cli_init(void)
{
    gcoap_register_listener(&_listener);
}
