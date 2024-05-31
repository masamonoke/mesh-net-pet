# WARNING: do not use this Makefile directly. Use it from root Makefile only

# ROOT_DIR, BUILD_DIR, CFLAGS are exported from root Makefile

ifeq ($(origin CC),default)
  CC = gcc
endif

CFLAGS=

all: zlib

.PHONY: clean zlib

zlib:
	cd zlib && $(MAKE)

clean:
	cd zlib && $(MAKE) clean
