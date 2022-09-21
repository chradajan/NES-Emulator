#include "../include/NES.hpp"
#include "../include/APU.hpp"
#include "../include/Cartridge.hpp"
#include "../include/CPU.hpp"
#include "../include/Controller.hpp"
#include "../include/DmcChannel.hpp"
#include "../include/NoiseChannel.hpp"
#include "../include/PulseChannel.hpp"
#include "../include/TriangleChannel.hpp"
#include "../include/mappers/AxROM.hpp"
#include "../include/mappers/CNROM.hpp"
#include "../include/mappers/MMC1.hpp"
#include "../include/mappers/MMC2.hpp"
#include "../include/mappers/MMC3.hpp"
#include "../include/mappers/NROM.hpp"
#include "../include/mappers/UxROM.hpp"
#include "../include/PPU.hpp"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

NES::NES(uint8_t* frameBuffer, std::ifstream& normalColors, std::ifstream& grayscaleColors)
{
    apu_ = std::make_unique<APU>();
    controller_ = std::make_unique<Controller>();
    ppu_ = std::make_unique<PPU>(frameBuffer, normalColors, grayscaleColors);
    cpu_ = std::make_unique<CPU>(*apu_, *controller_, *ppu_);
    cartridge_ = nullptr;
    cartLoaded_ = false;
    fileName_ = "";
}

NES::~NES()
{
    if (cartLoaded_)
    {
        cartridge_->SaveRAM();
    }
}

void NES::SetControllerInputs(uint8_t controller1, uint8_t controller2)
{
    controller_->SetControllerInputs(controller1, controller2);
}

void NES::Reset()
{
    cpu_->Reset();
    ppu_->Reset();
    apu_->Reset();

    if (cartLoaded_)
    {
        cartridge_->Reset();
    }
}

bool NES::Ready()
{
    return cartLoaded_;
}

bool NES::FrameReady()
{
    return ppu_->FrameReady();
}

int16_t NES::GetAudioSample()
{
    return apu_->GetSample();
}

std::string NES::GetFileName()
{
    return fileName_;
}

void NES::LoadCartridge(std::filesystem::path romPath, std::filesystem::path savePath)
{
    if (cartLoaded_)
    {
        cartridge_->SaveRAM();
        cpu_->Reset();
        ppu_->Reset();
        apu_->Reset();
    }

    InitializeCartridge(romPath.string(), savePath.string());

    if (cartLoaded_)
    {
        fileName_ = romPath.stem().string();
        cpu_->LoadCartridge(cartridge_.get());
        ppu_->LoadCartridge(cartridge_.get());
    }
}

void NES::Clock()
{
    if (cartLoaded_)
    {
        ppu_->Clock();
        apu_->Clock();
        cpu_->Clock();
    }
}

void NES::RunUntilFrameReady()
{
    if (cartLoaded_)
    {
        while(!ppu_->FrameReady())
        {
            ppu_->Clock();
            apu_->Clock();
            cpu_->Clock();
        }
    }
}

void NES::RunUntilSerializable()
{
    if (cartLoaded_)
    {
        while (!(cpu_->Serializable() && ppu_->Serializable()))
        {
            ppu_->Clock();
            apu_->Clock();
            cpu_->Clock();
        }
    }
}

void NES::Serialize(std::ofstream& saveState)
{
    if (cartLoaded_)
    {
        cpu_->Serialize(saveState);
        ppu_->Serialize(saveState);
        cartridge_->Serialize(saveState);
        apu_->Serialize(saveState);
    }
}

void NES::Deserialize(std::ifstream& saveState)
{
    if (cartLoaded_)
    {
        cpu_->Deserialize(saveState);
        ppu_->Deserialize(saveState);
        cartridge_->Deserialize(saveState);
        apu_->Deserialize(saveState);
    }
}

void NES::InitializeCartridge(std::string const romPath, std::string const savePath)
{
    cartridge_.reset();
    std::ifstream rom(romPath, std::ios::binary);
    std::array<uint8_t, 0x10> header;
    rom.read((char*)header.data(), 16);
    uint16_t mapper = (header[7] & UPPER_MAPPER_NYBBLE) | (header[6] >> 4);

    if ((header[0] != 0x4E) || (header[1] != 0x45) || (header[2] != 0x53) || (header[3] != 0x1A))
    {
        cartLoaded_ = false;
        return;
    }

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
