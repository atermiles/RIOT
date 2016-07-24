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
#include <stdlib.h>
#include <string.h>
#include "net/gnrc/ipv6/netif.h"
#include "nethead.h"

static void _nethead_state_change(nethead_state_t state);

static nethead_client_t _nethead = { KERNEL_PID_UNDEF, _nethead_state_change };

static void _nethead_state_change(nethead_state_t state)
{
    if (state == NETHEAD_STATE_HELLO_ACK) 
        puts("Server registration succeeded");
    else if (state == NETHEAD_STATE_HELLO_FAIL) 
        puts("Server registration failed");
}

int _nethead_init(char *iface)
{
    kernel_pid_t iface_pid   = (kernel_pid_t) atoi(iface);
    gnrc_ipv6_netif_t *entry = gnrc_ipv6_netif_get(iface_pid);

    if (entry == NULL) {
        puts("Unknown interface specified");
        return 1;
    } else {
        _nethead.iface_pid = iface_pid;
    }

#if defined(NETHEAD_MGR_ADDR) && defined(NETHEAD_MGR_PORT)
    if (nethead_init(&_nethead, NETHEAD_MGR_ADDR, NETHEAD_MGR_PORT) == 0) {
        puts("Server registration sent");
    } else {
        puts("Server registration failed");
    }
    /* TODO Alternatively, accept user provided address and port. */
    return 0;
#else
#error Nethead manager address or port macro not defined
    return 1;
#endif
}

int nethead_cmd(int argc, char **argv)
{
    if (argc == 1)
        goto end;
        
    if (strcmp(argv[1], "state") == 0) {
        printf("Nethead in state %d\n", nethead_op_state());
        return 0;
    }
        
    if (strcmp(argv[1], "push") == 0) {
        nethead_push_stats();
        return 0;
    }

    if (argc == 2)
        goto end;

    if (strcmp(argv[1], "init") == 0)
        return _nethead_init(argv[2]);

    end:
    printf("usage: %s ...\n", argv[0]);
    printf(" args: init <if_id>\n");
    printf("       state\n");
    printf("          Print state to console\n");
    printf("       push\n");
    printf("          Push current stats to server\n");
    return 1;
}
