# WARNING: do not use this Makefile directly. Use it from root Makefile only

# ROOT_DIR, BUILD_DIR, CFLAGS, DEFINES are exported from root Makefile

ifeq ($(origin CC),default)
  CC = gcc
endif

SRC = src/append_string.c src/codes.c src/io.c src/control_utils.c src/custom_logger.c src/connection.c src/format.c

BUILD_TYPE = debug

DEPS_OBJ = $(ROOT_DIR)/bin/$(BUILD_TYPE)/log.c/log.c.o
OBJS_BUILD = $(patsubst %.c, $(BUILD_DIR)/$(BUILD_TYPE)/common/%.o, $(SRC)) $(DEPS_OBJ)
DEPENDS = $(patsubst %.c, %.d, $(SRC))

IDEPS = -I$(ROOT_DIR)/deps/log.c/src
INCLUDE = -Iinclude $(IDEPS)

$(BUILD_DIR)/$(BUILD_TYPE)/node: $(OBJS_BUILD)
	ar rcs $(BUILD_DIR)/$(BUILD_TYPE)/common/libcommon.a $(OBJS_BUILD)

-include $(DEPENDS)

$(BUILD_DIR)/$(BUILD_TYPE)/common/%.o: %.c
	mkdir -p $(BUILD_DIR)/$(BUILD_TYPE)/common/src && $(CC) -MMD -MP $< -c -fPIC -o $@ $(CFLAGS) $(INCLUDE) $(DEFINES)
