#ifndef PPU_HPP
#define PPU_HPP

#include <cstdint>

class PPU
{
public:
    PPU();
    ~PPU() = default;
    void Reset();

    uint8_t ReadReg(uint16_t addr);
    void WriteReg(uint16_t addr, uint8_t data);

    bool NMI();
};

#endif
