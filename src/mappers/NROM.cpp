#include "../../include/mappers/NROM.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>

NROM::NROM(std::ifstream& rom, std::array<uint8_t, 16> const& header)
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
}

uint8_t NROM::ReadPRG(uint16_t addr)
{
    if (addr < 0x8000)
    {
        return 0x00;
    }

    return PRG_ROM_[addr - 0x8000];
}

void NROM::WritePRG(uint16_t addr, uint8_t data)
{
    (void)addr;
    (void)data;
}

uint8_t NROM::ReadCHR(uint16_t addr)
{
    return CHR_ROM_[addr];
}

void NROM::WriteCHR(uint16_t addr, uint8_t data)
{
    if (chrRamMode_)
    {
        CHR_ROM_[addr] = data;
    }
}

uint16_t NROM::NameTableAddress(uint16_t addr)
{
    addr &= 0x3FFF;

    if (addr > 0x2FFF)
    {
        addr -= 0x1000;
    }

    switch (mirrorType_)
    {
        case MirrorType::HORIZONTAL:
            addr = (addr - 0x2000) - (addr / 0x2400 * 0x0400) - (addr / 0x2C00 * 0x0400);
            break;
        case MirrorType::VERTICAL:
            addr = (addr - 0x2000) - (addr / 0x2800 * 0x0800);
            break;
        default:
            break;
    }

    return addr;
}

void NROM::SaveRAM()
{

}

bool NROM::IRQ()
{
    return false;
}

void NROM::LoadROM(std::ifstream& rom, size_t prgRomBanks, size_t chrRomBanks)
{
    chrRamMode_ = (chrRomBanks == 0);

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

    if (!chrRamMode_)
    {
        for (size_t chrIndex = 0x0000; chrIndex < 0x2000; ++chrIndex)
        {
            rom >> std::noskipws >> std::hex >> CHR_ROM_[chrIndex];
        }
    }
}
