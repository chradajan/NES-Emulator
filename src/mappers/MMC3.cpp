#include "../../include/mappers/MMC3.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>

MMC3::MMC3(std::ifstream& rom, std::string savePath, std::array<uint8_t, 16> const& header) :
    savePath_(savePath)
{
    batteryBackedRam_ = ((header[6] & BATTERY_BACKED_PRG_RAM) == BATTERY_BACKED_PRG_RAM);
    PRG_RAM_.fill(0x00);
    bankRegToUpdate_ = 0;
    bankRegister_.fill(0);
    prgBankMode_ = false;
    chrBankMode_ = false;

    ramEnabled_ = true;
    ramWritesDisabled_ = false;
    irqLatch_ = 0;
    irqEnable_ = false;
    irqCounter_ = 0;
    reloadIrqCounter_ = false;
    prevA12State = false;
    sendInterrupt_ = false;

    if ((header[6] & IGNORE_MIRRORING_CONTROL) == IGNORE_MIRRORING_CONTROL)
    {
        mirrorType_ = MirrorType::QUAD;
    }
    else if ((header[6] & VERTICAL_MIRRORING_FLAG) == VERTICAL_MIRRORING_FLAG)
    {
        mirrorType_ = MirrorType::VERTICAL;
    }
    else
    {
        mirrorType_ = MirrorType::HORIZONTAL;
    }

    size_t prgRomBanksCount = header[4] * 2;
    size_t chrRomBanksCount = header[5] * 8;

    LoadROM(rom, prgRomBanksCount, chrRomBanksCount);
    SetBanks();
}

uint8_t MMC3::ReadPRG(uint16_t addr)
{
    if (addr < 0x6000)
    {
        return 0x00;
    }
    else if (addr < 0x8000)
    {
        if (ramEnabled_)
        {
            return PRG_RAM_[addr - 0x6000];
        }

        // Should return open bus, but probably doesn't matter.
        return 0x00;
    }
    else
    {
        uint8_t bank = ((addr & 0x6000) >> 13);
        return PRG_ROM_BANKS_[prgIndex_[bank]][addr % MMC3_PRG_BANK_SIZE];
    }
}

void MMC3::WritePRG(uint16_t addr, uint8_t data)
{
    if (addr < 0x6000)
    {
        return;
    }
    else if (addr < 0x8000)
    {
        if (ramEnabled_ && !ramWritesDisabled_)
        {
            PRG_RAM_[addr - 0x6000] = data;
        }

        return;
    }

    bool evenAddr = ((addr % 2) == 0);

    if (addr < 0xA000)
    {
        if (evenAddr)
        {
            // Bank select
            bankRegToUpdate_ = data & MMC3_BANK_SELECT_MASK;
            prgBankMode_ = ((data & PRG_ROM_BANK_MODE_MASK) == PRG_ROM_BANK_MODE_MASK);
            chrBankMode_ = ((data & CHR_ROM_BANK_MODE_MASK) == CHR_ROM_BANK_MODE_MASK);
            SetBanks();
        }
        else
        {
            // Bank data
            bankRegister_[bankRegToUpdate_] = data;
            SetBanks();
        }
    }
    else if (addr < 0xC000)
    {
        if (evenAddr)
        {
            // Mirroring
            if (mirrorType_ != MirrorType::QUAD)
            {
                if ((data & MMC3_MIRRORING_MASK) == MMC3_MIRRORING_MASK)
                {
                    mirrorType_ = MirrorType::HORIZONTAL;
                }
                else
                {
                    mirrorType_ = MirrorType::VERTICAL;
                }
            }
        }
        else
        {
            // PRG RAM protect
            ramEnabled_ = ((data & PRG_RAM_CHIP_ENABLE_MASK) == PRG_RAM_CHIP_ENABLE_MASK);
            ramWritesDisabled_ = ((data & WRITE_PROTECTION_MASK) == WRITE_PROTECTION_MASK);
        }
    }
    else if (addr < 0xE000)
    {
        if (evenAddr)
        {
            // IRQ latch
            irqLatch_ = data;
        }
        else
        {
            // IRQ reload
            reloadIrqCounter_ = true;
        }
    }
    else
    {
        if (evenAddr)
        {
            // IRQ disable/acknowledge
            irqEnable_ = false;
            sendInterrupt_ = false;
        }
        else
        {
            // IRQ enable
            irqEnable_ = true;
        }
    }
}

uint8_t MMC3::ReadCHR(uint16_t addr)
{
    CheckA12(addr);

    if (addr >= 0x2000)
    {
        return 0x00;
    }

    size_t bank = ((addr & 0x1C00) >> 10);
    return CHR_ROM_BANKS_[chrIndex_[bank]][addr % MMC3_CHR_BANK_SIZE];
}

void MMC3::WriteCHR(uint16_t addr, uint8_t data)
{
    CheckA12(addr);

    if (chrRamMode_)
    {
        size_t bank = ((addr & 0x1C00) >> 10);
        CHR_ROM_BANKS_[chrIndex_[bank]][addr % MMC3_CHR_BANK_SIZE] = data;
    }
}

void MMC3::SaveRAM()
{
    if (batteryBackedRam_)
    {
        std::ofstream save(savePath_, std::ios::binary);

        if (!save.fail())
        {
            for (size_t i = 0; i < PRG_RAM_.size(); ++i)
            {
                save << std::noskipws << std::hex << PRG_RAM_[i];
            }
        }
    }
}

bool MMC3::IRQ()
{
    if (sendInterrupt_ && irqEnable_)
    {
        return true;
    }

    return false;
}

