#include "../../include/mappers/AxROM.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <vector>

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

void AxROM::Serialize(std::ofstream& saveState)
{
    saveState.write((char*)&prgIndex_, sizeof(prgIndex_));

    uint16_t byteExpander = static_cast<uint16_t>(mirrorType_);
    saveState.write((char*)&byteExpander, sizeof(mirrorType_));

    if (chrRamMode_)
    {
        saveState.write((char*)CHR_ROM_.data(), 0x2000);
    }
}

void AxROM::Deserialize(std::ifstream& saveState)
{
    saveState.read((char*)&prgIndex_, sizeof(prgIndex_));
    saveState.read((char*)&mirrorType_, sizeof(mirrorType_));

    if (chrRamMode_)
    {
        saveState.read((char*)CHR_ROM_.data(), 0x2000);
    }
}

void AxROM::LoadROM(std::ifstream& rom, size_t prgRomBanks, size_t chrRomBanks)
{
    prgRomBanks *= 2;
    chrRamMode_ = (chrRomBanks == 0);
    PRG_ROM_BANKS_.resize(prgRomBanks);

    for (size_t bankIndex = 0; bankIndex < prgRomBanks; ++bankIndex)
    {
        rom.read((char*)PRG_ROM_BANKS_[bankIndex].data(), 0x8000);
    }

    if (!chrRamMode_)
    {
        rom.read((char*)CHR_ROM_.data(), 0x2000);
    }
}
