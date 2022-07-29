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

class Cartridge
{
public:
    virtual ~Cartridge() {}

    virtual uint8_t ReadPRG(uint16_t addr) = 0;
    virtual void WritePRG(uint16_t addr, uint8_t data) = 0;

    virtual uint8_t ReadCHR(uint16_t addr) = 0;
    virtual void WriteCHR(uint16_t addr, uint8_t data) = 0;

    virtual uint16_t NameTableAddress(uint16_t addr) = 0;

protected:
    enum class MirrorType {HORIZONTAL, VERTICAL, SINGLE, QUAD};

protected:
    MirrorType mirrorType;
    virtual void LoadROM(std::ifstream& rom) = 0;
};

#endif
