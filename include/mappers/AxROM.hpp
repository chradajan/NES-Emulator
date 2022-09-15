#ifndef AXROM_HPP
#define AXROM_HPP

#include "../Cartridge.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

constexpr uint8_t AXROM_BANK_SELECT_MASK = 0x07;
constexpr uint8_t NAMETABLE_MIRRORING_MASK = 0x10;

class AxROM : public virtual Cartridge
{
public:
    AxROM(std::ifstream& rom, std::array<uint8_t, 16> const& header);

    void Reset() override;

    uint8_t ReadPRG(uint16_t addr) override;
    void WritePRG(uint16_t addr, uint8_t data) override;

    uint8_t ReadCHR(uint16_t addr) override;
    void WriteCHR(uint16_t addr, uint8_t data) override;

    void SaveRAM() override;
    bool IRQ() override;

private:
    std::vector<std::array<uint8_t, 0x8000>> PRG_ROM_BANKS_;
    std::array<uint8_t, 0x2000> CHR_ROM_;

    size_t prgIndex_;

    void LoadROM(std::ifstream& rom, size_t prgRomBanks, size_t chrRomBanks) override;
};

#endif
