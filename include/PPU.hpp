#ifndef PPU_HPP
#define PPU_HPP

#include <array>
#include <cstdint>
#include <fstream>
#include <utility>

// PPUCTRL $2000

constexpr uint8_t BASE_NT_ADDRESS_MASK = 0x03;
constexpr uint8_t INCREMENT_VRAM_MASK = 0x04;
constexpr uint8_t SPRITE_PT_ADDRESS_MASK = 0x08;
constexpr uint8_t BACKGROUND_PT_ADDRESS_MASK = 0x10;
constexpr uint8_t SPRITE_SIZE_MASK = 0x20;
constexpr uint8_t MASTER_SLAVE_SELECT_MASK = 0x40;
constexpr uint8_t GENERATE_NMI_MASK = 0x80;

// PPUMASK $2001

constexpr uint8_t GRAYSCALE_MASK = 0x01;
constexpr uint8_t SHOW_LEFT_BACKGROUND_MASK = 0x02;
constexpr uint8_t SHOW_LEFT_SPRITE_MASK = 0x04;
constexpr uint8_t SHOW_BACKGROUND_MASK = 0x08;
constexpr uint8_t SHOW_SPRITES_MASK = 0x10;
constexpr uint8_t EMPHASIZE_RED_MASK = 0x20;
constexpr uint8_t EMPHASIZE_GREEN_MASK = 0x40;
constexpr uint8_t EMPHASIZE_BLUE_MASK = 0x80;
constexpr uint8_t COLOR_EMPHASIS_MASK = EMPHASIZE_BLUE_MASK | EMPHASIZE_GREEN_MASK | EMPHASIZE_RED_MASK;

// PPUSTATUS $2002

constexpr uint8_t PPU_OPEN_BUS_MASK = 0x1F;
constexpr uint8_t SPRITE_OVERFLOW_MASK = 0x20;
constexpr uint8_t SPRITE_0_HIT_MASK = 0x40;
constexpr uint8_t VBLANK_STARTED_MASK = 0x80;

// Sprite Attributes
constexpr uint8_t BANK_SELECTION_MASK = 0x01;
constexpr uint8_t SPRITE_PALETTE_MASK = 0x03;
constexpr uint8_t BACKGROUND_PRIORITY_MASK = 0x20;
constexpr uint8_t FLIP_HORIZONTAL_MASK = 0x40;
constexpr uint8_t FLIP_VERTICAL_MASK = 0x80;
constexpr uint8_t LARGE_SPRITE_TILE_MASK = 0xFE;

// Other masks
constexpr uint16_t VRAM_ADDR_MASK = 0x3FFF;

class Cartridge;
class MMC3;

class PPU
{
public:
    PPU(uint8_t* frameBuffer, std::ifstream& normalColors, std::ifstream& grayscaleColors);
    ~PPU() = default;
    void Reset();

    void Clock();
    bool FrameReady();

    uint8_t ReadReg(uint16_t addr);
    void WriteReg(uint16_t addr, uint8_t data);

    bool NMI();
    std::pair<uint16_t, uint16_t> GetState();

    void LoadCartridge(Cartridge* cartridge);
    void SetOverscan(bool enabled);

public:
    bool Serializable();
    void Serialize(std::ofstream& saveState);
    void Deserialize(std::ifstream& saveState);

private:
    void Initialize();
    void InitializePalettes(std::ifstream& normalColors, std::ifstream& grayscaleColors);

private:
    uint8_t Read(uint16_t addr);
    void Write(uint16_t addr, uint8_t data);

    bool RenderingEnabled();
    void SetNMI();
    void RunAhead();

// Memory
private:
    std::array<uint8_t, 0x0100> OAM_;
    std::array<uint8_t, 0x0020> OAM_Secondary_;
    std::array<uint8_t, 0x1000> VRAM_;  // Double the size of actual PPU VRAM. Upper half ignored unless 4-screen mirroring is used.
    std::array<uint8_t, 0x0020> PaletteRAM_;

// Palettes
private:
    struct RGB
    {
        uint8_t R;
        uint8_t G;
        uint8_t B;
    };

