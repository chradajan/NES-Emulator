#include "../../include/mappers/UxROM.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <vector>

UxROM::UxROM(std::ifstream& rom, std::array<uint8_t, 16> const& header)
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
    prgIndex0 = 0;
    prgIndex1 = (PRG_ROM_BANKS_.size() - 1);
}

void UxROM::Reset()
{
    prgIndex0 = 0;
}

uint8_t UxROM::ReadPRG(uint16_t addr)
{
    if (addr < 0x8000)
    {
        return 0x00;
    }
    else if (addr < 0xC000)
    {
        return PRG_ROM_BANKS_[prgIndex0][addr - 0x8000];
    }
    else
    {
        return PRG_ROM_BANKS_[prgIndex1][addr - 0xC000];
    }
}

void UxROM::WritePRG(uint16_t addr, uint8_t data)
{
    (void)addr;
    prgIndex0 = data & UXROM_BANK_SELECT_MASK;
}

uint8_t UxROM::ReadCHR(uint16_t addr)
{
    return CHR_ROM_[addr];
}

void UxROM::WriteCHR(uint16_t addr, uint8_t data)
{
    if (chrRamMode_)
    {
        CHR_ROM_[addr] = data;
    }
}

void UxROM::SaveRAM()
{

}

bool UxROM::IRQ()
{
    return false;
}

void UxROM::Serialize(std::ofstream& saveState)
{
    if (chrRamMode_)
    {
        saveState.write((char*)CHR_ROM_.data(), 0x2000);
    }

    saveState.write((char*)&prgIndex0, sizeof(prgIndex0));
    saveState.write((char*)&prgIndex1, sizeof(prgIndex1));
}

void UxROM::Deserialize(std::ifstream& saveState)
{
    if (chrRamMode_)
    {
        saveState.read((char*)CHR_ROM_.data(), 0x2000);
    }

    saveState.read((char*)&prgIndex0, sizeof(prgIndex0));
    saveState.read((char*)&prgIndex1, sizeof(prgIndex1));
}

void UxROM::LoadROM(std::ifstream& rom, size_t prgRomBanks, size_t chrRomBanks)
{
    PRG_ROM_BANKS_.resize(prgRomBanks);
    chrRamMode_ = (chrRomBanks == 0);

    for (size_t bankIndex = 0; bankIndex < prgRomBanks; ++bankIndex)
    {
        for (size_t prgIndex = 0x0000; prgIndex < 0x4000; ++prgIndex)
        {
            rom >> std::noskipws >> std::hex >> PRG_ROM_BANKS_[bankIndex][prgIndex];
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
