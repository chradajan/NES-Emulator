COMPILER_FLAGS = -std=c++17 -O2 -Wall -Wextra #-Wl,-subsystem,windows
OUT_DIR = ./bin
SRC_DIR = ./src
MAPPER_DIR = ./src/mappers
SDL_INCLUDE = ./SDL2/include
SDL_LIBRARY = ./SDL2/lib
LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2
LOGGING_FLAGS = -DLOGGING

main: ./src/*.cpp
	g++ $(COMPILER_FLAGS) $(SRC_DIR)/*.cpp $(MAPPER_DIR)/*.cpp -I$(SDL_INCLUDE) -L$(SDL_LIBRARY) $(LINKER_FLAGS) -o $(OUT_DIR)/main
logging:
	g++ $(COMPILER_FLAGS) $(LOGGING_FLAGS) $(SRC_DIR)/*.cpp $(MAPPER_DIR)/*.cpp -I$(SDL_INCLUDE) -L$(SDL_LIBRARY) $(LINKER_FLAGS) -o $(OUT_DIR)/main
clean:
	cd bin && rm -f main
run:
	cd bin && ./main