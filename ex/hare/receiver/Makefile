CXX=clang++
INCLUDES=-I/usr/local/opt/openssl/include/ -I./ -I../
CXX_FLAGS=-g --std=gnu++17 -Wfatal-errors -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wno-format-security -Wno-c99-extensions -O3 $(INCLUDES)
LDFLAGS=-L/usr/local/opt/openssl/lib/ -lcrypto

BIN=receiver
BUILD_DIR = ./build

# List of all .cc source files.
CPP=$(wildcard *.cc) $(wildcard ../gason/*.cc)

# All .o files go to build dir.
OBJ = $(CPP:%.cc=$(BUILD_DIR)/%.o)
# Gcc/Clang will create these .d files containing dependencies.
DEP = $(OBJ:%.o=%.d)

all: $(BIN)

$(BIN) : $(BUILD_DIR)/$(BIN)
	@cp $^ $@
	@echo success

$(BUILD_DIR)/$(BIN) : $(OBJ)
	@mkdir -p $(@D)
	echo $@
	$(CXX) $(CXX_FLAGS) $(LDFLAGS) $^ -o $@

# Include all .d files
-include $(DEP)

# Build target for every single object file.
# The potential dependency on header files is covered
# by calling `-include $(DEP)`.
$(BUILD_DIR)/%.o : %.cc
	mkdir -p $(@D)
	$(CXX) $(CXX_FLAGS) -MMD -c $< -o $@

run: $(BIN)
	./$^

test:
	make -C ./test

.PHONY : clean $(BIN) test
.DELETE_ON_ERROR:
clean :
	-rm $(BUILD_DIR)/$(BIN) $(OBJ) $(DEP)
