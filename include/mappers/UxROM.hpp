#ifndef UXROM_HPP
#define UXROM_HPP

#include "../Cartridge.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

constexpr uint8_t UXROM_BANK_SELECT_MASK = 0x0F;

class UxROM : public virtual Cartridge
{
public:
    UxROM(std::ifstream& rom, std::array<uint8_t, 16> const& header);

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
    std::vector<std::array<uint8_t, 0x4000>> PRG_ROM_BANKS_;
    std::array<uint8_t, 0x2000> CHR_ROM_;

    size_t prgIndex0;
    size_t prgIndex1;

    void LoadROM(std::ifstream& rom, size_t prgRomBanks, size_t chrRomBanks) override;
};

#endif
