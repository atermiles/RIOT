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
 * @brief       Nethead example 
 *
 * @author      Ken Bannister
 *
 * @}
 */

#include <stdio.h>
#include "shell.h"
#include "msg.h"

#define MAIN_QUEUE_SIZE (4)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

extern int nethead_cmd(int argc, char **argv);

static const shell_command_t shell_commands[] = {
    { "nethead", "Nethead setup/status", nethead_cmd },
    { NULL, NULL, NULL }
};

int main(void)
{
    /* for the thread running the shell */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    puts("Nethead setup/status app");
    
    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should never be reached */
    return 0;
}
