#include "../include/APU.hpp"
#include "../include/Cartridge.hpp"
#include "../include/CPU.hpp"
#include "../include/Controller.hpp"
#include "../include/GameWindow.hpp"
#include "../include/NES.hpp"
#include "../include/PPU.hpp"
#include <array>
#include <filesystem>
#include <string>

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

    std::array<char, 256 * 240 * 3> frameBuffer;
    frameBuffer.fill(0x00);

    std::array<float, AUDIO_SAMPLE_BUFFER_COUNT> audioBuffer;

    NES nes(frameBuffer.data(), audioBuffer.data());

    if (argc > 1)
    {
        std::filesystem::path romPath = argv[1];
        savePath += romPath.filename();
        savePath.replace_extension(".sav");
        nes.LoadCartridge(romPath, savePath);
    }

    GameWindow gameWindow(nes, frameBuffer.data(), audioBuffer.data());
    gameWindow.Run();

    return 0;
}
