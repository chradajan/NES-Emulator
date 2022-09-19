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
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <string>

NES::NES(char* frameBuffer, float* audioBuffer)
{
    apu_ = std::make_unique<APU>();
    controller_ = std::make_unique<Controller>();
    ppu_ = std::make_unique<PPU>(frameBuffer);
    cpu_ = std::make_unique<CPU>(*apu_, *controller_, *ppu_);
    cartridge_ = nullptr;
    cartLoaded_ = false;
    fileName_ = "";
    apuOutputTimer_ = 0;
    bufferIndex_ = 0;
    audioBuffer_ = audioBuffer;
}

NES::~NES()
{
    if (cartLoaded_)
    {
        cartridge_->SaveRAM();
    }
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
    }

    InitializeCartridge(romPath.string(), savePath.string());

    if (cartLoaded_)
    {
        fileName_ = romPath.stem().string();
        cpu_->LoadCartridge(cartridge_.get());
        ppu_->LoadCartridge(cartridge_.get());
    }
}

void NES::Run(std::function<void()>& playAudio)
{
    if (cartLoaded_)
    {
        while (!ppu_->FrameReady())
        {
            ppu_->Clock();
            cpu_->Clock();
            apu_->Clock();
            ++apuOutputTimer_;

            if (apuOutputTimer_ == 41)
            {
                audioBuffer_[bufferIndex_++] = apu_->Output();
                apuOutputTimer_ = 0;

                if (bufferIndex_ == AUDIO_SAMPLE_BUFFER_COUNT)
                {
                    bufferIndex_ = 0;
                    playAudio();
                }
            }
        }
    }
}

void NES::Reset()
{
    cpu_->Reset();
    ppu_->Reset();

    if (cartLoaded_)
    {
        cartridge_->Reset();
    }
}

bool NES::Ready()
{
    return cartLoaded_;
}

void NES::RunUntilSerializable(std::function<void()>& playAudio)
{
    if (cartLoaded_)
    {
        while (!(cpu_->Serializable() && ppu_->Serializable()))
        {
            ppu_->Clock();
            cpu_->Clock();
            apu_->Clock();
            ++apuOutputTimer_;

            if (apuOutputTimer_ == 41)
            {
                audioBuffer_[bufferIndex_++] = apu_->Output();
                apuOutputTimer_ = 0;

                if (bufferIndex_ == AUDIO_SAMPLE_BUFFER_COUNT)
                {
                    bufferIndex_ = 0;
                    playAudio();
                }
            }
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
    }
}

void NES::Deserialize(std::ifstream& saveState)
{
    if (cartLoaded_)
    {
        cpu_->Deserialize(saveState);
        ppu_->Deserialize(saveState);
        cartridge_->Deserialize(saveState);
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
