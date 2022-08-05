#include "../include/PPU.hpp"
#include "../include/Cartridge.hpp"
#include "../include/RegisterAddresses.hpp"
#include <cstdint>

PPU::PPU(Cartridge& cartridge) :
    cartridge_(cartridge)
{
    uint8_t paletteArray[192] = {
        0x7C, 0x7C, 0x7C, 0x00, 0x00, 0xFC, 0x00, 0x00, 0xBC, 0x44, 0x28, 0xBC, 0x94, 0x00, 0x84, 0xA8, 0x00, 0x20,
        0xA8, 0x10, 0x00, 0x88, 0x14, 0x00, 0x50, 0x30, 0x00, 0x00, 0x78, 0x00, 0x00, 0x68, 0x00, 0x00, 0x58, 0x00,
        0x00, 0x40, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBC, 0xBC, 0xBC, 0x00, 0x78, 0xF8,
        0x00, 0x58, 0xF8, 0x68, 0x44, 0xFC, 0xD8, 0x00, 0xCC, 0xE4, 0x00, 0x58, 0xF8, 0x38, 0x00, 0xE4, 0x5C, 0x10,
        0xAC, 0x7C, 0x00, 0x00, 0xB8, 0x00, 0x00, 0xA8, 0x00, 0x00, 0xA8, 0x44, 0x00, 0x88, 0x88, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xF8, 0xF8, 0x3C, 0xBC, 0xFC, 0x68, 0x88, 0xFC, 0x98, 0x78, 0xF8,
        0xF8, 0x78, 0xF8, 0xF8, 0x58, 0x98, 0xF8, 0x78, 0x58, 0xFC, 0xA0, 0x44, 0xF8, 0xB8, 0x00, 0xB8, 0xF8, 0x18,
        0x58, 0xD8, 0x54, 0x58, 0xF8, 0x98, 0x00, 0xE8, 0xD8, 0x78, 0x78, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xFC, 0xFC, 0xFC, 0xA4, 0xE4, 0xFC, 0xB8, 0xB8, 0xF8, 0xD8, 0xB8, 0xF8, 0xF8, 0xB8, 0xF8, 0xF8, 0xA4, 0xC0,
        0xF0, 0xD0, 0xB0, 0xFC, 0xE0, 0xA8, 0xF8, 0xD8, 0x78, 0xD8, 0xF8, 0x78, 0xB8, 0xF8, 0xB8, 0xB8, 0xF8, 0xD8,
        0x00, 0xFC, 0xFC, 0xF8, 0xD8, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    size_t colorIndex = 0;

    for (int i = 0; i < 192; i += 3)
    {
        Colors_[colorIndex] = RGB{paletteArray[i], paletteArray[i + 1], paletteArray[i + 2]};
        ++colorIndex;
    }
}

void PPU::Reset()
{

}

void PPU::Tick()
{
    if (scanline_ < 240)
    {
        VisibleLine();
    }
    else if ((scanline_ == 241) && (dot_ == 1))
    {
        MemMappedRegisters_.PPUSTATUS |= VBLANK_STARTED_MASK;
        SetNMI();
    }
    else if (scanline_ == 261)
    {
        PreRenderLine();
    }

    DotIncrement();
}

bool PPU::FrameReady()
{
    if (frameReady_)
    {
        frameReady_ = false;
        return true;
    }

    return false;
}

char* PPU::GetFrameBuffer()
{
    return frameBuffer_.data();
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
            InternalRegisters_.w = false;
            returnData = MemMappedRegisters_.PPUSTATUS;
            MemMappedRegisters_.PPUSTATUS &= ~VBLANK_STARTED_MASK;
            break;
        case OAMDATA_ADDR:
            returnData = OAM_[MemMappedRegisters_.OAMADDR];
            break;
        case PPUDATA_ADDR:
            if (InternalRegisters_.v < 0x3F00)
            {
                returnData = readBuffer_;
                readBuffer_ = Read(InternalRegisters_.v & VRAM_ADDR_MASK);
            }
            else
            {
                returnData = Read(InternalRegisters_.v & VRAM_ADDR_MASK);
                readBuffer_ = Read(InternalRegisters_.v - 0x1000);
            }

            if (RenderingEnabled() && scanline_ < 240)
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
        addr = 0x2000 + (addr % 0x0008);
    }

    switch (addr)
    {
        case PPUCTRL_ADDR:
            MemMappedRegisters_.PPUCTRL = data;
            InternalRegisters_.t &= 0x73FF;
            InternalRegisters_.t |= ((data & 0x03) << 10);
            SetNMI();
            break;
        case PPUMASK_ADDR:
            MemMappedRegisters_.PPUMASK = data;
            break;
        case OAMADDR_ADDR:
            MemMappedRegisters_.OAMADDR = data;
            break;
        case OAMDATA_ADDR:
            OAM_[MemMappedRegisters_.OAMADDR] = data;
            ++MemMappedRegisters_.OAMADDR;
            break;
        case PPUSCROLL_ADDR:
            if (InternalRegisters_.w)
            {
                InternalRegisters_.t &= 0x0C1F;
                InternalRegisters_.t |= ((data & 0x07) << 12) | ((data & 0xF8) << 2);
                InternalRegisters_.w = false;
            }
            else
            {
                InternalRegisters_.t &= 0x7FE0;
                InternalRegisters_.t |= ((data & 0xF8) >> 3);
                InternalRegisters_.x = (data & 0x07);
                InternalRegisters_.w = true;
            }
            break;
        case PPUADDR_ADDR:
            if (InternalRegisters_.w)
            {
                InternalRegisters_.t &= 0x7F00;
                InternalRegisters_.t |= data;
                InternalRegisters_.v = InternalRegisters_.t;
                InternalRegisters_.w = false;
            }
            else
            {
                InternalRegisters_.t &= 0x00FF;
                InternalRegisters_.t |= ((data & 0x3F) << 8);
                InternalRegisters_.w = true;
            }
            break;
        case PPUDATA_ADDR:
            Write(InternalRegisters_.v & VRAM_ADDR_MASK, data);
            IncrementVRAMAddr();
            break;
        default:
            break;
    }
}

