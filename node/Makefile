# WARNING: do not use this Makefile directly. Use it from root Makefile only

# ROOT_DIR, BUILD_DIR, CFLAGS, DEFINES are exported from root Makefile

SRC = src/node.c src/node_listener.c src/node_essentials.c src/node_handler.c src/node_app.c

EXEC_BUILD_DIR = $(BUILD_DIR)/$(BUILD_TYPE)/node
OBJS_BUILD = $(patsubst %.c, $(EXEC_BUILD_DIR)/%.o, $(SRC))
DEPENDS = $(patsubst %.c, %.d, $(SRC))

IDEPS = -I$(ROOT_DIR)/deps/log.c/src
INCLUDE = -Iinclude -I$(ROOT_DIR)/common/include $(IDEPS) -I$(ROOT_DIR)/deps/zlib
LIBS = $(BUILD_DIR)/$(BUILD_TYPE)/common/libcommon.a $(ROOT_DIR)/deps/zlib/libz.a

NAME = mesh_node

EXEC = $(EXEC_BUILD_DIR)/$(NAME)

.PHONY: build

build: $(EXEC)

$(EXEC): $(OBJS_BUILD) $(LIBS)
	$(CC) $^ -o $(EXEC) $(CFLAGS)

-include $(DEPENDS)

$(EXEC_BUILD_DIR)/%.o: %.c
	mkdir -p $(EXEC_BUILD_DIR)/src && $(CC) -MMD -MP $< -c -o $@ $(INCLUDE) $(CFLAGS) $(DEFINES)
