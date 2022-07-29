#ifndef PPU_HPP
#define PPU_HPP

#include <cstdint>
#include <memory>

class Cartridge;

class PPU
{
public:
    PPU(Cartridge& cartridge);
    ~PPU() = default;
    void Reset();

    uint8_t ReadReg(uint16_t addr);
    void WriteReg(uint16_t addr, uint8_t data);

    bool NMI();

private:
    Cartridge& cartridge;
};

#endif
