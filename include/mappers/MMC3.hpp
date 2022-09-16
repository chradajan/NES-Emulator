#ifndef MMC3_HPP
#define MMC3_HPP

#include "../Cartridge.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

// BANK SIZES
constexpr size_t MMC3_PRG_BANK_SIZE = 0x2000;
constexpr size_t MMC3_CHR_BANK_SIZE = 0x0400;

// $8000-$9FFE, even
constexpr uint8_t MMC3_BANK_SELECT_MASK = 0x07;
constexpr uint8_t PRG_ROM_BANK_MODE_MASK = 0x40;
constexpr uint8_t CHR_ROM_BANK_MODE_MASK = 0x80;
constexpr uint8_t PRG_BANK_MASK = 0x3F;
constexpr uint8_t CHR_2KB_BANK_MASK = 0xFE;

// $A000-$BFFE, odd
constexpr uint8_t MMC3_MIRRORING_MASK = 0x01;

// $A001-$BFFF, odd
constexpr uint8_t PRG_RAM_CHIP_ENABLE_MASK = 0x80;
constexpr uint8_t WRITE_PROTECTION_MASK = 0x40;

// IRQ
constexpr uint16_t PPU_A12_MASK = 0x1000;

class MMC3 : public virtual Cartridge
{
public:
    MMC3(std::ifstream& rom, std::string savePath, std::array<uint8_t, 16> const& header);

    void Reset() override;

    uint8_t ReadPRG(uint16_t addr) override;
    void WritePRG(uint16_t addr, uint8_t data) override;

    uint8_t ReadCHR(uint16_t addr) override;
    void WriteCHR(uint16_t addr, uint8_t data) override;

    void SaveRAM() override;
    bool IRQ() override;

    void Serialize(std::ofstream& saveState) override;
    void Deserialize(std::ifstream& saveState) override;

private:
    void LoadROM(std::ifstream& rom, size_t prgRomBanks, size_t chrRomBanks) override;

private:
    std::array<uint8_t, 0x2000> PRG_RAM_;
    std::vector<std::array<uint8_t, MMC3_PRG_BANK_SIZE>> PRG_ROM_BANKS_;
    std::vector<std::array<uint8_t, MMC3_CHR_BANK_SIZE>> CHR_ROM_BANKS_;

    std::array<size_t, 4> prgIndex_;
    std::array<size_t, 8> chrIndex_;

// Bank data
private:
    uint8_t bankRegToUpdate_;
    std::array<size_t, 8> bankRegister_;

    // True:  $C000-$DFFF swappable, $8000-$9FFF fixed to second-to-last bank
    // False: $8000-$9FFF swappable, $C000-$DFFF fixed to second-to-last bank
    bool prgBankMode_;

    // True:  Two 2KB banks at $1000-$1FFF, four 1KB banks at $0000-$0FFF
    // False: Two 2KB banks at $0000-$0FFF, four 1KB banks at $1000-$1FFF
    bool chrBankMode_;

    void SetBanks();

// RAM
private:
    bool batteryBackedRam_;
    bool ramEnabled_;
    bool ramWritesDisabled_;
    std::string savePath_;

// IRQ
private:
    uint8_t irqLatch_;
    uint8_t irqCounter_;
    bool irqEnable_;
    bool reloadIrqCounter_;
    bool prevA12State;
    bool sendInterrupt_;

    void CheckA12(uint16_t addr);
    void ClockIRQ();
};

#endif
