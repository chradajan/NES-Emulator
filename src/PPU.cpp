#include "../include/PPU.hpp"
#include "../include/Cartridge.hpp"
#include "../include/RegisterAddresses.hpp"
#include <cstdint>

PPU::PPU(Cartridge& cartridge, char* frameBuffer) :
    cartridge_(cartridge),
    frameBuffer_(frameBuffer)
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

    oddFrame_ = false;
    scanline_ = 0;
    dot_ = 0;
    VRAM_.fill(0x00);
    OAM_.fill(0xFF);
    OAM_Secondary_.fill(0xFF);
    PaletteRAM_.fill(0xFF);
    openBus_ = 0x00;
}

void PPU::Reset()
{

}

void PPU::Tick()
{
    renderingEnabled_ = RenderingEnabled();

    for (size_t i = 0; i < 3; ++i)
    {
        if (scanline_ < 240)
        {
            VisibleLine();

            if ((scanline_ == 239) && (dot_ == 256))
            {
                frameReady_ = true;
                framePointer_ = 0;
            }
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
            returnData |= (openBus_ & 0x1F);
            openBus_ = returnData;
            break;
        case OAMDATA_ADDR:
            returnData = OAM_[MemMappedRegisters_.OAMADDR];
            openBus_ = returnData;
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

            openBus_ = returnData;
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

    openBus_ = data;

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
    return (MemMappedRegisters_.PPUMASK & (SHOW_BACKGROUND_MASK | SHOW_SPRITES_MASK)) != 0x00;
}

void PPU::SetNMI()
{
    nmiCpuCheck_ = ((MemMappedRegisters_.PPUCTRL & GENERATE_NMI_MASK) == GENERATE_NMI_MASK) &&
                   ((MemMappedRegisters_.PPUSTATUS & VBLANK_STARTED_MASK) == VBLANK_STARTED_MASK);
}

void PPU::PreRenderLine()
{
    switch (dot_)
    {
        case 0:
            ResetSpriteEvaluation();
            break;
        case 1:
            MemMappedRegisters_.PPUSTATUS &= ~(VBLANK_STARTED_MASK | SPRITE_0_HIT_MASK);
            BackgroundFetch();
            break;
        case 2 ... 256:
            BackgroundFetch();
            break;
        case 257:
            if (renderingEnabled_)
            {
                TransferHorizontalPosition();
            }
            oamSecondaryIndex_ = 0;
            SpriteFetch();
            break;
        case 258 ... 279:
            SpriteFetch();
            break;
        case 280 ... 304:
            if (renderingEnabled_)
            {
                TransferVerticalPosition();
            }
            SpriteFetch();
            break;
        case 305 ... 320:
            SpriteFetch();
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

void PPU::VisibleLine()
{
    switch (dot_)
    {
        case 0:
            ResetSpriteEvaluation();
            if (!oddFrame_ && renderingEnabled_)
            {
                Read(0x2000 | (InternalRegisters_.v & 0x0FFF));
            }
            break;
        case 1 ... 64:
            CreateBackgroundPixel();
            CreateSpritePixel();
            RenderPixel();
            BackgroundFetch();
            break;
        case 65 ... 256:
            CreateBackgroundPixel();
            CreateSpritePixel();
            RenderPixel();
            BackgroundFetch();
            SpriteEvaluation();
            break;
        case 257:
            if (renderingEnabled_)
            {
                TransferHorizontalPosition();
            }
            oamSecondaryIndex_ = 0;
            SpriteFetch();
            break;
        case 258 ... 320:
            SpriteFetch();
            break;
        case 321 ... 336:
            BackgroundFetch();
            break;
        case 337 ... 340:
            if (renderingEnabled_ && ((dot_ % 2) == 0))
            {
                Read(0x2000 | (InternalRegisters_.v & 0x0FFF));
            }
            break;
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
        ++scanline_;

        if (scanline_ == 262)
        {
            scanline_ = 0;
            oddFrame_ = !oddFrame_;
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
    uint8_t paletteAddr = addr % 0x20;

    switch (paletteAddr)
    {
        case 0x10:
        case 0x14:
        case 0x18:
        case 0x1C:
            paletteAddr -= 0x10;
            break;
    }

    return paletteAddr;
}

void PPU::BackgroundFetch()
{
    ShiftRegisters();
    ++backgroundFetchCycle_;

    if (renderingEnabled_)
    {
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
    else
    {
        switch (backgroundFetchCycle_)
        {
            case 2:
                nametableByte_ = 0x00;
                break;
            case 4:
                attributeTableByte_ = 0x00;
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
                backgroundFetchCycle_ = 0;
                break;
            default:
                break;
        }
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

void PPU::ResetSpriteEvaluation()
{
    oamIndex_ = 0;
    oamOffset_ = 0;
    oamSecondaryIndex_ = 0;
    spritesFound_ = 0;
    sprite0Loaded_ = false;
    spriteState_ = SpriteEvalState::READ;
    spriteIndex_ = 0;
    OAM_Secondary_.fill(0xFF);
}

void PPU::SpriteEvaluation()
{
    switch (spriteState_)
    {
        case SpriteEvalState::READ:
        {
            oamByte_ = OAM_[((oamIndex_ * 4) + oamOffset_)];

            if (spritesFound_ == 8)
            {
                spriteState_ = SpriteEvalState::OVERFLOW;
            }
            else if (oamOffset_ == 0)
            {
                spriteState_ = SpriteEvalState::WRITE_Y;
            }
            else
            {
                spriteState_ = SpriteEvalState::WRITE_DATA;
            }
            break;
        }
        case SpriteEvalState::WRITE_Y:
        {
            OAM_Secondary_[oamSecondaryIndex_] = oamByte_;
            int16_t yOffset = scanline_ - oamByte_;
            uint8_t spriteHeight = ((MemMappedRegisters_.PPUCTRL & SPRITE_SIZE_MASK) == SPRITE_SIZE_MASK) ? 16 : 8;

            if ((yOffset >= 0) && (yOffset < spriteHeight))
            {
                if (oamIndex_ == 0)
                {
                    sprite0Loaded_ = true;
                }

                ++oamOffset_;
                ++oamSecondaryIndex_;
                spriteState_ = SpriteEvalState::READ;
            }
            else
            {
                ++oamIndex_;

                if (oamIndex_ == 64)
                {
                    spriteState_ = SpriteEvalState::FINISHED;
                }
                else
                {
                    spriteState_ = SpriteEvalState::READ;
                }
            }
            break;
        }
        case SpriteEvalState::WRITE_DATA:
        {
            OAM_Secondary_[oamSecondaryIndex_] = oamByte_;
            ++oamOffset_;
            ++oamSecondaryIndex_;

            if (oamOffset_ == 4)
            {
                ++spritesFound_;
                ++oamIndex_;
                oamOffset_ = 0;

                if (oamIndex_ == 64)
                {
                    spriteState_ = SpriteEvalState::FINISHED;
                }
                else
                {
                    spriteState_ = SpriteEvalState::READ;
                }
            }
            else
            {
                spriteState_ = SpriteEvalState::READ;
            }
            break;
        }
        case SpriteEvalState::OVERFLOW:
        {
            int16_t yOffset = scanline_ - oamByte_;
            uint8_t spriteHeight = ((MemMappedRegisters_.PPUCTRL & SPRITE_SIZE_MASK) == SPRITE_SIZE_MASK) ? 16 : 8;

            if ((yOffset >= 0) && (yOffset < spriteHeight))
            {
                MemMappedRegisters_.PPUSTATUS |= SPRITE_OVERFLOW_MASK;
                spriteState_ = SpriteEvalState::FINISHED;
            }
            else
            {
                ++oamIndex_;
                oamOffset_ = (oamOffset_ + 1) % 4;

                if (oamIndex_ == 64)
                {
                    spriteState_ = SpriteEvalState::FINISHED;
                }
                else
                {
                    spriteState_ = SpriteEvalState::READ;
                }
            }
            break;
        }
        case SpriteEvalState::FINISHED:
            break;
    }
}

void PPU::SpriteFetch()
{
    ++spriteFetchCycle_;

    switch (spriteFetchCycle_)
    {
        case 2:
        {
            if (renderingEnabled_)
            {
                Read(0x2000 | (InternalRegisters_.v & 0x0FFF));
            }
            break;
        }
        case 4:
        {
            if (renderingEnabled_)
            {
                Read(0x23C0 |
                     (InternalRegisters_.v & 0x0C00) |
                     ((InternalRegisters_.v >> 4) & 0x0038) |
                     ((InternalRegisters_.v >> 2) & 0x0007));
            }
            break;
        }
        case 6:
        {
            Sprites_[spriteIndex_].y = OAM_Secondary_[oamSecondaryIndex_++];
            Sprites_[spriteIndex_].tile = OAM_Secondary_[oamSecondaryIndex_++];
            Sprites_[spriteIndex_].attributes = OAM_Secondary_[oamSecondaryIndex_++];
            Sprites_[spriteIndex_].x = OAM_Secondary_[oamSecondaryIndex_++];
            Sprites_[spriteIndex_].sprite0 = (sprite0Loaded_ && (spriteIndex_ == 0));

            uint8_t yOffset = scanline_ - Sprites_[spriteIndex_].y;

            if ((MemMappedRegisters_.PPUCTRL & SPRITE_SIZE_MASK) == SPRITE_SIZE_MASK)
            {
                // 8x16 sprite mode

                if ((Sprites_[spriteIndex_].attributes & FLIP_VERTICAL_MASK) == FLIP_VERTICAL_MASK)
                {
                    yOffset ^= 0x0F;
                }

                uint8_t tile = Sprites_[spriteIndex_].tile & LARGE_SPRITE_TILE_MASK;

                if (yOffset >= 8)
                {
                    ++tile;
                }

                yOffset %= 0x08;

                patternTableAddress_ = ((Sprites_[spriteIndex_].tile & BANK_SELECTION_MASK) << 12) | (tile << 4) | yOffset;
            }
            else
            {
                // 8x8 sprite mode

                if ((Sprites_[spriteIndex_].attributes & FLIP_VERTICAL_MASK) == FLIP_VERTICAL_MASK)
                {
                    yOffset ^= 0x07;
                }

                patternTableAddress_ = ((MemMappedRegisters_.PPUCTRL & SPRITE_PT_ADDRESS_MASK) << 9) |
                                       (Sprites_[spriteIndex_].tile << 4) |
                                       yOffset;
            }

            Sprites_[spriteIndex_].patternTableLowByte = Read(patternTableAddress_);
            break;
        }
        case 8:
        {
            patternTableAddress_ |= 0x08;
            Sprites_[spriteIndex_].patternTableHighByte = Read(patternTableAddress_);

            if (spritesFound_ > 0)
            {
                Sprites_[spriteIndex_].valid = true;
                --spritesFound_;
            }
            else
            {
                Sprites_[spriteIndex_].valid = false;
            }

            spriteFetchCycle_ = 0;
            ++spriteIndex_;
            break;
        }
    }
}

void PPU::CreateBackgroundPixel()
{
    backgroundPixelAddr_ = 0x3F00;
    uint16_t attributeTableMask = 0x0080 >> InternalRegisters_.x;
    uint16_t patternTableMask = 0x8000 >> InternalRegisters_.x;

    backgroundPixelAddr_ |= (((attributeTableShifterHigh_ & attributeTableMask) == attributeTableMask) ? 0x0008 : 0x0000);
    backgroundPixelAddr_ |= (((attributeTableShifterLow_ & attributeTableMask) == attributeTableMask) ? 0x0004 : 0x0000);
    backgroundPixelAddr_ |= (((patternTableShifterHigh_ & patternTableMask) == patternTableMask) ? 0x0002 : 0x0000);
    backgroundPixelAddr_ |= (((patternTableShifterLow_ & patternTableMask) == patternTableMask) ? 0x0001 : 0x0000);
}

void PPU::CreateSpritePixel()
{
    spritePixelAddr_ = 0x3F10;

    if (scanline_ == 0)
    {
        return;
    }

    for (int i = 7; i >= 0; --i)
    {
        Sprite& sprite = Sprites_[i];

        if (!sprite.valid)
        {
            continue;
        }

        --sprite.x;

        if ((sprite.x >= -8) && (sprite.x <= -1))
        {
            uint8_t pixelNibble = ((sprite.attributes & SPRITE_PALETTE_MASK) << 2);

            if ((sprite.attributes & FLIP_HORIZONTAL_MASK) == FLIP_HORIZONTAL_MASK)
            {
                pixelNibble |= ((sprite.patternTableHighByte & 0x01) << 1);
                pixelNibble |= (sprite.patternTableLowByte & 0x01);
                sprite.patternTableHighByte >>= 1;
                sprite.patternTableLowByte >>= 1;
            }
            else
            {
                pixelNibble |= ((sprite.patternTableHighByte & 0x80) >> 6);
                pixelNibble |= ((sprite.patternTableLowByte & 0x80) >> 7);
                sprite.patternTableHighByte <<= 1;
                sprite.patternTableLowByte <<= 1;
            }

            if ((pixelNibble & 0x03) != 0x00)
            {
                spritePixelAddr_ = 0x3F10 | pixelNibble;
                checkSprite0Hit_ = sprite.sprite0;
                backgroundPriority_ = ((sprite.attributes & BACKGROUND_PRIORITY_MASK) == BACKGROUND_PRIORITY_MASK);
            }
        }
    }
}

PPU::RGB PPU::PixelMultiplexer()
{
    uint16_t colorAddr = 0x3F00;

    bool showBackground = ((MemMappedRegisters_.PPUMASK & SHOW_BACKGROUND_MASK) == SHOW_BACKGROUND_MASK);
    bool showSprites = ((MemMappedRegisters_.PPUMASK & SHOW_SPRITES_MASK) == SHOW_SPRITES_MASK);
    bool leftBackgroundHidden = ((MemMappedRegisters_.PPUMASK & SHOW_LEFT_BACKGROUND_MASK) != SHOW_LEFT_BACKGROUND_MASK);
    bool leftSpritesHidden = ((MemMappedRegisters_.PPUMASK & SHOW_LEFT_SPRITE_MASK) != SHOW_LEFT_SPRITE_MASK);

    bool opaqueBackground = ((backgroundPixelAddr_ & 0x03) != 0x00);
    bool opaqueSprite = ((spritePixelAddr_ & 0x03) != 0x00);

    if ((leftBackgroundHidden || leftSpritesHidden) && (dot_ < 9))
    {
        if (leftBackgroundHidden && !leftSpritesHidden && showSprites)
        {
            colorAddr = spritePixelAddr_;
        }
        else if (leftSpritesHidden && !leftBackgroundHidden && showBackground)
        {
            colorAddr = backgroundPixelAddr_;
        }
    }
    else if (showBackground && !showSprites)
    {
        colorAddr = backgroundPixelAddr_;
    }
    else if (showSprites && !showBackground)
    {
        colorAddr = spritePixelAddr_;
    }
    else if (showBackground && showSprites)
    {
        if (opaqueBackground && opaqueSprite)
        {
            colorAddr = backgroundPriority_ ? backgroundPixelAddr_ : spritePixelAddr_;

            if (checkSprite0Hit_ && (dot_ > 1) && (dot_ != 255))
            {
                MemMappedRegisters_.PPUSTATUS |= SPRITE_0_HIT_MASK;
            }
        }
        else if (opaqueBackground)
        {
            colorAddr = backgroundPixelAddr_;
        }
        else if (opaqueSprite)
        {
            colorAddr = spritePixelAddr_;
        }
    }

    return Colors_[Read(colorAddr)];
}

void PPU::RenderPixel()
{
    RGB rgb = PixelMultiplexer();

    frameBuffer_[framePointer_++] = rgb.R;
    frameBuffer_[framePointer_++] = rgb.G;
    frameBuffer_[framePointer_++] = rgb.B;
}
