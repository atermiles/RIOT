# SLIP Border Router Setup
`Makefile.slip` assumes use of SAMR21 Xplained Pro. This file describes how we use the board as a border router. See the README for the gnrc_border_router example for more background.

## USB serial port
The USB serial port requires a USB-TTL converter cable with 3.3 V output.

Pin connections for SAMR21 board:

* PA23 (RX) -- TXD wire
* PA22 (TX) -- RXD wire
* GND -- GND wire

## Network configuration
Define a TUN interface on an Ubuntu host with tunslip, in the `dist/tools/tunslip` directory. In the example below, the tun interface is host 1 on prefix `bbbb`.

    sudo ./tunslip6 -s ttyUSB0 -t tun0 bbbb::1/64

Configure the border router from the RIOT terminal. We configure the SLIP interface manually, via the FIB table. The example commands below assume SLIP is on interface 8, and use host 2 on the tun `bbbb` network.

    # Set address for SLIP interface
    ifconfig 8 add unicast bbbb::2/64
    # Set default gateway to SLIP interface
    fibroute add :: via bbbb:1 dev 8
    # Add the Ubuntu host to the neighbor cache
    ncache add 8 bbbb::1
    
## RPL configuration

On border router:
    
    # Assumes radio is interface 7.
    ifconfig 7 add unicast aaaa::1/64
    rpl init 7
    # Also execute the previous command on the remote,
    # as shown in the section below, before executing 
    # the next command on the LBR.
    
    # Border router radio interface is DAG root
    rpl root 1 aaaa::1
    # The previous step sets a global address on the 
    # remote node. You can execute 'rpl' on both
    # nodes to see the outcome.
    
On remote node:

    # Just init RPL. When root node is setup on LBR,
    # it will assign the global IP.
    # Assumes radio is interface 7.
    rpl init 7
