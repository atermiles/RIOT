/*
 * Copyright (c) 2015 Ken Bannister. All rights reserved.
 *  
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       GNRC CoAP example 
 *
 * @author      Ken Bannister
 *
 * @}
 */

#include <stdio.h>
#include "shell.h"

#define MAIN_MSG_QUEUE_SIZE (1)

extern int nethead_cmd(int argc, char **argv);

static const shell_command_t shell_commands[] = {
    { "nethead", "Nethead setup/status", nethead_cmd },
    { NULL, NULL, NULL }
};

int main(void)
{
    puts("Nethead setup/status app");
    
    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should never be reached */
    return 0;
}
