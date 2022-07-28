CXXFLAGS = -ggdb -std=c++17 -Wall -Wextra
OUT_DIR = ./bin
SRC_DIR = ./src
MAPPER_DIR = ./src/mappers

main: ./src/*.cpp
	g++ $(CXXFLAGS) $(SRC_DIR)/*.cpp $(MAPPER_DIR)/*.cpp -o $(OUT_DIR)/main
clean:
	cd bin && rm -f main
run:
	cd bin && ./main