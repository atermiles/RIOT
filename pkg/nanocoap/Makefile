PKG_NAME=nanocoap
PKG_URL=https://github.com/kb2ma/sock
PKG_VERSION=734bf682ca1d749b33b76e07043e7c2b1330d3c7 

.PHONY: all

all: git-download
	@cp Makefile.nanocoap $(PKG_BUILDDIR)/nanocoap/Makefile
	"$(MAKE)" -C $(PKG_BUILDDIR)/nanocoap

include $(RIOTBASE)/pkg/pkg.mk
