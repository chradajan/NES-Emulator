
#include "../../include/mappers/CNROM.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>

CNROM::CNROM(std::ifstream& rom, std::array<uint8_t, 16> const& header)
{
    if ((header[6] & VERTICAL_MIRRORING_FLAG) == VERTICAL_MIRRORING_FLAG)
    {
        mirrorType_ = MirrorType::VERTICAL;
    }
    else
    {
        mirrorType_ = MirrorType::HORIZONTAL;
    }

    LoadROM(rom, header[4], header[5]);
    chrIndex_ = 0;
}

void CNROM::Reset()
{
    chrIndex_ = 0;
}

uint8_t CNROM::ReadPRG(uint16_t addr)
{
    if (addr < 0x8000)
    {
        return 0x00;
    }

    return PRG_ROM_[addr - 0x8000];
}

void CNROM::WritePRG(uint16_t addr, uint8_t data)
{
    (void)addr;
    chrIndex_ = data & 0x03;
}

uint8_t CNROM::ReadCHR(uint16_t addr)
{
    return CHR_ROM_[chrIndex_][addr];
}

void CNROM::WriteCHR(uint16_t addr, uint8_t data)
{
    (void)addr;
    (void)data;
}

void CNROM::SaveRAM()
{

}

bool CNROM::IRQ()
{
    return false;
}

void CNROM::Serialize(std::ofstream& saveState)
{
    saveState.write((char*)&chrIndex_, sizeof(chrIndex_));
}

void CNROM::Deserialize(std::ifstream& saveState)
{
    saveState.read((char*)&chrIndex_, sizeof(chrIndex_));
}

void CNROM::LoadROM(std::ifstream& rom, size_t prgRomBanks, size_t chrRomBanks)
{
    for (size_t prgIndex = 0x0000; prgIndex < 0x4000; ++prgIndex)
    {
        rom >> std::noskipws >> std::hex >> PRG_ROM_[prgIndex];
    }

    if (prgRomBanks == 1)
    {
        std::copy(PRG_ROM_.begin(), PRG_ROM_.begin() + 0x4000, PRG_ROM_.begin() + 0x4000);
    }
    else
    {
        for (size_t prgIndex = 0x4000; prgIndex < 0x8000; ++prgIndex)
        {
            rom >> std::noskipws >> std::hex >> PRG_ROM_[prgIndex];
        }
    }

    CHR_ROM_.resize(chrRomBanks);

    for (size_t bankIndex = 0; bankIndex < chrRomBanks; ++bankIndex)
    {
        for (size_t chrIndex = 0x0000; chrIndex < 0x2000; ++chrIndex)
        {
            rom >> std::noskipws >> std::hex >> CHR_ROM_[bankIndex][chrIndex];
        }
    }
}
