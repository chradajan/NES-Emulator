#include "../include/APU.hpp"
#include "../include/Cartridge.hpp"
#include "../include/CPU.hpp"
#include "../include/Controller.hpp"
#include "../include/GameWindow.hpp"
#include "../include/NES.hpp"
#include "../include/PPU.hpp"
#include <SDL2/SDL.h>
#include <iostream>
#include <string>

int main(int, char**)
{
    std::string path = "../roms/";
    std::string extension = ".nes";
    std::string game;

    std::cin >> game;

    GameWindow window(path + game + extension, game);
    window.StartEmulator();
    return 0;
}