#ifndef CARTRIDGE_HPP
#define CARTRIDGE_HPP

#include <cstdint>
#include <fstream>

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