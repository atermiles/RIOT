/*
 * Copyright (c) 2015 Ken Bannister. All rights reserved.
 *  
 * Released under the Mozilla Public License 2.0, as published at the link below.
 * http://opensource.org/licenses/MPL-2.0
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Nethead CLI support
 *
 * @author      Ken Bannister
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
#include "nethead.h"

static void _nethead_state_change(nethead_state_t state);

static nethead_client_t _nethead = { _nethead_state_change };

static void _nethead_state_change(nethead_state_t state)
{
    if (state == NETHEAD_STATE_HELLO_ACK) 
        puts("Server registration succeeded");
    else if (state == NETHEAD_STATE_HELLO_FAIL) 
        puts("Server registration failed");
}

int nethead_cmd(int argc, char **argv)
{
    if (argc == 1)
        goto end;
        
    if (strcmp(argv[1], "state") == 0) {
        printf("Nethead in state %d\n", nethead_op_state());
        return 0;
    }

#if defined(NETHEAD_MGR_ADDR) && defined(NETHEAD_MGR_PORT)
    if (strcmp(argv[1], "init") == 0) {
        if (nethead_init(&_nethead, NETHEAD_MGR_ADDR, NETHEAD_MGR_PORT) == 0) {
            puts("Server registration sent");
        } else {
            puts("Server registration failed");
        }
        /* TODO Alternatively, accept user provided address and port. */
        return 0;
    }
#else
#error Nethead manager address or port macro not defined
#endif

    end:
    printf("usage: %s <init|state>\n", argv[0]);
    return 1;
}