void MMC3::LoadROM(std::ifstream& rom, size_t prgRomBanks, size_t chrRomBanks)
{
    PRG_ROM_BANKS_.resize(prgRomBanks);
    CHR_ROM_BANKS_.resize(chrRomBanks);

    if (chrRomBanks == 0)
    {
        chrRamMode_ = true;
        CHR_ROM_BANKS_.resize(256);
    }

    prgIndex_[3] = PRG_ROM_BANKS_.size() - 1;

    for (size_t bankIndex = 0; bankIndex < prgRomBanks; ++bankIndex)
    {
        for (size_t prgByte = 0; prgByte < MMC3_PRG_BANK_SIZE; ++prgByte)
        {
            rom >> std::noskipws >> std::hex >> PRG_ROM_BANKS_[bankIndex][prgByte];
        }
    }

    for (size_t bankIndex = 0; bankIndex < chrRomBanks; ++bankIndex)
    {
        for (size_t chrByte = 0; chrByte < MMC3_CHR_BANK_SIZE; ++chrByte)
        {
            rom >> std::noskipws >> std::hex >> CHR_ROM_BANKS_[bankIndex][chrByte];
        }
    }
}

void MMC3::SetBanks()
{
    // CHR Setup:
    // ---------------------------

    //             $0000   $0400   $0800   $0C00   $1000   $1400   $1800   $1C00
    //             +---------------+---------------+-------+-------+-------+-------+
    // CHR Mode 0: |     <R:0>     |     <R:1>     |  R:2  |  R:3  |  R:4  |  R:5  |
    //             +---------------+---------------+---------------+---------------+
    // CHR Mode 1: |  R:2  |  R:3  |  R:4  |  R:5  |     <R:0>     |     <R:1>     |
    //             +-------+-------+-------+-------+---------------+---------------+

    // PRG Setup:
    // ---------------------------

    //             $8000   $A000   $C000   $E000 
    //             +-------+-------+-------+-------+
    // PRG Mode 0: |  R:6  |  R:7  | { -2} | { -1} |
    //             +-------+-------+-------+-------+
    // PRG Mode 1: | { -2} |  R:7  |  R:6  | { -1} |
    //             +-------+-------+-------+-------+

    // Set PRG Banks
    prgIndex_[1] = (bankRegister_[7] & PRG_BANK_MASK) % PRG_ROM_BANKS_.size();

    if (prgBankMode_)
    {
        prgIndex_[0] = PRG_ROM_BANKS_.size() - 2;
        prgIndex_[2] = (bankRegister_[6] & PRG_BANK_MASK) % PRG_ROM_BANKS_.size();
    }
    else
    {
        prgIndex_[0] = (bankRegister_[6] & PRG_BANK_MASK) % PRG_ROM_BANKS_.size();
        prgIndex_[2] = PRG_ROM_BANKS_.size() - 2;
    }

    // Set CHR Banks
    if (chrBankMode_)
    {
        chrIndex_[0] = bankRegister_[2] % CHR_ROM_BANKS_.size();
        chrIndex_[1] = bankRegister_[3] % CHR_ROM_BANKS_.size();
        chrIndex_[2] = bankRegister_[4] % CHR_ROM_BANKS_.size();
        chrIndex_[3] = bankRegister_[5] % CHR_ROM_BANKS_.size();
        chrIndex_[4] = (bankRegister_[0] & CHR_2KB_BANK_MASK) % CHR_ROM_BANKS_.size();
        chrIndex_[5] = (chrIndex_[4] | 0x01) % CHR_ROM_BANKS_.size();
        chrIndex_[6] = (bankRegister_[1] & CHR_2KB_BANK_MASK) % CHR_ROM_BANKS_.size();
        chrIndex_[7] = (chrIndex_[6] | 0x01) % CHR_ROM_BANKS_.size();
    }
    else
    {
        chrIndex_[0] = (bankRegister_[0] & CHR_2KB_BANK_MASK) % CHR_ROM_BANKS_.size();
        chrIndex_[1] = (chrIndex_[0] | 0x01) % CHR_ROM_BANKS_.size();
        chrIndex_[2] = (bankRegister_[1] & CHR_2KB_BANK_MASK) % CHR_ROM_BANKS_.size();
        chrIndex_[3] = (chrIndex_[2] | 0x01) % CHR_ROM_BANKS_.size();
        chrIndex_[4] = bankRegister_[2] % CHR_ROM_BANKS_.size();
        chrIndex_[5] = bankRegister_[3] % CHR_ROM_BANKS_.size();
        chrIndex_[6] = bankRegister_[4] % CHR_ROM_BANKS_.size();
        chrIndex_[7] = bankRegister_[5] % CHR_ROM_BANKS_.size();
    }
}

void MMC3::CheckA12(uint16_t addr)
{
    bool currA12State = ((addr & PPU_A12_MASK) == PPU_A12_MASK);

    if (!prevA12State && currA12State)
    {
        // Rising edge detected
        ClockIRQ();
    }

    prevA12State = currA12State;
}

void MMC3::ClockIRQ()
{
    if (reloadIrqCounter_ || (irqCounter_ == 0))
    {
        if (irqLatch_ == 0)
        {
            sendInterrupt_ = irqEnable_;
        }

        reloadIrqCounter_ = false;
        irqCounter_ = irqLatch_;
    }
    else
    {
        --irqCounter_;

        if (irqCounter_ == 0)
        {
            sendInterrupt_ = irqEnable_;
        }
    }
}
