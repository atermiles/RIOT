# gCoAP Example

This application provides command line access to a GNRC implementation of CoAP. 
See the [CoAP spec][1] for background.

We provide two flavors of Makefile for this example:

* `Makefile` -- native networking
* `Makefile.slip` -- SLIP-based border router

See README-slip.md for details on setup of the SLIP flavor.

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

No resources on the server, which is expected from the default gCoAP server.

[1]: https://tools.ietf.org/html/rfc7252        "CoAP spec"
