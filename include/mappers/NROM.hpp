#ifndef NROM_HPP
#define NROM_HPP

#include "../Cartridge.hpp"
#include <array>
#include <cstdint>
#include <fstream>

class NROM : public virtual Cartridge
{
public:
    NROM(std::ifstream& rom, std::array<uint8_t, 16> const& header);

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
    std::array<uint8_t, 0x8000> PRG_ROM_;
    std::array<uint8_t, 0x2000> CHR_ROM_;

    void LoadROM(std::ifstream& rom, size_t prgRomBanks, size_t chrRomBanks) override;
};

#endif
