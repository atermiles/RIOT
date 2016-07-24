# Default Makefile, for host native networking

# name of your application
APPLICATION = nethead

# If no BOARD is found in the environment, use this default:
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

BOARD_INSUFFICIENT_MEMORY := airfy-beacon chronos msb-430 msb-430h nrf51dongle nrf6310 \
                             nucleo-f334 pca10000 pca10005 stm32f0discovery telosb wsn430-v1_3b \
                             wsn430-v1_4 yunjia-nrf51822 z1

# Bootstrapping information for Nethead
ifeq (,$(NETHEAD_MGR_ADDR))
  NETHEAD_MGR_ADDR='"bbbb::1"'
endif
ifeq (,$(NETHEAD_MGR_PORT))
  NETHEAD_MGR_PORT='"5683"'
endif
CFLAGS += -DNETHEAD_MGR_ADDR=$(NETHEAD_MGR_ADDR)
CFLAGS += -DNETHEAD_MGR_PORT=$(NETHEAD_MGR_PORT)

# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
# Specify the mandatory networking modules
USEMODULE += gnrc_ipv6_router_default
USEMODULE += gnrc_coap
# Add a routing protocol
USEMODULE += gnrc_rpl

USEMODULE += netstats_l2
USEMODULE += nethead

# Additional networking modules that can be dropped if not needed
USEMODULE += gnrc_icmpv6_echo

# Required by gcoap
USEMODULE += od
USEMODULE += fmt
# Add also the shell, some shell commands
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