bool PPU::NMI()
{
    if (nmiCpuCheck_)
    {
        nmiCpuCheck_ = false;
        return true;
    }

    return false;
}

uint8_t PPU::Read(uint16_t addr)
{
    if (addr < 0x2000)
    {
        return cartridge_.ReadCHR(addr);
    }
    else if (addr < 0x3F00)
    {
        return VRAM_[cartridge_.NameTableAddress(addr)];
    }
    else
    {
        return PaletteRAM_[PaletteAddress(addr)];
    }
}

void PPU::Write(uint16_t addr, uint8_t data)
{
    if (addr < 0x2000)
    {
        cartridge_.WriteCHR(addr, data);
    }
    else if (addr < 0x3F00)
    {
        VRAM_[cartridge_.NameTableAddress(addr)] = data;
    }
    else
    {
        PaletteRAM_[PaletteAddress(addr)] = data;
    }
}

bool PPU::RenderingEnabled()
{
    constexpr uint8_t renderFlags = SHOW_BACKGROUND_MASK | SHOW_SPITES_MASK;
    return (MemMappedRegisters_.PPUMASK & renderFlags) != 0x00;
}

void PPU::SetNMI()
{
    nmiCpuCheck_ = ((MemMappedRegisters_.PPUCTRL & GENERATE_NMI_MASK) == GENERATE_NMI_MASK) &&
                   ((MemMappedRegisters_.PPUSTATUS & VBLANK_STARTED_MASK) == VBLANK_STARTED_MASK);
}

void PPU::PreRenderLine()
{
    if (RenderingEnabled())
    {
        switch (dot_)
        {
            case 0:
                break;
            case 1:
                MemMappedRegisters_.PPUSTATUS &= ~(VBLANK_STARTED_MASK | SPRITE_0_HIT_MASK);
                BackgroundFetch();
                break;
            case 2 ... 256:
                BackgroundFetch();
                break;
            case 257:
                TransferHorizontalPosition();
                break;
            case 258 ... 279:
                break;
            case 280 ... 304:
                TransferVerticalPosition();
                break;
            case 305 ... 320:
                break;
            case 321 ... 336:
                BackgroundFetch();
                break;
            case 337 ... 340:
                if ((dot_ % 2) == 0)
                {
                    Read(0x2000 | (InternalRegisters_.v & 0x0FFF));
                }
                break;
        }
    }
    else if (dot_ == 1)
    {
        MemMappedRegisters_.PPUSTATUS &= ~(VBLANK_STARTED_MASK | SPRITE_0_HIT_MASK);
    }
}

