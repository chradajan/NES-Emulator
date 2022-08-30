#include "../../include/mappers/MMC1.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>

MMC1::MMC1(std::ifstream& rom, std::string const savePath, std::array<uint8_t, 16> const& header)
{
    batteryBackedRam_ = ((header[6] & BATTERY_BACKED_PRG_RAM) == BATTERY_BACKED_PRG_RAM);
    savePath_ = savePath;

    Index_.prgRam = 0;
    Reg_.control = 0x0C;
    Reg_.chrBank0 = 0x00;
    Reg_.chrBank1 = 0x00;
    Reg_.prgBank = 0x00;

    for (auto& ramBank : PRG_RAM_BANKS_)
    {
        ramBank.fill(0x00);
    }

    for (auto& romBank : PRG_ROM_BANKS_)
    {
        romBank.fill(0x00);
    }
    
    for (auto& romBank : CHR_ROM_BANKS_)
    {
        romBank.fill(0x00);
    }

    if (batteryBackedRam_)
    {
        std::ifstream save(savePath, std::ios::binary);

        if (!save.fail())
        {
            for (size_t ramBankIndex = 0; ramBankIndex < 4; ++ramBankIndex)
            {
                for (size_t index = 0; index < 0x2000; ++index)
                {
                    save >> std::noskipws >> std::hex >> PRG_RAM_BANKS_[ramBankIndex][index];
                }
            }
        }
    }

    // Check for trainer data.
    if ((header[6] & TRAINER_DATA) == TRAINER_DATA)
    {
        // Load 0x0200 bytes of trainer data into PRG RAM at $7000.
        uint16_t trainerAddr = 0x1000;

        while (trainerAddr < 0x1200)
        {
            rom >> std::noskipws >> std::hex >> PRG_RAM_BANKS_[Index_.prgRam][trainerAddr];
            ++trainerAddr;
        }
    }

    LoadROM(rom, header[4], header[5]);
    UpdateIndices();
}

uint8_t MMC1::ReadPRG(uint16_t addr)
{
    if (addr < 0x6000)
    {
        return 0x00;
    }
    else if (addr < 0x8000)
    {
        return PRG_RAM_BANKS_[Index_.prgRam][addr - 0x6000];
    }
    else if (addr < 0xC000)
    {
        return PRG_ROM_BANKS_[Index_.prg0][addr - 0x8000];
    }
    else
    {
        return PRG_ROM_BANKS_[Index_.prg1][addr - 0xC000];
    }
}

void MMC1::WritePRG(uint16_t addr, uint8_t data)
{
    if (addr < 0x6000)
    {
        return;
    }
    if (addr < 0x8000)
    {
        PRG_RAM_BANKS_[Index_.prgRam][addr - 0x6000] = data;
    }
    else
    {
        if ((data & SHIFT_RESET_MASK) == SHIFT_RESET_MASK)
        {
            writeCounter_ = 0;
            Reg_.load = 0x00;
        }
        else
        {
            Reg_.load >>= 1;
            Reg_.load |= ((data & 0x01) << 4);
            ++writeCounter_;

            if (writeCounter_ == 5)
            {
                writeCounter_ = 0;
                SetRegisters(addr);
                Reg_.load = 0;
            }
        }
    }
}

uint8_t MMC1::ReadCHR(uint16_t addr)
{
    if (addr < 0x1000)
    {
        return CHR_ROM_BANKS_[Index_.chr0][addr];
    }
    else if (addr < 0x2000)
    {
        return CHR_ROM_BANKS_[Index_.chr1][addr - 0x1000];
    }
    else
    {
        return 0x00;
    }

}

void MMC1::WriteCHR(uint16_t addr, uint8_t data)
{
    if (chrRamMode_)
    {
        if (addr < 0x1000)
        {
            CHR_ROM_BANKS_[Index_.chr0][addr] = data;
        }
        else if (addr < 0x2000)
        {
            CHR_ROM_BANKS_[Index_.chr1][addr - 0x1000] = data;
        }
    }
}

