#include "../../include/mappers/MMC2.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <vector>

MMC2::MMC2(std::ifstream& rom, std::array<uint8_t, 16> const& header)
{
    if ((header[6] & VERTICAL_MIRRORING_FLAG) == VERTICAL_MIRRORING_FLAG)
    {
        mirrorType_ = MirrorType::VERTICAL;
    }
    else
    {
        mirrorType_ = MirrorType::HORIZONTAL;
    }

    size_t prgRomBanksCount = header[4] * 2;
    size_t chrRomBanksCount = header[5] * 2;

    LoadROM(rom, prgRomBanksCount, chrRomBanksCount);

    prgIndex_[0] = 0;

    for (size_t i = 1; i < 4; ++i)
    {
        prgIndex_[i] = PRG_ROM_BANKS_.size() - (4 - i);
    }

    chrIndex0_ = 0;
    chrIndex1_ = 1;
}

void MMC2::Reset()
{
    prgIndex_[0] = 0;
    chrIndex0_ = 0;
    chrIndex1_ = 1;
}

uint8_t MMC2::ReadPRG(uint16_t addr)
{
    size_t prgBank = (addr & MMC2_PRG_READ_BANK_SELECT_MASK) >> 13;
    return PRG_ROM_BANKS_[prgIndex_[prgBank]][addr % MMC2_PRG_BANK_SIZE];
}

void MMC2::WritePRG(uint16_t addr, uint8_t data)
{
    uint16_t writeReg = (addr & MMC2_PRG_WRITE_ADDR_MASK) >> 12;

    switch (writeReg)
    {
        case 2:  // PRG ROM bank select ($A000-$AFFF)
            prgIndex_[0] = data & MMC2_PRG_BANK_SELECT_MASK;
            break;
        case 3:  // CHR ROM $FD/0000 bank select ($B000-$BFFF)
            leftBankFD_ = data & MMC2_CHR_BANK_SELECT_MASK;
            UpdateChrBanks();
            break;
        case 4:  // CHR ROM $FE/0000 bank select ($C000-$CFFF)
            leftBankFE_ = data & MMC2_CHR_BANK_SELECT_MASK;
            UpdateChrBanks();
            break;
        case 5:  // CHR ROM $FD/1000 bank select ($D000-$DFFF)
            rightBankFD_ = data & MMC2_CHR_BANK_SELECT_MASK;
            UpdateChrBanks();
            break;
        case 6:  // CHR ROM $FE/1000 bank select ($E000-$EFFF)
            rightBankFE_ = data & MMC2_CHR_BANK_SELECT_MASK;
            UpdateChrBanks();
            break;
        case 7:  // Mirroring ($F000-$FFFF)
            if ((data & MMC2_MIRRORING_SELECT_MASK) == MMC2_MIRRORING_SELECT_MASK)
            {
                mirrorType_ = MirrorType::HORIZONTAL;
            }
            else
            {
                mirrorType_ = MirrorType::VERTICAL;
            }
            break;
        default:
            break;
    }
}

uint8_t MMC2::ReadCHR(uint16_t addr)
{
    uint8_t chrByte;

    if (addr < 0x1000)
    {
        chrByte = CHR_ROM_BANKS_[chrIndex0_][addr];
    }
    else
    {
        chrByte = CHR_ROM_BANKS_[chrIndex1_][addr - 0x1000];
    }

    switch (addr)
    {
        case 0x0FD8:
            latch0_ = 0xFD;
            UpdateChrBanks();
            break;
        case 0x0FE8:
            latch0_ = 0xFE;
            UpdateChrBanks();
            break;
        case 0x1FD8 ... 0x1FDF:
            latch1_ = 0xFD;
            UpdateChrBanks();
            break;
        case 0x1FE8 ... 0x1FEF:
            latch1_ = 0xFE;
            UpdateChrBanks();
            break;
        default:
            break;
    }

    return chrByte;
}

