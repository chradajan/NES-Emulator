#include "../../include/mappers/NROM.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <vector>

NROM::NROM(std::ifstream& rom, std::array<uint8_t, 16> const& header)
{
    // Check for trainer data.
    if ((header[6] & TRAINER_DATA) == TRAINER_DATA)
    {
        // Load 0x0200 bytes of trainer data into PRG RAM at $7000.
        uint16_t trainerAddr = 0x1000;

        while (trainerAddr < 0x1200)
        {
            rom >> std::noskipws >> std::hex >> PRG_RAM[trainerAddr];
            ++trainerAddr;
        }
    }

    // Check number of 16KB blocks of PRG ROM.
    if (header[4] == 0x01)
    {
        PRG_Mirroring = true;
        PRG_ROM.resize(0x4000);
    }
    else
    {
        PRG_Mirroring = false;
        PRG_ROM.resize(0x8000);
    }

    // Check if ROM uses vertical or horizontal nametable mirroring.
    if ((header[6] & VERTICAL_MIRRORING_FLAG) == VERTICAL_MIRRORING_FLAG)
    {
        mirrorType = MirrorType::VERTICAL;
    }
    else
    {
        mirrorType = MirrorType::HORIZONTAL;
    }

    LoadROM(rom, 1, 1);
}

uint8_t NROM::ReadPRG(uint16_t addr)
{
    if (addr < 0x6000)
    {
        // Should not be reading below $6000.
        return 0x00;
    }
    else if (addr < 0x8000)
    {
        return PRG_RAM[addr - 0x6000];
    }
    else if ((addr < 0xC000) || !PRG_Mirroring)
    {
        return PRG_ROM[addr - 0x8000];
    }
    else
    {
        return PRG_ROM[addr - 0xC000];
    }
}

void NROM::WritePRG(uint16_t addr, uint8_t data)
{
    (void)addr;
    (void)data;
}

uint8_t NROM::ReadCHR(uint16_t addr)
{
    if (addr >= 0x2000)
    {
        // Should not be reading beyond $2000.
        return 0x00;
    }

    return CHR_ROM[addr];
}

void NROM::WriteCHR(uint16_t addr, uint8_t data)
{
    (void)addr;
    (void)data;
}

uint16_t NROM::NameTableAddress(uint16_t addr)
{
    addr &= 0x3FFF;

    if (addr > 0x2FFF)
    {
        addr -= 0x1000;
    }

    switch (mirrorType)
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

void NROM::LoadROM(std::ifstream& rom, uint8_t prgRomBanks, uint8_t chrRomBanks)
{
    (void)prgRomBanks;
    (void)chrRomBanks;

    // Load PRG ROM data.
    uint16_t maxPrgAddr = PRG_Mirroring ? 0x4000 : 0x8000;

    for (uint16_t prgAddr = 0x0000; prgAddr < maxPrgAddr; ++prgAddr)
    {
        rom >> std::noskipws >> std::hex >> PRG_ROM[prgAddr];
    }

    // Load CHR ROM data.
    for (uint16_t chrAddr = 0x0000; chrAddr < 0x2000; ++chrAddr)
    {
        rom >> std::noskipws >> std::hex >> CHR_ROM[chrAddr];
    }
}
