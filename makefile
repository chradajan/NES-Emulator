COMPILER_FLAGS = -std=c++17 -O2 -Wall -Wextra -Wl,-subsystem,windows
OUT_DIR = ./bin
SRC_DIR = ./src
MAPPER_DIR = ./src/mappers
SDL_INCLUDE = ./library/SDL2/include/SDL2
SDL_LIBRARY = ./library/SDL2/lib
LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2
LOGGING_FLAGS = -DLOGGING
IMGUI = ./library/imgui
MD5 = ./library/md5

main: ./src/*.cpp
	g++ $(COMPILER_FLAGS) $(SRC_DIR)/*.cpp $(MAPPER_DIR)/*.cpp $(IMGUI)/*.cpp $(MD5)/*.cpp -I$(SDL_INCLUDE) -L$(SDL_LIBRARY) $(LINKER_FLAGS) -o $(OUT_DIR)/NES_EMU
logging:
	g++ $(COMPILER_FLAGS) $(LOGGING_FLAGS) $(SRC_DIR)/*.cpp $(MAPPER_DIR)/*.cpp -I$(SDL_INCLUDE) -L$(SDL_LIBRARY) $(LINKER_FLAGS) -o $(OUT_DIR)/NES_EMU
clean:
	cd bin && rm -f NES_EMU
run:
	cd bin && ./NES_EMU