#include "../../include/mappers/NROM.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>

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

void NROM::Reset()
{

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

void NROM::SaveRAM()
{

}

bool NROM::IRQ()
{
    return false;
}

void NROM::Serialize(std::ofstream& saveState)
{
    if (chrRamMode_)
    {
        saveState.write((char*)CHR_ROM_.data(), 0x2000);
    }
}

void NROM::Deserialize(std::ifstream& saveState)
{
    if (chrRamMode_)
    {
        saveState.read((char*)CHR_ROM_.data(), 0x2000);
    }
}

void NROM::LoadROM(std::ifstream& rom, size_t prgRomBanks, size_t chrRomBanks)
{
    chrRamMode_ = (chrRomBanks == 0);

    if (prgRomBanks == 1)
    {
        rom.read((char*)PRG_ROM_.data(), 0x4000);
        std::copy(PRG_ROM_.begin(), PRG_ROM_.begin() + 0x4000, PRG_ROM_.begin() + 0x4000);
    }
    else
    {
        rom.read((char*)PRG_ROM_.data(), 0x8000);
    }

    if (!chrRamMode_)
    {
        rom.read((char*)CHR_ROM_.data(), 0x2000);
    }
}
