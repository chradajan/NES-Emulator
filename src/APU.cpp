#include "../include/APU.hpp"
#include <cstdint>

APU::APU()
{

}

void APU::Reset()
{

}

uint8_t APU::ReadReg(uint16_t addr)
{
    (void)addr;
    return 0x00;
}

void APU::WriteReg(uint16_t addr, uint8_t data)
{
    (void)addr;
    (void)data;
}