void MMC2::WriteCHR(uint16_t addr, uint8_t data)
{
    (void)addr;
    (void)data;
}

void MMC2::SaveRAM()
{

}

bool MMC2::IRQ()
{
    return false;
}

void MMC2::Serialize(std::ofstream& saveState)
{
    saveState.write((char*)PRG_RAM_.data(), 0x2000);
    saveState.write((char*)prgIndex_.data(), 4 * sizeof(prgIndex_[0]));
    saveState.write((char*)&chrIndex0_, sizeof(chrIndex0_));
    saveState.write((char*)&chrIndex1_, sizeof(chrIndex1_));

    uint16_t byteExpander = latch0_;
    saveState.write((char*)&byteExpander, sizeof(latch0_));

    byteExpander = latch1_;
    saveState.write((char*)&byteExpander, sizeof(latch1_));

    byteExpander = leftBankFD_;
    saveState.write((char*)&byteExpander, sizeof(leftBankFD_));

    byteExpander = leftBankFE_;
    saveState.write((char*)&byteExpander, sizeof(leftBankFE_));

    byteExpander = rightBankFD_;
    saveState.write((char*)&byteExpander, sizeof(rightBankFD_));

    byteExpander = rightBankFE_;
    saveState.write((char*)&byteExpander, sizeof(rightBankFE_));

    byteExpander = static_cast<uint16_t>(mirrorType_);
    saveState.write((char*)&byteExpander, sizeof(mirrorType_));
}

void MMC2::Deserialize(std::ifstream& saveState)
{
    saveState.read((char*)PRG_RAM_.data(), 0x2000);
    saveState.read((char*)prgIndex_.data(), 4 * sizeof(prgIndex_[0]));
    saveState.read((char*)&chrIndex0_, sizeof(chrIndex0_));
    saveState.read((char*)&chrIndex1_, sizeof(chrIndex1_));

    saveState.read((char*)&latch0_, sizeof(latch0_));
    saveState.read((char*)&latch1_, sizeof(latch1_));
    saveState.read((char*)&leftBankFD_, sizeof(leftBankFD_));
    saveState.read((char*)&leftBankFE_, sizeof(leftBankFE_));
    saveState.read((char*)&rightBankFD_, sizeof(rightBankFD_));
    saveState.read((char*)&rightBankFE_, sizeof(rightBankFE_));
    saveState.read((char*)&mirrorType_, sizeof(mirrorType_));
}

void MMC2::LoadROM(std::ifstream& rom, size_t prgRomBanks, size_t chrRomBanks)
{
    PRG_ROM_BANKS_.resize(prgRomBanks);
    CHR_ROM_BANKS_.resize(chrRomBanks);

    for (size_t bankIndex = 0; bankIndex < prgRomBanks; ++bankIndex)
    {
        for (size_t prgByte = 0; prgByte < MMC2_PRG_BANK_SIZE; ++prgByte)
        {
            rom >> std::noskipws >> std::hex >> PRG_ROM_BANKS_[bankIndex][prgByte];
        }
    }

    for (size_t bankIndex = 0; bankIndex < chrRomBanks; ++bankIndex)
    {
        for (size_t chrByte = 0; chrByte < MMC2_CHR_BANK_SIZE; ++chrByte)
        {
            rom >> std::noskipws >> std::hex >> CHR_ROM_BANKS_[bankIndex][chrByte];
        }
    }
}

void MMC2::UpdateChrBanks()
{
    if (latch0_ == 0xFD)
    {
        chrIndex0_ = leftBankFD_;
    }
    else if (latch0_ == 0xFE)
    {
        chrIndex0_ = leftBankFE_;
    }

    if (latch1_ == 0xFD)
    {
        chrIndex1_ = rightBankFD_;
    }
    else if (latch1_ == 0xFE)
    {
        chrIndex1_ = rightBankFE_;
    }
}
