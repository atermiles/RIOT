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

int gcoap_cmd(int argc, char **argv)
{
    if (argc == 1) {
        /* show help for main commands */
        goto end;
    }

    if (strcmp(argv[1], "info") == 0) {
        if (argc == 2) {
            printf("CoAP server is listening on port %u\n", GCOAP_PORT);
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
