PROGRAM=luftikus

EXTRA_CFLAGS=-DLWIP_HTTPD_CGI=1 -DLWIP_HTTPD_SSI=1 -I./fsdata \
	-DVERSION="\"$(shell date +"%Y-%m-%d").$(shell git rev-list --abbrev-commit --max-count 1 HEAD)\""

#Enable debugging
EXTRA_CFLAGS+=-DLWIP_DEBUG=1 -DHTTPD_DEBUG=LWIP_DBG_ON

EXTRA_COMPONENTS=extras/softuart extras/dht

include esp-open-rtos/common.mk

html:
	@echo "Generating fsdata.."
	cd fsdata && ./makefsdata
