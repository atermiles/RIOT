/*
 * Copyright (c) 2015-2016 Ken Bannister. All rights reserved.
 *  
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    nethead  Nethead
 * @brief       Network monitoring interface
 * 
 * @{
 *
 * @file
 * @brief       Nethead definition
 *
 * @author      Ken Bannister
 */

#ifndef NETHEAD_H_
#define NETHEAD_H_

#include "kernel_types.h"
#include "net/gnrc/netif/hdr.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Path for registration message */
#define NETHEAD_PATH_HELLO "/nh/lo"

/**
 *  @brief Enum for state of Nethead tool
 */
typedef enum
{
    NETHEAD_STATE_INIT,         /**< Just started, no messaging yet */
    NETHEAD_STATE_HELLO_REQ,    /**< Hello request sent */
    NETHEAD_STATE_HELLO_ACK,    /**< Hello acknowledged */
    NETHEAD_STATE_HELLO_FAIL    /**< Hello request failed */
}
nethead_state_t;

/**
 * @brief Internal client for this Nethead agent
 */
typedef struct {
    kernel_pid_t iface_pid;                       /**< Network interface being monitored */
    void (*state_cbf)(nethead_state_t state);     /**< State change callback */
}
nethead_client_t;

/**
 * @brief   Initializes the module and registers with Nethead server.
 * 
 * Requires that networking is available. The caller should verify that
 * registration succeeds (op state NETHEAD_STATE_HELLO_ACK) before attempting
 * to send a message to the server.
 * 
 * @param[in] client Nethead client data container
 * @param[in] addr_str Address of the Nethead server
 * @param[in] port_str Port on the Nethead server
 * 
 * @return     0 on success
 * @return    -1 if an argument is not valid
 * @return    -2 if CoAP failed to initialize
 * @return    -3 if unable to communicate with the server
 */
int nethead_init(nethead_client_t *client, const char *addr_str, const char *port_str);

/**
 * @brief   Provides the operational state of the Nethead agent
 * 
 * @return  nethead_state_t enum value for the state
 */
nethead_state_t nethead_op_state(void);

/**
 * @brief   Pushes current stats to Nethead agent
 */
void nethead_push_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* NETHEAD_H_ */
/** @} */
