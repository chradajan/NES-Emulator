#include "../include/NES.hpp"
#include "../include/APU.hpp"
#include "../include/Cartridge.hpp"
#include "../include/CPU.hpp"
#include "../include/Controller.hpp"
#include "../include/mappers/MMC1.hpp"
#include "../include/mappers/NROM.hpp"
#include "../include/PPU.hpp"
#include <cstdint>
#include <iomanip>
#include <fstream>
#include <memory>

NES::NES(std::string const romPath)
{
    InitializeCartridge(romPath);
    apu = std::make_unique<APU>();
    controller = std::make_unique<Controller>();
    ppu = std::make_unique<PPU>(*cartridge);
    cpu = std::make_unique<CPU>(*apu, *cartridge, *controller, *ppu);
}

void NES::Run()
{
    while (!ppu->FrameReady())
    {
        ppu->Tick();
        ppu->Tick();
        ppu->Tick();
        cpu->Tick();
    }
}

char* NES::GetFrameBuffer()
{
    return ppu->GetFrameBuffer();
}

void NES::InitializeCartridge(std::string const romPath)
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
            cartridge = std::make_unique<MMC1>(rom, header);
            break;
        default:
            cartridge = nullptr;
    }
}