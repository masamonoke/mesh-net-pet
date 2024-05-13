# WARNING: do not use this Makefile directly. Use it from root Makefile only

# ROOT_DIR, BUILD_DIR, CFLAGS are exported from root Makefile

ifeq ($(origin CC),default)
  CC = gcc
endif

$(BUILD_DIR)/log.c.o:
	@mkdir -p $(BUILD_DIR)/$(BUILD_TYPE)/log.c && $(CC) log.c/src/log.c -c -o $(BUILD_DIR)/$(BUILD_TYPE)/log.c/log.c.o $(COMMON_OBJ) $(CFLAGS)

.PHONY: clean