void PPU::VisibleLine()
{
    if (RenderingEnabled())
    {
        switch (dot_)
        {
            case 0:
                if (!oddFrame_)
                {
                    Read(0x2000 | (InternalRegisters_.v & 0x0FFF));
                }
                break;
            case 1 ... 256:
                CreateBackgroundPixel();
                RenderPixel();
                BackgroundFetch();
                break;
            case 257:
                TransferHorizontalPosition();
                break;
            case 258 ... 279:
                break;
            case 280 ... 320:
                break;
            case 321 ... 336:
                BackgroundFetch();
                break;
            case 337 ... 340:
                if ((dot_ % 2) == 0)
                {
                    Read(0x2000 | (InternalRegisters_.v & 0x0FFF));
                }
                break;
        }
    }
    else
    {
        return;
    }
}

void PPU::IncrementVRAMAddr()
{
    InternalRegisters_.v &= 0x7FFF;

    if ((MemMappedRegisters_.PPUCTRL & INCREMENT_VRAM_MASK) == INCREMENT_VRAM_MASK)
    {
        InternalRegisters_.v += 0x20;
    }
    else
    {
        InternalRegisters_.v += 0x01;
    }
}

void PPU::CoarseXIncrement()
{
    InternalRegisters_.v &= 0x7FFF;

    if ((InternalRegisters_.v & 0x001F) == 0x001F)
    {
        InternalRegisters_.v &= 0xFFE0;
        InternalRegisters_.v ^= 0x0400;
    }
    else
    {
        ++InternalRegisters_.v;
    }
}

void PPU::YIncrement()
{
    InternalRegisters_.v &= 0x7FFF;

    if ((InternalRegisters_.v & 0x7000) != 0x7000)
    {
        InternalRegisters_.v += 0x1000;
    }
    else
    {
        InternalRegisters_.v &= 0x8FFF;
        uint16_t y = (InternalRegisters_.v & 0x03E0) >> 5;

        if (y == 29)
        {
            y = 0;
            InternalRegisters_.v ^= 0x0800;
        }
        else if (y == 31)
        {
            y = 0;
        }
        else
        {
            ++y;
        }

        InternalRegisters_.v = (InternalRegisters_.v & 0xFC1F) | (y << 5);
    }
}

void PPU::DotIncrement()
{
    if (dot_ < 339)
    {
        ++dot_;
    }
    else if (dot_ == 339)
    {
        if (oddFrame_ && (scanline_ == 261) && RenderingEnabled())
        {
            dot_ = 0;
            scanline_ = 0;
            oddFrame_ = !oddFrame_;
        }
        else
        {
            ++dot_;
        }
    }
    else
    {
        dot_ = 0;

        if (scanline_ == 261)
        {
            scanline_ = 0;
            oddFrame_ = !oddFrame_;
        }
        else
        {
            ++scanline_;
        }
    }
}

void PPU::TransferHorizontalPosition()
{
    InternalRegisters_.v = (InternalRegisters_.v & 0x7BE0) | (InternalRegisters_.t & 0x041F);
}

void PPU::TransferVerticalPosition()
{
    InternalRegisters_.v = (InternalRegisters_.v & 0x041F) | (InternalRegisters_.t & 0x7BE0);
}

uint8_t PPU::PaletteAddress(uint16_t addr)
{
    // TODO: Is this correct implementation?
    uint8_t paletteAddr = addr % 0x20;

    if (paletteAddr == 0x10)
    {
        paletteAddr = 0x00;
    }

    return paletteAddr;
}

