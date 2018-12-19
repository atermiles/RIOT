# nanocoap Test App
Provides a CLI based tool for interactive or scripted testing of nanocoap client
and server operation. Top level commands:

## *server*
Options are limited to *start* to start listening. Provides these resources:

`/value`<br>
read/write an unsigned 8-bit integer

`/.well-known/core`<br>
Read the list of resources. Expects a block2 based request or else returns the
first 16 bytes of the list.

## *client*
Provides get/put/post to an address, with an optional payload. Enter the command
with no other text (just *client*) to see command syntax. Options are similar to
the gcoap example app. A request always is sent confirmably.
