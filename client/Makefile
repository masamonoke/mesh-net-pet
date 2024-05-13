# WARNING: do not use this Makefile directly. Use it from root Makefile only

# ROOT_DIR, BUILD_DIR, CFLAGS, DEFINES are exported from root Makefile

ifeq ($(origin CC),default)
  CC = gcc
endif

SRC = src/client.c

EXEC_BUILD_DIR = $(BUILD_DIR)/$(BUILD_TYPE)/client
OBJS_BUILD = $(patsubst %.c, $(EXEC_BUILD_DIR)/%.o, $(SRC))

IDEPS = -I$(ROOT_DIR)/deps/log.c/src
INCLUDE = -Iinclude -I$(ROOT_DIR)/common/include $(IDEPS)
LIBS = $(BUILD_DIR)/$(BUILD_TYPE)/common/libcommon.a

NAME = client

EXEC = $(EXEC_BUILD_DIR)/$(NAME)

.PHONY: build

build: $(EXEC)

$(EXEC): $(OBJS_BUILD)
	$(CC) $^ -o $(EXEC) $(CFLAGS) $(LIBS)

$(EXEC_BUILD_DIR)/%.o: %.c
	mkdir -p $(EXEC_BUILD_DIR)/src && $(CC) $< -c -o $@ $(INCLUDE) $(CFLAGS) $(DEFINES)

