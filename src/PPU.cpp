#include "../include/PPU.hpp"
#include "../include/RegisterAddresses.hpp"
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
    if (addr > 0x2007)
    {
        addr = 0x2000 + (addr % 0x08);
    }

    uint8_t returnData = 0x00;

    switch (addr)
    {
        case PPUSTATUS_ADDR:
            InternalRegisters.w = false;
            returnData = MemMappedRegisters.PPUSTATUS;
            MemMappedRegisters.PPUSTATUS &= 0x7F;
            break;
        case OAMDATA_ADDR:
            returnData = OAM[MemMappedRegisters.OAMADDR];
            break;
        case PPUDATA_ADDR:
            if (InternalRegisters.v < 0x3F00)
            {
                returnData = readBuffer;
                readBuffer = Read(InternalRegisters.v & VRAM_ADDR_MASK);
            }
            else
            {
                returnData = Read(InternalRegisters.v & VRAM_ADDR_MASK);
                readBuffer = Read(InternalRegisters.v - 0x1000);
            }

            if (RenderingEnabled() && scanline < 240)
            {
                CoarseXIncrement();
                YIncrement();
            }
            else
            {
                IncrementVRAMAddr();
            }
            break;
        default:
            break;
    }

    return returnData;
}

void PPU::WriteReg(uint16_t addr, uint8_t data)
{
    if (addr > 0x2007)
    {
        addr = 0x2000 + (addr % 0x08);
    }

    switch (addr)
    {
        case PPUCTRL_ADDR:
            MemMappedRegisters.PPUCTRL = data;
            InternalRegisters.t &= 0x73FF;
            InternalRegisters.t |= ((data & 0x03) << 10);
            break;
        case PPUMASK_ADDR:
            MemMappedRegisters.PPUMASK = data;
            break;
        case OAMADDR_ADDR:
            MemMappedRegisters.OAMADDR = data;
            break;
        case OAMDATA_ADDR:
            OAM[MemMappedRegisters.OAMADDR] = data;
            ++MemMappedRegisters.OAMADDR;
            break;
        case PPUSCROLL_ADDR:
            if (InternalRegisters.w)
            {
                InternalRegisters.t &= 0x0C1F;
                InternalRegisters.t |= (((data & 0x07) << 12) | ((data & 0xF8) << 2));
                InternalRegisters.w = false;
            }
            else
            {
                InternalRegisters.t &= 0x7FE0;
                InternalRegisters.t |= ((data & 0xF8) >> 3);
                InternalRegisters.x = (data & 0x07);
                InternalRegisters.w = true;
            }
            break;
        case PPUADDR_ADDR:
            if (InternalRegisters.w)
            {
                InternalRegisters.t &= 0x7F00;
                InternalRegisters.t |= data;
                InternalRegisters.v = InternalRegisters.t;
                InternalRegisters.w = false;
            }
            else
            {
                InternalRegisters.t &= 0x00FF;
                InternalRegisters.t |= ((data & 0x3F) << 8);
                InternalRegisters.w = true;
            }
            break;
        case PPUDATA_ADDR:
            Write(InternalRegisters.v & VRAM_ADDR_MASK, data);
            IncrementVRAMAddr();
            break;
        default:
            break;
    }
}

bool PPU::NMI()
{
    return false;
}

uint8_t PPU::Read(uint16_t addr)
{
    (void)addr;
    return 0x00;
}

void PPU::Write(uint16_t addr, uint8_t data)
{
    (void)addr;
    (void)data;
}

bool PPU::RenderingEnabled()
{
    constexpr uint8_t renderFlags = SHOW_BACKGROUND_MASK | SHOW_SPITES_MASK;
    return (MemMappedRegisters.PPUMASK & renderFlags) != 0x00;
}

void PPU::IncrementVRAMAddr()
{
    InternalRegisters.v &= 0x7FFF;

    if ((MemMappedRegisters.PPUCTRL & INCREMENT_VRAM_MASK) == INCREMENT_VRAM_MASK)
    {
        InternalRegisters.v += 0x20;
    }
    else
    {
        InternalRegisters.v += 0x01;
    }
}

void PPU::CoarseXIncrement()
{
    InternalRegisters.v &= 0x7FFF;

    if ((InternalRegisters.v & 0x001F) == 31)
    {
        InternalRegisters.v &= ~0x001F;
        InternalRegisters.v ^= 0x0400;
    }
    else
    {
        ++InternalRegisters.v;
    }
}

void PPU::YIncrement()
{
    InternalRegisters.v &= 0x7FFF;

    if ((InternalRegisters.v & 0x7000) != 0x7000)
    {
        InternalRegisters.v += 0x1000;
    }
    else
    {
        InternalRegisters.v &= ~0x7000;
        uint16_t y = (InternalRegisters.v & 0x03E0) >> 5;

        if (y == 29)
        {
            y = 0;
            InternalRegisters.v ^= 0x0800;
        }
        else if (y == 31)
        {
            y = 0;
        }
        else
        {
            ++y;
        }

        InternalRegisters.v = (InternalRegisters.v & ~0x03E0) | (y << 5);
    }
}
