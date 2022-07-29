#ifndef NROM_HPP
#define NROM_HPP

#include "../Cartridge.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <vector>

class NROM : public virtual Cartridge
{
public:
    NROM(std::ifstream& rom, std::array<uint8_t, 16> const& header);

    uint8_t ReadPRG(uint16_t addr) override;
    void WritePRG(uint16_t addr, uint8_t data) override;

    uint8_t ReadCHR(uint16_t addr) override;
    void WriteCHR(uint16_t addr, uint8_t data) override;

    uint16_t NameTableAddress(uint16_t addr) override;

private:
    std::array<uint8_t, 0x2000> PRG_RAM;
    std::vector<uint8_t> PRG_ROM;
    std::array<uint8_t, 0x2000> CHR_ROM;
    bool PRG_Mirroring;

    void LoadROM(std::ifstream& rom) override;
};

#endif