void PPU::BackgroundFetch()
{
    ShiftRegisters();
    ++backgroundFetchCycle_;

    switch (backgroundFetchCycle_)
    {
        case 2:
            nametableByte_ = Read(0x2000 | (InternalRegisters_.v & 0x0FFF));
            break;
        case 4:
            attributeTableByte_ = Read(0x23C0 |
                                       (InternalRegisters_.v & 0x0C00) |
                                       ((InternalRegisters_.v >> 4) & 0x0038) |
                                       ((InternalRegisters_.v >> 2) & 0x0007));
            break;
        case 6:
            patternTableAddress_ = ((MemMappedRegisters_.PPUCTRL & BACKGROUND_PT_ADDRESS_MASK) << 8) |
                                   (nametableByte_ << 4) |
                                   ((InternalRegisters_.v & 0x7000) >> 12);
            patternTableLowByte_ = Read(patternTableAddress_);
            break;
        case 8:
            patternTableAddress_ |= 0x0008;
            patternTableHighByte_ = Read(patternTableAddress_);
            LoadShiftRegisters();
            CoarseXIncrement();

            if (dot_ == 256)
            {
                YIncrement();
            }

            backgroundFetchCycle_ = 0;
            break;
        default:
            break;
    }
}

void PPU::LoadShiftRegisters()
{
    patternTableShifterHigh_ |= patternTableHighByte_;
    patternTableShifterLow_ |= patternTableLowByte_;

    bool right = ((InternalRegisters_.v & 0x0002) == 0x0002);
    bool bottom = ((InternalRegisters_.v & 0x0040) == 0x0040);

    if (right && bottom)
    {
        attributeTableLatchHigh_ = ((attributeTableByte_ & 0x80) == 0x80);
        attributeTableLatchLow_ = ((attributeTableByte_ & 0x40) == 0x40);
    }
    else if (!right && bottom)
    {
        attributeTableLatchHigh_ = ((attributeTableByte_ & 0x20) == 0x20);
        attributeTableLatchLow_ = ((attributeTableByte_ & 0x10) == 0x10);
    }
    else if (right && !bottom)
    {
        attributeTableLatchHigh_ = ((attributeTableByte_ & 0x08) == 0x08);
        attributeTableLatchLow_ = ((attributeTableByte_ & 0x04) == 0x04);
    }
    else
    {
        attributeTableLatchHigh_ = ((attributeTableByte_ & 0x02) == 0x02);
        attributeTableLatchLow_ = ((attributeTableByte_ & 0x01) == 0x01);
    }
}

void PPU::ShiftRegisters()
{
    patternTableShifterHigh_ <<= 1;
    patternTableShifterLow_ <<= 1;

    attributeTableShifterHigh_ <<= 1;
    attributeTableShifterHigh_ |= (attributeTableLatchHigh_ ? 0x01 : 0x00);

    attributeTableShifterLow_ <<= 1;
    attributeTableShifterLow_ |= (attributeTableLatchLow_ ? 0x01 : 0x00);
}

void PPU::CreateBackgroundPixel()
{
    backgroundPixel_ = 0x3F00;
    uint16_t attributeTableMask = 0x0080 >> InternalRegisters_.x;
    uint16_t patternTableMask = 0x8000 >> InternalRegisters_.x;

    backgroundPixel_ |= (((attributeTableShifterHigh_ & attributeTableMask) == attributeTableMask) ? 0x0008 : 0x0000);
    backgroundPixel_ |= (((attributeTableShifterLow_ & attributeTableMask) == attributeTableMask) ? 0x0004 : 0x0000);
    backgroundPixel_ |= (((patternTableShifterHigh_ & patternTableMask) == patternTableMask) ? 0x0002 : 0x0000);
    backgroundPixel_ |= (((patternTableShifterLow_ & patternTableMask) == patternTableMask) ? 0x0001 : 0x0000);
}

uint8_t PPU::PixelMultiplexer()
{
    return Read(backgroundPixel_);
}

void PPU::RenderPixel()
{
    uint8_t colorIndex = PixelMultiplexer();
    auto rgb = Colors_[colorIndex];

    frameBuffer_[framePointer_++] = rgb.R;
    frameBuffer_[framePointer_++] = rgb.G;
    frameBuffer_[framePointer_++] = rgb.B;

    if (framePointer_ >= 184320)
    {
        frameReady_ = true;
        framePointer_ = 0;
    }
}
