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

int main(int argc, char** argv)
{
    if (argc <= 1)
    {
        std::cout << "Failed to specify path to ROM." << std::endl;
        return 0;
    }

    std::string romPath = argv[1];
    std::string fileNameWithExtension = romPath.substr(romPath.find_last_of("/\\") + 1);
    std::string fileName = fileNameWithExtension.substr(0, fileNameWithExtension.find_last_of('.'));
    std::string savePath = "../saves/" + fileName + ".sav";

    std::array<char, 256 * 240 * 3> frameBuffer;

    NES nes(romPath,
            savePath,
            frameBuffer.data());

    GameWindow gameWindow(nes, frameBuffer.data());
    gameWindow.Run();
    return 0;
}