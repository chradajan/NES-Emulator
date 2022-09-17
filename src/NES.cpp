#include "../include/NES.hpp"
#include "../include/APU.hpp"
#include "../include/Cartridge.hpp"
#include "../include/CPU.hpp"
#include "../include/Controller.hpp"
#include "../include/mappers/AxROM.hpp"
#include "../include/mappers/CNROM.hpp"
#include "../include/mappers/MMC1.hpp"
#include "../include/mappers/MMC2.hpp"
#include "../include/mappers/MMC3.hpp"
#include "../include/mappers/NROM.hpp"
#include "../include/mappers/UxROM.hpp"
#include "../include/PPU.hpp"
#include <cstdint>
#include <iomanip>
#include <fstream>
#include <memory>

NES::NES(std::string const romPath, std::string const savePath, char* frameBuffer)
{
    InitializeCartridge(romPath, savePath);

    if (cartLoaded_)
    {
        apu_ = std::make_unique<APU>();
        controller_ = std::make_unique<Controller>();
        ppu_ = std::make_unique<PPU>(*cartridge_, frameBuffer);
        cpu_ = std::make_unique<CPU>(*apu_, *cartridge_, *controller_, *ppu_);
    }
}

NES::~NES()
{
    cartridge_->SaveRAM();
}

void NES::Run()
{
    while (!ppu_->FrameReady())
    {
        ppu_->Clock();
        cpu_->Clock();
    }
}

void NES::Reset()
{
    cpu_->Reset();
    ppu_->Reset();
    cartridge_->Reset();
}

bool NES::Ready()
{
    return cartLoaded_;
}

void NES::RunUntilSerializable()
{
    while (!(cpu_->Serializable() && ppu_->Serializable()))
    {
        ppu_->Clock();
        cpu_->Clock();
    }
}

void NES::Serialize(std::ofstream& saveState)
{
    cpu_->Serialize(saveState);
    ppu_->Serialize(saveState);
    cartridge_->Serialize(saveState);
}

void NES::Deserialize(std::ifstream& saveState)
{
    cpu_->Deserialize(saveState);
    ppu_->Deserialize(saveState);
    cartridge_->Deserialize(saveState);
}

void NES::InitializeCartridge(std::string const romPath, std::string const savePath)
{
    std::ifstream rom(romPath, std::ios::binary);
    std::array<uint8_t, 0x10> header;
    rom.read((char*)header.data(), 16);
    uint16_t mapper = (header[7] & UPPER_MAPPER_NYBBLE) | (header[6] >> 4);

    switch (mapper)
    {
        case 0:
            cartridge_ = std::make_unique<NROM>(rom, header);
            break;
        case 1:
            cartridge_ = std::make_unique<MMC1>(rom, savePath, header);
            break;
        case 2:
            cartridge_ = std::make_unique<UxROM>(rom, header);
            break;
        case 3:
            cartridge_ = std::make_unique<CNROM>(rom, header);
            break;
        case 4:
            cartridge_ = std::make_unique<MMC3>(rom, savePath, header);
            break;
        case 7:
            cartridge_ = std::make_unique<AxROM>(rom, header);
            break;
        case 9:
            cartridge_ = std::make_unique<MMC2>(rom, header);
            break;
        default:
            cartLoaded_ = false;
            return;
    }

    cartLoaded_ = true;
}
