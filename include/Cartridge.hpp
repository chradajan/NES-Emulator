#ifndef CARTRIDGE_HPP
#define CARTRIDGE_HPP

#include <cstdint>
#include <fstream>

// iNES Header Flags

// Flags 6
constexpr uint8_t VERTICAL_MIRRORING_FLAG = 0x01;
constexpr uint8_t BATTERY_BACKED_PRG_RAM = 0x02;
constexpr uint8_t TRAINER_DATA = 0x04;
constexpr uint8_t IGNORE_MIRRORING_CONTROL = 0x08;
constexpr uint8_t LOWER_MAPPER_NYBBLE = 0xF0;

// Flags 7
constexpr uint8_t VS_UNISYSTEM = 0x01;
constexpr uint8_t PLAYCHOICE_10 = 0x02;
constexpr uint8_t NES_2_0_FORMAT = 0x0C;
constexpr uint8_t UPPER_MAPPER_NYBBLE = 0xF0;

// Flags 9
constexpr uint8_t TV_SYSTEM = 0x01;

// Flags 10
constexpr uint8_t TV_SYSTEM_UNOFFICIAL = 0x03;
constexpr uint8_t PRG_RAM_PRESENT = 0x10;
constexpr uint8_t BUS_CONFLICT = 0x20;

enum class MirrorType : uint8_t {HORIZONTAL, VERTICAL, SINGLE_LOW, SINGLE_HIGH, QUAD};

class Cartridge
{
public:
    virtual ~Cartridge() {}

    virtual void Reset() = 0;

    virtual uint8_t ReadPRG(uint16_t addr) = 0;
    virtual void WritePRG(uint16_t addr, uint8_t data) = 0;

    virtual uint8_t ReadCHR(uint16_t addr) = 0;
    virtual void WriteCHR(uint16_t addr, uint8_t data) = 0;

    MirrorType GetMirrorType() { return mirrorType_; };
    virtual void SaveRAM() = 0;
    virtual bool IRQ() = 0;

    virtual void Serialize(std::ofstream& saveState) = 0;
    virtual void Deserialize(std::ifstream& saveState) = 0;

protected:
    MirrorType mirrorType_;
    bool chrRamMode_;
    virtual void LoadROM(std::ifstream& rom, size_t prgRomBanks, size_t chrRomBanks) = 0;
};

#endif
