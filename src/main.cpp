#include "../include/GameWindow.hpp"
#include "../include/NES.hpp"
#include "../include/Paths.hpp"
#include <array>
#include <filesystem>
#include <fstream>

int main(int argc, char** argv)
{
    if (!std::filesystem::is_directory(SAVE_PATH))
    {
        std::filesystem::create_directory(SAVE_PATH);
    }

    if (!std::filesystem::is_directory(SAVE_STATE_PATH))
    {
        std::filesystem::create_directory(SAVE_STATE_PATH);
    }

    if (!std::filesystem::is_directory(LOG_PATH))
    {
        std::filesystem::create_directory(LOG_PATH);
    }

    std::array<uint8_t, SCREEN_WIDTH * SCREEN_HEIGHT * CHANNELS> frameBuffer;
    frameBuffer.fill(0x00);

    std::ifstream normalColors(PALETTE_PATH.string() + "ntsc_normal.pal", std::ios::binary);
    std::ifstream grayscaleColors(PALETTE_PATH.string() + "ntsc_grayscale.pal", std::ios::binary);

    NES nes(frameBuffer.data(), normalColors, grayscaleColors);
    std::filesystem::path romPath = "";

    if (argc > 1)
    {
        romPath = argv[1];
    }

    GameWindow gameWindow(nes, frameBuffer.data(), romPath);
    gameWindow.Run();

    return 0;
}
