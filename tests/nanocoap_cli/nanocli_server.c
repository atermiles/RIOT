/*
 * Copyright (c) 2018 Ken Bannister. All rights reserved.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       nanocoap test server
 *
 * @author      Ken Bannister <kb2ma@runbox.com>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "net/nanocoap_sock.h"
#include "net/sock/udp.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

static void _start_server(void)
{
    uint8_t buf[128];
    sock_udp_ep_t local = { .port=COAP_PORT, .family=AF_INET6 };
    nanocoap_server(&local, buf, sizeof(buf));
}

int nanotest_server_cmd(int argc, char **argv)
{
    if (argc == 2) {
        if (strncmp("start", argv[1], 5) == 0) {
            puts("starting server\n");
            _start_server();
            return 0;
        }
    }

    printf("usage: %s start\n", argv[0]);
    return 1;
}
