/*
 * Copyright (c) 2015-2016 Ken Bannister. All rights reserved.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     net_gnrc_coap
 * @{
 *
 * @file
 * @brief       GNRC's implementation of CoAP protocol
 *
 * Runs a thread (_pid) to manage request/response messaging.
 *
 * @author      Ken Bannister <kb2ma@runbox.com>
 */

#include <errno.h>
#include "net/gnrc/coap.h"
#include "thread.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

/** @brief Stack size for module thread */
#if ENABLE_DEBUG
#define GNRC_COAP_STACK_SIZE (THREAD_STACKSIZE_DEFAULT + THREAD_EXTRA_STACKSIZE_PRINTF)
#else
#define GNRC_COAP_STACK_SIZE (THREAD_STACKSIZE_DEFAULT)
#endif

/* Internal variables and functions */
static kernel_pid_t _pid = KERNEL_PID_UNDEF;
static char _msg_stack[GNRC_COAP_STACK_SIZE];

/**
 * @brief Event/Message loop for coap _pid thread.
 */
static void *_event_loop(void *arg)
{
    msg_t msg_rcvd, msg_queue[GNRC_COAP_MSG_QUEUE_SIZE];

    (void)arg;
    msg_init_queue(msg_queue, GNRC_COAP_MSG_QUEUE_SIZE);

    while (1) {
        msg_receive(&msg_rcvd);

        switch (msg_rcvd.type) {
            case GNRC_NETAPI_MSG_TYPE_RCV:
                DEBUG("coap: GNRC_NETAPI_MSG_TYPE_RCV\n");
                break;

            default:
                break;
        }
    }
    return 0;
}

kernel_pid_t gnrc_coap_init(void)
{
    if (_pid != KERNEL_PID_UNDEF)
        return -EEXIST;

    _pid = thread_create(_msg_stack, sizeof(_msg_stack), THREAD_PRIORITY_MAIN - 1,
                            THREAD_CREATE_STACKTEST, _event_loop, NULL, "coap");

    return _pid;
}

/** @} */
