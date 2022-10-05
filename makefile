COMPILER_FLAGS = -std=c++17 -Wall -Wextra -O2 -Wl,-subsystem,windows
SRC_DIRS = ./src/*.cpp ./src/mappers/*.cpp ./library/md5/*.cpp ./library/DearImGui/*.cpp
LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image
LOGGING_FLAGS = -DLOGGING
RESOURCES = ./resources/resources.res

APPLE_COMPILER_FLAGS = -std=c++17 -Wall -Wextra -O2
APPLE_LINKER_FLAGS = -lSDL2main -lSDL2 -lSDL2_image
APPLE_INCLUDE_PATHS = -I/Library/Frameworks/SDL2.framework/Headers -I/Library/Frameworks/SDL2_image.framework/Headers

main: ./src/*.cpp
	g++ $(COMPILER_FLAGS) $(SRC_DIRS) $(LINKER_FLAGS) -o NES_EMU $(RESOURCES)
apple:
	g++ $(APPLE_COMPILER_FLAGS) -framework SDL2 -framework SDL2_image -F /Library/Frameworks $(SRC_DIRS) $(APPLE_INCLUDE_PATHS) -o NES_EMU
release:
	g++ $(COMPILER_FLAGS) -static-libgcc -static-libstdc++ $(SRC_DIRS) $(LINKER_FLAGS) -o NES_EMU $(RESOURCES)
logging:
	g++ $(COMPILER_FLAGS) $(LOGGING_FLAGS) $(SRC_DIRS) -I$(IMGUI_INCLUDE) -L$(IMGUI_LIBRARY) $(LINKER_FLAGS) -o NES_EMU $(RESOURCES)
resource:
	windres ./resources/resources.rc -O coff ./resources/resources.res