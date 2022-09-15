#ifndef MMC1_HPP
#define MMC1_HPP

#include "../Cartridge.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

constexpr uint8_t MIRRORING_MASK = 0x03;
constexpr uint8_t PRG_ROM_BANK_MODE = 0x0C;
constexpr uint8_t CHR_ROM_BANK_MODE = 0x10;
constexpr uint8_t SHIFT_RESET_MASK = 0x80;
constexpr uint16_t MMC1_ADDR_MASK = 0x6000;

class MMC1 : public virtual Cartridge
{
public:
    MMC1(std::ifstream& rom, std::string savePath, std::array<uint8_t, 16> const& header);

    uint8_t ReadPRG(uint16_t addr) override;
    void WritePRG(uint16_t addr, uint8_t data) override;

    uint8_t ReadCHR(uint16_t addr) override;
    void WriteCHR(uint16_t addr, uint8_t data) override;

    uint16_t NameTableAddress(uint16_t addr) override;

    void SaveRAM() override;

    bool IRQ() override;

private:
    std::array<std::array<uint8_t, 0x2000>, 4> PRG_RAM_BANKS_;
    std::vector<std::array<uint8_t, 0x4000>> PRG_ROM_BANKS_;
    std::vector<std::array<uint8_t, 0x1000>> CHR_ROM_BANKS_;

    struct
    {
        uint8_t load;
        uint8_t control;
        uint8_t chrBank0;
        uint8_t chrBank1;
        uint8_t prgBank;
    } Reg_;

    struct
    {
        size_t chr0;
        size_t chr1;
        size_t prg0;
        size_t prg1;
        size_t prgRam;
    } Index_;

    bool batteryBackedRam_;
    uint8_t writeCounter_;
    std::string savePath_;

    void LoadROM(std::ifstream& rom, size_t prgRomBanks, size_t chrRomBanks) override;
    void SetRegisters(uint16_t addr);
    void UpdateIndices();
};

#endif