uint16_t MMC1::NameTableAddress(uint16_t addr)
{
    addr &= 0x3FFF;

    if (addr > 0x2FFF)
    {
        addr -= 0x1000;
    }

    switch (mirrorType)
    {
        case MirrorType::SINGLE_LOW:
            addr %= 0x0400;
            break;
        case MirrorType::SINGLE_HIGH:
            addr = 0x0400 | (addr % 0x0400);
            break;
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

void MMC1::SaveRAM()
{
    if (batteryBackedRam_)
    {
        std::ofstream save(savePath_, std::ios::binary);

        if (!save.fail())
        {
            for (size_t ramBankIndex = 0; ramBankIndex < 4; ++ramBankIndex)
            {
                for (size_t index = 0; index < 0x2000; ++index)
                {
                    save << std::noskipws << std::hex << PRG_RAM_BANKS_[ramBankIndex][index];
                }
            }
        }
    }
}

void MMC1::LoadROM(std::ifstream& rom, uint8_t prgRomBanks, uint8_t chrRomBanks)
{
    PRG_ROM_BANKS_.resize(prgRomBanks);

    if (chrRomBanks == 0)
    {
        CHR_ROM_BANKS_.resize(2);
        chrRamMode_ = true;
    }
    else
    {
        CHR_ROM_BANKS_.resize(chrRomBanks);
    }

    for (uint8_t bankIndex = 0; bankIndex < prgRomBanks; bankIndex++)
    {
        for (uint16_t prgAddr = 0x0000; prgAddr < 0x4000; ++prgAddr)
        {
            rom >> std::noskipws >> std::hex >> PRG_ROM_BANKS_[bankIndex][prgAddr];
        }
    }

    for (uint8_t bankIndex = 0; bankIndex < chrRomBanks; bankIndex++)
    {
        for (uint16_t chrAddr = 0x0000; chrAddr < 0x1000; ++chrAddr)
        {
            rom >> std::noskipws >> std::hex >> CHR_ROM_BANKS_[bankIndex][chrAddr];
        }
    }
}

void MMC1::SetRegisters(uint16_t addr)
{
    uint8_t regSelection = ((addr & ADDR_MASK) >> 13);

    switch (regSelection)
    {
        case 0:
            Reg_.control = Reg_.load;
            break;
        case 1:
            Reg_.chrBank0 = Reg_.load;
            break;
        case 2:
            Reg_.chrBank1 = Reg_.load;
            break;
        case 3:
            Reg_.prgBank = Reg_.load;
            break;
        default:
            break;
    }

    UpdateIndices();
}

void MMC1::UpdateIndices()
{
    uint8_t mirroring = (Reg_.control & MIRRORING_MASK);
    uint8_t prgMode = ((Reg_.control & PRG_ROM_BANK_MODE) >> 2);
    uint8_t chrMode = ((Reg_.control & CHR_ROM_BANK_MODE) >> 4);

    switch (mirroring)
    {
        case 0:
            mirrorType = MirrorType::SINGLE_LOW;
            break;
        case 1:
            mirrorType = MirrorType::SINGLE_HIGH;
            break;
        case 2:
            mirrorType = MirrorType::VERTICAL;
            break;
        case 3:
            mirrorType = MirrorType::HORIZONTAL;
            break;
        default:
            break;
    }

    switch (prgMode)
    {
        case 0:
        case 1:
            Index_.prg0 = Reg_.prgBank & 0xFE;
            Index_.prg1 = Index_.prg0 + 1;
            break;
        case 2:
            Index_.prg0 = 0;
            Index_.prg1 = Reg_.prgBank & 0x0F;
            break;
        case 3:
            Index_.prg0 = Reg_.prgBank & 0x0F;
            Index_.prg1 = PRG_ROM_BANKS_.size() - 1;
            break;
        default:
            break;
    }

    switch (chrMode)
    {
        case 0:
            Index_.chr0 = Reg_.chrBank0 & 0xFE;
            Index_.chr1 = Index_.chr0 + 1;
            break;
        case 1:
            Index_.chr0 = Reg_.chrBank0;
            Index_.chr1 = Reg_.chrBank1;
            break;
        default:
            break;
    }
}
