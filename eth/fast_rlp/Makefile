CXX=clang++
CXX_FLAGS=-g --std=c++17 -Wfatal-errors -Wall -Wextra -Wpedantic -Wconversion -Wshadow -O3

# Final binary
BIN = main
# Put all auto generated stuff to this build dir.l
BUILD_DIR = ./build

# List of all .cpp source files.
CPP=$(wildcard *.cc)

# All .o files go to build dir.
OBJ = $(CPP:%.cc=$(BUILD_DIR)/%.o)
# Gcc/Clang will create these .d files containing dependencies.
DEP = $(OBJ:%.o=%.d)

all: run

$(BIN) : $(BUILD_DIR)/$(BIN)
	cp $^ $@

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

.PHONY : clean $(BIN)
clean :
	-rm $(BUILD_DIR)/$(BIN) $(OBJ) $(DEP)
