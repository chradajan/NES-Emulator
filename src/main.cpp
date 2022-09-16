#include "../include/APU.hpp"
#include "../include/Cartridge.hpp"
#include "../include/CPU.hpp"
#include "../include/Controller.hpp"
#include "../include/GameWindow.hpp"
#include "../include/NES.hpp"
#include "../include/PPU.hpp"
#include <array>
#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    if (argc <= 1)
    {
        std::cout << "Failed to specify path to ROM.\n";
        return 0;
    }

    std::filesystem::path romPath = argv[1];
    std::filesystem::path savePath = std::filesystem::path("../saves/");
    std::filesystem::path saveStatePath = std::filesystem::path("../savestates/");
    std::filesystem::path logPath = std::filesystem::path("../logs/");

    if (!std::filesystem::is_directory(savePath))
    {
        std::filesystem::create_directory(savePath);
    }

    if (!std::filesystem::is_directory(saveStatePath))
    {
        std::filesystem::create_directory(saveStatePath);
    }

    if (!std::filesystem::is_directory(logPath))
    {
        std::filesystem::create_directory(logPath);
    }

    savePath += romPath.filename();
    savePath.replace_extension(".sav");

    std::array<char, 256 * 240 * 3> frameBuffer;

    NES nes(romPath.string(),
            savePath.string(),
            frameBuffer.data());

    if (nes.Ready())
    {
        GameWindow gameWindow(nes, frameBuffer.data(), romPath.stem().string());
        gameWindow.Run();
    }
    else
    {
        std::cout << "Could not load selected ROM.\n";
    }

    return 0;
}
