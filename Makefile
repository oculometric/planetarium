SRC_DIR			:= src
BIN_DIR			:= bin
OBJ_DIR			:= bin/obj
SHR_DIR			:= shr

CC				:= g++
CC_FLAGS		:= -std=c++17 -g -O2
CC_INCLUDE		:= 

LD				:= g++
LD_FLAGS		:= -g
LD_INCLUDE		:= -lpthread -lglfw -lvulkan -ldl -lX11  -lXrandr -lXi

SC				:= glslc
SC_FLAGS		:=

CC_FILES_IN		:= $(wildcard $(SRC_DIR)/*.cpp)
CC_FILES_OUT	:= $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(CC_FILES_IN))

SC_V_FILES_IN	:= $(wildcard $(SHR_DIR)/*.vert)
SC_F_FILES_IN	:= $(wildcard $(SHR_DIR)/*.frag)
SC_FILES_OUT	:= $(patsubst $(SHR_DIR)/%.vert, $(BIN_DIR)/%_vert.spv, $(SC_V_FILES_IN)) $(patsubst $(SHR_DIR)/%.frag, $(BIN_DIR)/%_frag.spv, $(SC_F_FILES_IN))

EXE_OUT			:= $(BIN_DIR)/planetarium

$(BIN_DIR):
	@mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(BIN_DIR)
	@echo "Compiling" $< to $@
	@$(CC) $(CC_FLAGS) $(CC_INCLUDE) -c $< -o $@

$(BIN_DIR)/%_vert.spv: $(SHR_DIR)/%.vert $(BIN_DIR)
	@echo "Compiling vertex shader" $<
	@$(SC) $< -o $@

$(BIN_DIR)/%_frag.spv: $(SHR_DIR)/%.frag $(BIN_DIR)
	@echo "Compiling fragment shader" $<
	@$(SC) $< -o $@

shaders: $(SC_FILES_OUT)
	@cp $(SC_FILES_OUT) .

build: $(BIN_DIR) $(CC_FILES_OUT) shaders
	@echo "Linking" $(EXE_OUT)
	@$(LD) $(LD_FLAGS) -o $(EXE_OUT) $(CC_FILES_OUT) $(LD_INCLUDE)

execute: build
	@$(EXE_OUT)

clean:
	@rm -r $(BIN_DIR)
	@rm *.spv