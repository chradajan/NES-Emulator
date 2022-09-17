#ifndef CNROM_HPP
#define CNROM_HPP

#include "../Cartridge.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

class CNROM : public virtual Cartridge
{
public:
    CNROM(std::ifstream& rom, std::array<uint8_t, 16> const& header);

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
    std::vector<std::array<uint8_t, 0x2000>> CHR_ROM_BANKS_;

    size_t chrIndex_;

    void LoadROM(std::ifstream& rom, size_t prgRomBanks, size_t chrRomBanks) override;
};

#endif
