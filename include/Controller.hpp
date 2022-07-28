#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include <cstdint>

constexpr uint8_t LATCH_FLAG = 0x01;

class Controller
{
public:
    Controller();
    ~Controller() = default;

    uint8_t ReadReg(uint16_t addr);
    void WriteReg(uint8_t data);
};

#endif
