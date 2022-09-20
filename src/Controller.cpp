#include "../include/Controller.hpp"
#include "../include/RegisterAddresses.hpp"
#include <cstdint>

Controller::Controller()
{

}

void Controller::SetControllerInputs(uint8_t controller1, uint8_t controller2)
{
    controller1_ = controller1;
    controller2_ = controller2;
}

uint8_t Controller::ReadReg(uint16_t addr)
{
    busData_ = addr;
    uint8_t controllerReading = 0x00;

    if (addr == JOY1_ADDR)
    {
        if (strobeLatch_)
        {
            controllerReading = controller1_ & 0x01;
        }
        else
        {
            controllerReading |= (latchedController1_ & 0x01);
            latchedController1_ >>= 1;
            latchedController1_ |= 0x80;
        }
    }
    else if (addr == JOY2_ADDR)
    {
        if (strobeLatch_)
        {
            controllerReading = controller2_ & 0x01;
        }
        else
        {
            controllerReading |= (latchedController2_ & 0x01);
            latchedController2_ >>= 1;
            latchedController2_ |= 0x80;
        }
    }

    controllerReading |= ((busData_ & BUS_MASK) >> 8);
    return controllerReading;
}

void Controller::WriteReg(uint8_t data)
{
    if ((data & LATCH_MASK) == LATCH_MASK)
    {
        strobeLatch_ = true;
    }
    else
    {
        strobeLatch_ = false;
        latchedController1_ = controller1_;
        latchedController2_ = controller2_;
    }
}
