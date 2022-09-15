#include "../../include/mappers/AxROM.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>

AxROM::AxROM(std::ifstream& rom, std::array<uint8_t, 16> const& header)
{
    LoadROM(rom, header[4], header[5]);
    prgIndex_ = 0;
    mirrorType_ = MirrorType::SINGLE_LOW;
}

void AxROM::Reset()
{
    prgIndex_ = 0;
    mirrorType_ = MirrorType::SINGLE_LOW;
}

uint8_t AxROM::ReadPRG(uint16_t addr)
{
    if (addr < 0x8000)
    {
        return 0x00;
    }

    return PRG_ROM_BANKS_[prgIndex_][addr - 0x8000];
}

void AxROM::WritePRG(uint16_t addr, uint8_t data)
{
    if (addr >= 0x8000)
    {
        prgIndex_ = data & AXROM_BANK_SELECT_MASK;

        if ((data & NAMETABLE_MIRRORING_MASK) == NAMETABLE_MIRRORING_MASK)
        {
            mirrorType_ = MirrorType::SINGLE_HIGH;
        }
        else
        {
            mirrorType_ = MirrorType::SINGLE_LOW;
        }
    }
}

uint8_t AxROM::ReadCHR(uint16_t addr)
{
    return CHR_ROM_[addr];
}

void AxROM::WriteCHR(uint16_t addr, uint8_t data)
{
    if (chrRamMode_)
    {
        CHR_ROM_[addr] = data;
    }
}

void AxROM::SaveRAM()
{

}

bool AxROM::IRQ()
{
    return false;
}

void AxROM::LoadROM(std::ifstream& rom, size_t prgRomBanks, size_t chrRomBanks)
{
    prgRomBanks *= 2;
    chrRamMode_ = (chrRomBanks == 0);
    PRG_ROM_BANKS_.resize(prgRomBanks);

    for (size_t bankIndex = 0; bankIndex < prgRomBanks; ++bankIndex)
    {
        for (size_t prgAddr = 0x0000; prgAddr < 0x8000; ++prgAddr)
        {
            rom >> std::noskipws >> std::hex >> PRG_ROM_BANKS_[bankIndex][prgAddr];
        }
    }

    if (!chrRamMode_)
    {
        for (size_t chrAddr = 0x0000; chrAddr < 0x2000; ++chrAddr)
        {
            rom >> std::noskipws >> std::hex >> CHR_ROM_[chrAddr];
        }
    }
}
