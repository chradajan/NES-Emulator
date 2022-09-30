COMPILER_FLAGS = -std=c++17 -Wall -Wextra -O2 -Wl,-subsystem,windows
SRC_DIRS = ./src/*.cpp ./src/mappers/*.cpp ./library/md5/*.cpp
LINKER_FLAGS = -lmingw32 -lDearImGui -lSDL2main -lSDL2 -lSDL2_image
LOGGING_FLAGS = -DLOGGING
RESOURCES = ./resources/resources.res
INCLUDE_PATHS = -I./library/DearImGui/include -I./library/SDL2/include -I./library/SDL2_Image/include
LIBRARY_PATHS = -L./library/DearImGui/lib -L./library/SDL2/lib -L./library/SDL2_Image/lib

main: ./src/*.cpp
	g++ $(COMPILER_FLAGS) $(SRC_DIRS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(LINKER_FLAGS) -o NES_EMU $(RESOURCES)
release:
	g++ $(COMPILER_FLAGS) -static-libgcc -static-libstdc++ $(SRC_DIRS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(LINKER_FLAGS) -o NES_EMU $(RESOURCES)
logging:
	g++ $(COMPILER_FLAGS) $(LOGGING_FLAGS) $(SRC_DIRS) -I$(IMGUI_INCLUDE) -L$(IMGUI_LIBRARY) $(LINKER_FLAGS) -o NES_EMU $(RESOURCES)
resource:
	windres ./resources/resources.rc -O coff ./resources/resources.res