SRC_DIR			:= src
BIN_DIR			:= bin
OBJ_DIR			:= bin/obj

CC				:= g++
CC_FLAGS		:= -std=c++17 -g -O0 #-O2
CC_INCLUDE		:= 

LD				:= g++
LD_FLAGS		:= -g
LD_INCLUDE		:= -lpthread -lglfw -lvulkan -ldl -lX11 -lXxf86vm -lXrandr -lXi

CC_FILES_IN		:= $(wildcard $(SRC_DIR)/*.cpp)
CC_FILES_OUT	:= $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(CC_FILES_IN))

EXE_OUT			:= $(BIN_DIR)/planetarium

$(BIN_DIR):
	@mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(BIN_DIR)
	@echo "Compiling" $< to $@
	@$(CC) $(CC_FLAGS) $(CC_INCLUDE) -c $< -o $@

build: $(BIN_DIR) $(CC_FILES_OUT)
	@echo "Linking" $(EXE_OUT)
	@$(LD) $(LD_FLAGS) -o $(EXE_OUT) $(CC_FILES_OUT) $(LD_INCLUDE)

execute: build
	@$(EXE_OUT)

clean:
	@rm -r $(BIN_DIR)