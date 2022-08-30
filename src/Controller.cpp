#include "../include/Controller.hpp"
#include "../include/RegisterAddresses.hpp"
#include <cstdint>
#include <SDL2/SDL.h>

Controller::Controller()
{

}

uint8_t Controller::ReadReg(uint16_t addr)
{
    busData_ = addr;
    uint8_t controllerReading = 0x00;

    if (addr == JOY1_ADDR)
    {
        if (strobeLatch_)
        {
            uint8_t const* currentKeyStates = SDL_GetKeyboardState(NULL);
            controllerReading = (currentKeyStates[SDL_SCANCODE_L] ? 0x01 : 0x00);
        }
        else
        {
            controllerReading |= (controller1_ & 0x01);
            controller1_ >>= 1;
            controller1_ |= 0x80;
        }
    }
    else if (addr == JOY2_ADDR)
    {
        // TODO: Implement controller 2 inputs.
    }

    controllerReading |= ((busData_ & BUS_MASK) >> 8);
    return controllerReading;
}

void Controller::WriteReg(uint8_t data)
{
    if ((data & LATCH_MASK) == LATCH_MASK)
    {
        strobeLatch_ = true;
        controller1_ = 0x00;
        controller2_ = 0x00;
    }
    else
    {
        strobeLatch_ = false;
        ReadController1();
        ReadController2();
    }
}

void Controller::ReadController1()
{
    uint8_t const* currentKeyStates = SDL_GetKeyboardState(NULL);
    controller1_ |= currentKeyStates[SDL_SCANCODE_L] ? 0x01 : 0x00; // A
    controller1_ |= currentKeyStates[SDL_SCANCODE_K] ? 0x02 : 0x00; // B
    controller1_ |= currentKeyStates[SDL_SCANCODE_O] ? 0x04 : 0x00; // SELECT
    controller1_ |= currentKeyStates[SDL_SCANCODE_P] ? 0x08 : 0x00; // START
    controller1_ |= currentKeyStates[SDL_SCANCODE_W] ? 0x10 : 0x00; // UP
    controller1_ |= currentKeyStates[SDL_SCANCODE_S] ? 0x20 : 0x00; // DOWN
    controller1_ |= currentKeyStates[SDL_SCANCODE_A] ? 0x40 : 0x00; // LEFT
    controller1_ |= currentKeyStates[SDL_SCANCODE_D] ? 0x80 : 0x00; // RIGHT
}

void Controller::ReadController2()
{

}
