#include "../../include/mappers/MMC1.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

MMC1::MMC1(std::ifstream& rom, std::string const savePath, std::array<uint8_t, 16> const& header) :
    savePath_(savePath)
{
    batteryBackedRam_ = ((header[6] & BATTERY_BACKED_PRG_RAM) == BATTERY_BACKED_PRG_RAM);

    Reg_.control = 0x0C;
    Reg_.chrBank0 = 0x00;
    Reg_.chrBank1 = 0x00;
    Reg_.prgBank = 0x00;

    PRG_RAM_.fill(0x00);

    if (batteryBackedRam_)
    {
        std::ifstream save(savePath, std::ios::binary);

        if (!save.fail())
        {
            save.read((char*)PRG_RAM_.data(), 0x2000);
        }
    }

    size_t prgRomBanksCount = header[4];
    size_t chrRomBanksCount = header[5] * 2;

    LoadROM(rom, prgRomBanksCount, chrRomBanksCount);
    UpdateIndices();
}

void MMC1::Reset()
{
    Reg_.control = 0x0C;
    Reg_.chrBank0 = 0x00;
    Reg_.chrBank1 = 0x00;
    Reg_.prgBank = 0x00;
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
        return PRG_RAM_[addr - 0x6000];
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
        PRG_RAM_[addr - 0x6000] = data;
    }
    else
    {
        if ((data & SHIFT_RESET_MASK) == SHIFT_RESET_MASK)
        {
            writeCounter_ = 0;
            Reg_.load = 0x00;
            Reg_.control |= 0x0C;
            UpdateIndices();
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
                Reg_.load = 0x00;
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

void MMC1::SaveRAM()
{
    if (batteryBackedRam_)
    {
        std::ofstream save(savePath_, std::ios::binary);

        if (!save.fail())
        {
            save.write((char*)PRG_RAM_.data(), 0x2000);
        }
    }
}

bool MMC1::IRQ()
{
    return false;
}

void MMC1::Serialize(std::ofstream& saveState)
{
    saveState.write((char*)PRG_RAM_.data(), 0x2000);
    saveState.write((char*)&Reg_, sizeof(Reg_));
    saveState.write((char*)&Index_, sizeof(Index_));

    uint16_t byteExpander = writeCounter_;
    saveState.write((char*)&byteExpander, sizeof(writeCounter_));

    byteExpander = static_cast<uint16_t>(mirrorType_);
    saveState.write((char*)&byteExpander, sizeof(mirrorType_));

    if (chrRamMode_)
    {
        for (auto& chrRomBank : CHR_ROM_BANKS_)
        {
            saveState.write((char*)chrRomBank.data(), 0x1000);
        }
    }
}

void MMC1::Deserialize(std::ifstream& saveState)
{
    saveState.read((char*)PRG_RAM_.data(), 0x2000);
    saveState.read((char*)&Reg_, sizeof(Reg_));
    saveState.read((char*)&Index_, sizeof(Index_));
    saveState.read((char*)&writeCounter_, sizeof(writeCounter_));
    saveState.read((char*)&mirrorType_, sizeof(mirrorType_));

    if (chrRamMode_)
    {
        for (auto& chrRomBank : CHR_ROM_BANKS_)
        {
            saveState.read((char*)chrRomBank.data(), 0x1000);
        }
    }
}

void MMC1::LoadROM(std::ifstream& rom, size_t prgRomBanks, size_t chrRomBanks)
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

    for (size_t bankIndex = 0; bankIndex < prgRomBanks; ++bankIndex)
    {
        rom.read((char*)PRG_ROM_BANKS_[bankIndex].data(), 0x4000);
    }

    for (size_t bankIndex = 0; bankIndex < chrRomBanks; ++bankIndex)
    {
        rom.read((char*)CHR_ROM_BANKS_[bankIndex].data(), 0x1000);
    }
}

void MMC1::SetRegisters(uint16_t addr)
{
    uint8_t regSelection = ((addr & MMC1_ADDR_MASK) >> 13);

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
            mirrorType_ = MirrorType::SINGLE_LOW;
            break;
        case 1:
            mirrorType_ = MirrorType::SINGLE_HIGH;
            break;
        case 2:
            mirrorType_ = MirrorType::VERTICAL;
            break;
        case 3:
            mirrorType_ = MirrorType::HORIZONTAL;
            break;
        default:
            break;
    }

    switch (prgMode)
    {
        case 0:
        case 1:
            Index_.prg0 = Reg_.prgBank & 0x0E;
            Index_.prg1 = Index_.prg0 | 0x01;
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
            Index_.chr0 = Reg_.chrBank0 & 0x1E;
            Index_.chr1 = Index_.chr0 | 0x01;
            break;
        case 1:
            Index_.chr0 = Reg_.chrBank0;
            Index_.chr1 = Reg_.chrBank1;
            break;
        default:
            break;
    }
}