    std::array<std::array<RGB, 0x40>, 8> NormalColors_;
    std::array<std::array<RGB, 0x40>, 8> GrayscaleColors_;
    size_t paletteIndex_;
    bool useGrayscale_;

// Scanlines
private:
    void PreRenderLine();
    void VisibleLine();

// Pointer increments
private:
    void IncrementVRAMAddr();
    void CoarseXIncrement();
    void YIncrement();
    void DotIncrement();
    void TransferHorizontalPosition();
    void TransferVerticalPosition();

    uint8_t PaletteAddress(uint16_t addr);

// Background fetch
private:
    size_t backgroundFetchCycle_;
    uint8_t nametableByte_;
    uint8_t attributeTableByte_;
    uint16_t patternTableAddress_;
    uint8_t patternTableLowByte_;
    uint8_t patternTableHighByte_;

    uint16_t patternTableShifterHigh_;
    uint16_t patternTableShifterLow_;
    uint8_t attributeTableShifterHigh_;
    uint8_t attributeTableShifterLow_;
    bool attributeTableLatchHigh_;
    bool attributeTableLatchLow_;

    void BackgroundFetch();
    void LoadShiftRegisters();
    void ShiftRegisters();
    uint16_t NameTableAddress(uint16_t addr);

// Sprite evaluation
private:
    size_t oamIndex_;
    size_t oamOffset_;
    size_t oamSecondaryIndex_;
    uint8_t oamByte_;
    size_t spritesFound_;
    bool sprite0Loaded_;

    enum class SpriteEvalState
    {
        READ,
        WRITE_Y,
        WRITE_DATA,
        SPRITE_OVERFLOW,
        FINISHED,
    };
    SpriteEvalState spriteState_;

    void ResetSpriteEvaluation();
    void SpriteEvaluation();

// Sprite fetch
private:
    size_t spriteFetchCycle_;
    size_t spriteIndex_;
    bool checkSprite0Hit_;

    struct Sprite
    {
        uint8_t y;
        uint8_t tile;
        uint8_t attributes;
        int16_t x;

        bool sprite0;
        bool valid;
        uint8_t patternTableLowByte;
        uint8_t patternTableHighByte;
    };
    std::array<Sprite, 8> Sprites_;
    void SpriteFetch();

// Pixel retrieval
private:
    uint16_t backgroundPixelAddr_;
    uint16_t spritePixelAddr_;
    bool backgroundPriority_;

    void CreateBackgroundPixel();
    void CreateSpritePixel();
    RGB PixelMultiplexer();
    void RenderPixel();

// Other components
private:
    Cartridge* cartridge_;

// Registers
private:
    struct
    {
        uint16_t v; // Current VRAM address
        uint16_t t; // Temporary VRAM address
        uint8_t x;  // Fine X scroll
        bool w;     // Write toggle
    } InternalRegisters_;

    struct
    {
        uint8_t PPUCTRL;    // $2000
        uint8_t PPUMASK;    // $2001
        uint8_t PPUSTATUS;  // $2002
        uint8_t OAMADDR;    // $2003
    } MemMappedRegisters_;

    uint8_t readBuffer_;

// Frame state
private:
    size_t scanline_;
    size_t dot_;
    bool oddFrame_;
    uint8_t openBus_;
    bool renderingEnabled_;
    bool runAhead_;

// NMI
private:
    bool nmiCpuCheck_;
    bool suppressVblFlag_;
    bool ignoreNextNmiCheck_;

// Frame buffer
private:
    bool frameReady_;
    uint8_t* frameBuffer_;
    size_t framePointer_;
    bool overscan_;

// Special mappers
private:
    MMC3* mmc3Cart_;

    void SetCartType();
};

#endif
