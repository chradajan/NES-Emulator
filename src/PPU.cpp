#include "../include/PPU.hpp"
#include <cstdint>

PPU::PPU(Cartridge& cartridge) :
    cartridge(cartridge)
{

}

void PPU::Reset()
{

}

uint8_t PPU::ReadReg(uint16_t addr)
{
    (void)addr;
    return 0x00;
}

void PPU::WriteReg(uint16_t addr, uint8_t data)
{
    (void)addr;
    (void)data;
}

bool PPU::NMI()
{
    return false;
}
