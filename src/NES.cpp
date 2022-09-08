#include "../include/NES.hpp"
#include "../include/APU.hpp"
#include "../include/Cartridge.hpp"
#include "../include/CPU.hpp"
#include "../include/Controller.hpp"
#include "../include/mappers/AxROM.hpp"
#include "../include/mappers/MMC1.hpp"
#include "../include/mappers/MMC3.hpp"
#include "../include/mappers/NROM.hpp"
#include "../include/PPU.hpp"
#include <cstdint>
#include <iomanip>
#include <fstream>
#include <memory>

NES::NES(std::string const romPath, std::string const savePath, char* frameBuffer)
{
    InitializeCartridge(romPath, savePath);
    apu = std::make_unique<APU>();
    controller = std::make_unique<Controller>();
    ppu = std::make_unique<PPU>(*cartridge, frameBuffer);
    cpu = std::make_unique<CPU>(*apu, *cartridge, *controller, *ppu);
}

NES::~NES()
{
    cartridge->SaveRAM();
}

void NES::Run()
{
    while (!ppu->FrameReady())
    {
        ppu->Clock();
        ppu->Clock();
        ppu->Clock();
        cpu->Clock();
    }
}

void NES::InitializeCartridge(std::string const romPath, std::string const savePath)
{
    std::ifstream rom(romPath, std::ios::binary);
    std::array<uint8_t, 0x10> header;

    for (size_t i = 0x00; i < 0x10; ++i)
    {
        rom >> std::noskipws >> std::hex >> header[i];
    }

    uint16_t mapper = (header[7] & UPPER_MAPPER_NYBBLE) | (header[6] >> 4);

    switch (mapper)
    {
        case 0:
            cartridge = std::make_unique<NROM>(rom, header);
            break;
        case 1:
            cartridge = std::make_unique<MMC1>(rom, savePath, header);
            break;
        case 4:
            cartridge = std::make_unique<MMC3>(rom, savePath, header);
            break;
        case 7:
            cartridge = std::make_unique<AxROM>(rom, header);
            break;
        default:
            cartridge = nullptr;
    }
}