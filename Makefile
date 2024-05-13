BUILD_TYPE = debug

ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BUILD_DIR = $(ROOT_DIR)/bin
DEFINES = -DLOG_FILE

TARGETS = $(BUILD_DIR)/node $(BUILD_DIR)/server $(BUILD_DIR)/client


CFLAGS = -Waddress -Wall -Warray-bounds -Wbool-operation -Wchar-subscripts -Wcomment \
		-Wmisleading-indentation -Wparentheses -Wextra -pedantic -Wstrict-prototypes \
		-Wshadow -Wconversion -Wvla -Wdouble-promotion -Wmissing-noreturn            \
		-Wmissing-format-attribute -Wmissing-prototypes -Wc99-extensions -Wunused-result

ifeq ($(BUILD_TYPE), debug)
	CFLAGS += -g -fsanitize=address,undefined,null,bounds -DDEBUG
else
	CFLAGS += -O3 -DNDEBUG
endif

export CFLAGS
export ROOT_DIR
export BUILD_DIR
export BUILD_TYPE
export DEFINES

$(TARGETS): build

.PHONY: build clean

build: build_node build_server build_client
	@echo Build done

server: build_server build_node
	cd $(BUILD_DIR)/$(BUILD_TYPE)/server && ./server
	rm -f $(BUILD_DIR)/$(BUILD_TYPE)/logs.log

node: build_node
	cd $(BUILD_DIR)/$(BUILD_TYPE)/node && ./node

client: build_client
	cd $(BUILD_DIR)/$(BUILD_TYPE)/client && ./client $(TARGET_ARGS)

build_node: build_common
	@cd node && $(MAKE) && cd ..

build_server: build_common
	@cd server && $(MAKE) && cd ..

build_client: build_common
	@cd client && $(MAKE) && cd ..

build_common: build_deps
	@cd common && $(MAKE) && cd ..

build_deps:
	@cd deps && $(MAKE) && cd ..

clean:
	rm -rf bin && rm -f compile_commands.json
