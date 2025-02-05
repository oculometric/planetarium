SRC_DIR			:= src/
BIN_DIR			:= bin/
OBJ_DIR			:= bin/obj/
SHR_DIR			:= shr/

CC				:= g++
<<<<<<< Updated upstream
CC_FLAGS		:= -std=c++20 -g -O0 -Iinc -Istui/inc -Wall
=======
CC_FLAGS		:= -std=c++20 -g -O0 -Iinc -Iinc/graphics -Iinc/math -Iinc/scenegraph -Istui/inc -Wall
>>>>>>> Stashed changes
CC_INCLUDE		:= 

LD				:= g++
LD_FLAGS		:= -g
LD_INCLUDE		:= -lpthread -lglfw -lvulkan -ldl -lX11  -lXrandr -lXi

SC				:= glslc
SC_FLAGS		:=

DEP_FLAGS		:= -MMD -MP

CC_FILES_IN		:= $(wildcard $(SRC_DIR)*.cpp) $(wildcard $(SRC_DIR)*/*.cpp)
CC_FILES_OUT	:= $(patsubst $(SRC_DIR)%.cpp, $(OBJ_DIR)%.o, $(CC_FILES_IN))
CC_FILES_DEP	:= $(patsubst $(SRC_DIR)%.cpp, $(OBJ_DIR)%.d, $(CC_FILES_IN))

SC_V_FILES_IN	:= $(wildcard $(SHR_DIR)*.vert)
SC_F_FILES_IN	:= $(wildcard $(SHR_DIR)*.frag)
SC_FILES_OUT	:= $(patsubst $(SHR_DIR)%.vert, $(BIN_DIR)%_vert.spv, $(SC_V_FILES_IN)) $(patsubst $(SHR_DIR)%.frag, $(BIN_DIR)%_frag.spv, $(SC_F_FILES_IN))

EXE_OUT			:= $(BIN_DIR)planetarium

.PHONY: clean $(BIN_DIR) $(OBJ_DIR)

all: execute

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling" $< to $@
	@$(CC) $(CC_FLAGS) $(CC_INCLUDE) $(DEP_FLAGS) -c $< -o $@

-include $(CC_FILES_DEP)

$(BIN_DIR)%_vert.spv: $(SHR_DIR)%.vert
	@mkdir -p $(BIN_DIR)
	@echo "Compiling vertex shader" $<
	@$(SC) $< -o $@

$(BIN_DIR)%_frag.spv: $(SHR_DIR)%.frag
	@mkdir -p $(BIN_DIR)
	@echo "Compiling fragment shader" $<
	@$(SC) $< -o $@

shaders: $(SC_FILES_OUT)
	@cp $(SC_FILES_OUT) .

$(EXE_OUT): $(CC_FILES_OUT)
	@echo "Linking" $(EXE_OUT)
	@$(LD) $(LD_FLAGS) -o $@ $(CC_FILES_OUT) $(LD_INCLUDE)

build: $(EXE_OUT)

execute: $(EXE_OUT) shaders
	@$(EXE_OUT)

clean:
	@rm -r $(BIN_DIR)
	@rm *.spv
