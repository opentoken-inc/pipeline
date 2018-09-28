MKFILE_PATH:=$(abspath $(lastword $(MAKEFILE_LIST)))
ROOT:=$(shell dirname $(MKFILE_PATH))/
UNAME_S:=$(shell uname -s)

CXX:=g++
INCLUDES:=-isystem /usr/local/opt/openssl/include/ -I$(ROOT) -isystem $(ROOT)uWebsockets/src/
CXX_FLAGS:=-g --std=gnu++17 -Wfatal-errors -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wno-format-security -Wno-c99-extensions -O3 -flto $(INCLUDES)
LDFLAGS:=-L/usr/local/opt/openssl/lib/ -lssl -lcrypto -L$(ROOT) -luWS -flto

ifeq ($(UNAME_S),Darwin)
LIBUWS:=libuWS.dylib
else
LIBUWS:=libuWS.so
endif

BUILD_DIR = $(ROOT)build

BIN=$(THIS_BIN)

OBJ_ABS=$(CPP:%.cc=%.o)
OBJ=$(OBJ_ABS:$(ROOT)%=$(BUILD_DIR)/%)

DEP=$(OBJ:%.o=%.d)

all: $(BIN)

dbg_make:
	@echo ROOT $(ROOT)
	@echo CPP $(CPP)
	@echo OBJ $(OBJ)

$(ROOT)$(LIBUWS):
	@make -C $(ROOT)uWebsockets
	@cp $(ROOT)uWebsockets/$(LIBUWS) $(ROOT)

$(BIN) : $(BUILD_DIR)/$(BIN)
	@cp $^ $(ROOT)$@
	@echo success

$(BUILD_DIR)/$(BIN) : $(OBJ) $(ROOT)$(LIBUWS)
	@mkdir -p $(@D)
	echo $@
	$(CXX) $(CXX_FLAGS) $(LDFLAGS) $^ -o $@

# Include all .d files
-include $(DEP)

$(BUILD_DIR)/%.o : $(ROOT)%.cc
	@mkdir -p $(@D)
	$(CXX) $(CXX_FLAGS) -MMD -c $< -o $@

run: $(BIN)
	<ws_binance_2018_09_01_00_03_53_a0c01c2b982_cd36103aa0f934e6.json ./$^

test:
	make -C ./test

.PHONY : clean $(BIN) test receiver sender
.DELETE_ON_ERROR:
clean :
	-rm -f $(ROOT)$(BIN) $(BUILD_DIR)/$(BIN) $(OBJ) $(DEP)
