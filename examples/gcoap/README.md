# gCoAP Example

This application provides command line access to a GNRC implementation of CoAP. See the [CoAP spec][1] for background, and the Modules>Networking>GNRC>CoAP topic in the source documentation for the structure of the implementation.

We support two setup options for this example:

### Native networking
Build with the standard `Makefile`. Follow the setup [generic instructions][2] on the RIOT wiki, especially for TAP-based network setup.

### SLIP-based border router

Build with `Makefile.slip`. Follow the setup instructions in README-slip.md, which are based on [6LBR instructions][3] on the Wiki. We also plan to provide or reference the ethos/UHCP instructions, but we haven't tried this option yet.

## Current Status
gCoAP is a work in progress (WIP), and is minimally useful. Available features include:

* Server listens on a port, and provides a callback for generation of an application-specific response.
* Client uses gnrc_coap_send() for a request, and listens for a response via a callback. Matches response on IP port and CoAP token.
* Message Type: Supports non-confirmable (NON) messaging.
* Options: Supports URI-Path (ASCII, no percent-encoding) and Content-Format for payload.


## Example Use
This example uses gCoAP to set up a server on one endpoint (board), configured as address aaaa::1, and then queries it for resources from another endpoint.

### Server setup

    coap s 5683

Expected response:

    gcoap: started CoAP server on port 5683

### Client query

    coap c get aaaa::1 5683 /.well-known/core

Example response:

    > gcoap: response Success, code 2.05, empty payload

The response indicates no resources on the server, which is expected from the default gCoAP server.

[1]: https://tools.ietf.org/html/rfc7252    "CoAP spec"
[2]: https://github.com/RIOT-OS/RIOT/wiki/Creating-your-first-RIOT-project    "generic instructions"
[3]: https://github.com/RIOT-OS/RIOT/blob/master/examples/gnrc_border_router/README.md    "6LBR instructions"
