# WARNING: do not use this Makefile directly. Use it from root Makefile only

# ROOT_DIR, BUILD_DIR, CFLAGS, DEFINES are exported from root Makefile

SRC = src/server.c src/server_listener.c src/server_essentials.c src/server_handler.c

EXEC_BUILD_DIR = $(BUILD_DIR)/$(BUILD_TYPE)/server
OBJS_BUILD = $(patsubst %.c, $(EXEC_BUILD_DIR)/%.o, $(SRC))
DEPENDS = $(patsubst %.c, %.d, $(SRC))

IDEPS = -I../deps/log.c/src
INCLUDE = -Iinclude -I../common/include $(IDEPS)
LIBS = $(BUILD_DIR)/$(BUILD_TYPE)/common/libcommon.a

NAME = server

EXEC = $(EXEC_BUILD_DIR)/$(NAME)

.PHONY: build

build: $(EXEC)

$(EXEC): $(OBJS_BUILD) $(LIBS)
	$(CC) $^ -o $(EXEC) $(CFLAGS)

-include $(DEPENDS)

$(EXEC_BUILD_DIR)/%.o: %.c
	mkdir -p $(EXEC_BUILD_DIR)/src && $(CC) -MMD -MP $< -c -o $@ $(INCLUDE) $(CFLAGS) $(DEFINES)
