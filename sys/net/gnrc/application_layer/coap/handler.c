#include "net/nanocoap.h"

/*
 * These functions are not used by gcoap. They are here temporarily to 
 * satisfy nanocoap for compilation.
 */

ssize_t _test_handler(coap_pkt_t* pkt, uint8_t *buf, size_t len)
{
    (void)pkt;
    (void)buf;
    (void)len;

    return -1;
}

ssize_t _well_known_core_handler(coap_pkt_t* pkt, uint8_t *buf, size_t len)
{
    (void)pkt;
    (void)buf;
    (void)len;

    return -1;
}
