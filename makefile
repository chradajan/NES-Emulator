COMPILER_FLAGS = -std=c++17 -O2 -Wall -Wextra -Wl,-subsystem,windows
SRC_DIRS = ./src/*.cpp ./src/mappers/*.cpp ./library/imgui/*.cpp ./library/md5/*.cpp
SDL_INCLUDE = ./library/SDL2/include/SDL2
SDL_LIBRARY = ./library/SDL2/lib
SDL_IMAGE_INCLUDE = ./library/SDL2_Image/include/SDL2
SDL_IMAGE_LIBRARY = ./library/SDL2_Image/lib
LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image
LOGGING_FLAGS = -DLOGGING
RESOURCES = ./resources/resources.res

main: ./src/*.cpp
	g++ $(COMPILER_FLAGS) $(SRC_DIRS) -I$(SDL_INCLUDE) -L$(SDL_LIBRARY) $(LINKER_FLAGS) -I$(SDL_IMAGE_INCLUDE) -L$(SDL_IMAGE_LIBRARY) -o NES_EMU $(RESOURCES)
logging:
	g++ $(COMPILER_FLAGS) $(LOGGING_FLAGS) $(SRC_DIRS) -I$(SDL_INCLUDE) -L$(SDL_LIBRARY) $(LINKER_FLAGS) -I$(SDL_IMAGE_INCLUDE) -L$(SDL_IMAGE_LIBRARY) -o NES_EMU $(RESOURCES)
resource:
	windres ./resources/resources.rc -O coff ./resources/resources.res