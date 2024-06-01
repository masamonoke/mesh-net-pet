# WARNING: do not use this Makefile directly. Use it from root Makefile only

# ROOT_DIR, BUILD_DIR, CFLAGS, DEFINES are exported from root Makefile

SRC = src/io.c src/control_utils.c src/custom_logger.c src/connection.c src/format_client_server.c \
	  src/format_server_node.c src/serving.c src/format.c src/routing.c src/format_app.c src/crc.c

OBJS_BUILD = $(patsubst %.c, $(BUILD_DIR)/$(BUILD_TYPE)/common/%.o, $(SRC)) $(DEPS_OBJ)
DEPENDS = $(patsubst %.c, %.d, $(SRC))

IDEPS = 
INCLUDE = -Iinclude $(IDEPS)

$(BUILD_DIR)/$(BUILD_TYPE)/node: $(OBJS_BUILD)
	ar rcs $(BUILD_DIR)/$(BUILD_TYPE)/common/libcommon.a $(OBJS_BUILD)

-include $(DEPENDS)

$(BUILD_DIR)/$(BUILD_TYPE)/common/%.o: %.c
	mkdir -p $(BUILD_DIR)/$(BUILD_TYPE)/common/src && $(CC) -MMD -MP $< -c -fPIC -o $@ $(CFLAGS) $(INCLUDE) $(DEFINES)
