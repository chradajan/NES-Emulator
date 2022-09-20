#include "../include/GameWindow.hpp"
#include "../include/NES.hpp"
#include <array>
#include <filesystem>
#include <fstream>

int main(int argc, char** argv)
{
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

    std::array<uint8_t, SCREEN_WIDTH * SCREEN_HEIGHT * CHANNELS> frameBuffer;
    frameBuffer.fill(0x00);

    std::ifstream normalColors("../palettes/ntsc_normal.pal", std::ios::binary);
    std::ifstream grayscaleColors("../palettes/ntsc_grayscale.pal", std::ios::binary);

    NES nes(frameBuffer.data(), normalColors, grayscaleColors);

    if (argc > 1)
    {
        std::filesystem::path romPath = argv[1];
        savePath += romPath.filename();
        savePath.replace_extension(".sav");
        nes.LoadCartridge(romPath, savePath);
    }

    GameWindow gameWindow(nes, frameBuffer.data());
    gameWindow.Run();

    return 0;
}
