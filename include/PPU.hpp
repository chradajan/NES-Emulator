#ifndef PPU_HPP
#define PPU_HPP

#include <array>
#include <cstdint>
#include <memory>

// PPUCTRL $2000

constexpr uint8_t BASE_NT_ADDRESS_MASK = 0x03;
constexpr uint8_t INCREMENT_VRAM_MASK = 0x04;
constexpr uint8_t SPRITE_PT_ADDRESS_MASK = 0x08;
constexpr uint8_t BACKGROUND_PT_ADDRESS_MASK = 0x10;
constexpr uint8_t SPRITE_SIZE_MASK = 0x20;
constexpr uint8_t MASTER_SLAVE_SELECT_MASK = 0x40;
constexpr uint8_t GENERATE_NMI_MASK = 0x80;

// PPUMASK $2001

constexpr uint8_t GREYSCALE_MASK = 0x01;
constexpr uint8_t SHOW_LEFT_BACKGROUND_MASK = 0x02;
constexpr uint8_t SHOW_LEFT_SPRITE_MASK = 0x04;
constexpr uint8_t SHOW_BACKGROUND_MASK = 0x08;
constexpr uint8_t SHOW_SPITES_MASK = 0x10;
constexpr uint8_t EMPHASIZE_RED_MASK = 0x20;
constexpr uint8_t EMPHASIZE_GREEN_MASK = 0x40;
constexpr uint8_t EMPHASIZE_BLUE_MASK = 0x80;

// PPUSTATUS $2002

constexpr uint8_t PPU_OPEN_BUS_MASK = 0x1F;
constexpr uint8_t SPRITE_OVERFLOW_MASK = 0x20;
constexpr uint8_t SPRITE_0_HIT_MASK = 0x40;
constexpr uint8_t VBLANK_STARTED_MASK = 0x80;

// Other Masks
constexpr uint16_t VRAM_ADDR_MASK = 0x3FFF;

class Cartridge;

class PPU
{
public:
    PPU(Cartridge& cartridge);
    ~PPU() = default;
    void Reset();

    void Tick();

    uint8_t ReadReg(uint16_t addr);
    void WriteReg(uint16_t addr, uint8_t data);

    bool NMI();

private:
    uint8_t Read(uint16_t addr);
    void Write(uint16_t addr, uint8_t data);

    bool RenderingEnabled();

// Scanlines
private:
    void PreRenderLine();
    void VisibleLine();

// Pointer Increments
private:
    void IncrementVRAMAddr();
    void CoarseXIncrement();
    void YIncrement();
    void DotIncrement();

    uint8_t PaletteAddress(uint16_t addr);

// Other components
private:
    Cartridge& cartridge;

// Registers
private:
    struct
    {
        uint16_t v; // Current VRAM address
        uint16_t t; // Temporary VRAM address
        uint8_t x;  // Fine X scroll
        bool w;     // Write toggle
    } InternalRegisters;

    struct
    {
        uint8_t PPUCTRL;    // $2000
        uint8_t PPUMASK;    // $2001
        uint8_t PPUSTATUS;  // $2002
        uint8_t OAMADDR;    // $2003
    } MemMappedRegisters;

    uint8_t readBuffer;

// Memory
private:
    std::array<uint8_t, 0x0100> OAM;
    std::array<uint8_t, 0x0800> VRAM;
    std::array<uint8_t, 0x0020> PaletteRAM;

// Frame state
private:
    uint16_t scanline;
    uint16_t dot;
    bool oddFrame;
};

#endif
