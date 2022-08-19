#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include <cstdint>

constexpr uint8_t LATCH_MASK = 0x01;
constexpr uint16_t BUS_MASK = 0xE000;

class Controller
{
public:
    Controller();
    ~Controller() = default;

    uint8_t ReadReg(uint16_t addr);
    void WriteReg(uint8_t data);

private:
    bool strobeLatch_;
    uint8_t controller1_;
    uint8_t controller2_;
    uint16_t busData_;

    void ReadController1();
    void ReadController2();
};

#endif
