#ifndef MMC2_HPP
#define MMC2_HPP

#include "../Cartridge.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

constexpr size_t MMC2_PRG_BANK_SIZE = 0x2000;
constexpr size_t MMC2_CHR_BANK_SIZE = 0x1000;

constexpr uint16_t MMC2_PRG_READ_BANK_SELECT_MASK = 0x6000;
constexpr uint16_t MMC2_PRG_WRITE_ADDR_MASK = 0x7000;

constexpr uint8_t MMC2_PRG_BANK_SELECT_MASK = 0x0F;
constexpr uint8_t MMC2_CHR_BANK_SELECT_MASK = 0x1F;
constexpr uint8_t MMC2_MIRRORING_SELECT_MASK = 0x01;

class MMC2 : public virtual Cartridge
{
public:
    MMC2(std::ifstream& rom, std::array<uint8_t, 16> const& header);

    void Reset() override;

    uint8_t ReadPRG(uint16_t addr) override;
    void WritePRG(uint16_t addr, uint8_t data) override;

    uint8_t ReadCHR(uint16_t addr) override;
    void WriteCHR(uint16_t addr, uint8_t data) override;

    void SaveRAM() override;
    bool IRQ() override;

private:
    std::array<uint8_t, 0x2000> PRG_RAM_;
    std::vector<std::array<uint8_t, MMC2_PRG_BANK_SIZE>> PRG_ROM_BANKS_;
    std::vector<std::array<uint8_t, MMC2_CHR_BANK_SIZE>> CHR_ROM_BANKS_;

    std::array<size_t, 4> prgIndex_;
    size_t chrIndex0_;
    size_t chrIndex1_;

    uint8_t latch0_;
    uint8_t latch1_;
    uint8_t leftBankFD_;    // latch0_ == 0xFD
    uint8_t leftBankFE_;    // latch0_ == 0xFE
    uint8_t rightBankFD_;   // latch1_ == 0xFD
    uint8_t rightBankFE_;   // latch1_ == 0xFE

    void UpdateChrBanks();

    void LoadROM(std::ifstream& rom, size_t prgRomBanks, size_t chrRomBanks) override;
};

#endif
