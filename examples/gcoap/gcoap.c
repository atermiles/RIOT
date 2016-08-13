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

#include <stdio.h>

int gcoap_cmd(int argc, char **argv)
{
    if (argc > 1) {
        /* show help for main commands */
        goto end;
    }

    printf("Hi Mom\n");
    return 0;

    end:
    printf("usage: %s\n", argv[0]);
    return 1;
}
