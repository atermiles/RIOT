# gCoAP Example

This application provides command line access to a GNRC implementation of CoAP. 
See the [CoAP spec][1] for background.

We provide two flavors of Makefile for this example:

* `Makefile` -- native networking
* `Makefile.slip` -- SLIP-based border router

See README-slip.md for details on setup of the SLIP flavor.

## WIP Notes
This section describes what has been implemented so far, and what remains to be done.

A sender, as well as a server, listens for incoming packets on a given port. In other words, there is a single source port per sender. How to timeout listening in this case -- add an 'is_active' attribute to gnrc_coap_listener_t, or simply remove the listener? Need to add a timer in any case.

### ToDo

1. Observe protocol (RFC 7641)
1. Confirmable messaging -- message type CON/ACK


[1]: https://tools.ietf.org/html/rfc7252        "CoAP spec"
