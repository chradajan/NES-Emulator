#ifndef APU_HPP
#define APU_HPP

#include <array>

class APU
{
public:
    APU();
    ~APU() = default;
    void Reset();

    uint8_t ReadReg(uint16_t addr);
    void WriteReg(uint16_t addr, uint8_t data);
};

#endif
