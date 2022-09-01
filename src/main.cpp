#include "../include/APU.hpp"
#include "../include/Cartridge.hpp"
#include "../include/CPU.hpp"
#include "../include/Controller.hpp"
#include "../include/GameWindow.hpp"
#include "../include/NES.hpp"
#include "../include/PPU.hpp"
#include <array>
#include <iostream>
#include <string>

int main(int, char**)
{
    std::string rom_path = "../roms/";
    std::string save_path = "../saves/";
    std::string rom_extension = ".nes";
    std::string save_extension = ".sav";

    std::string game;
    std::cin >> game;

    std::array<char, 256 * 240 * 3> frameBuffer;

    NES nes(rom_path + game + rom_extension,
            save_path + game + save_extension,
            frameBuffer.data());

    GameWindow gameWindow(nes, frameBuffer.data());
    gameWindow.Run();
    return 0;
}