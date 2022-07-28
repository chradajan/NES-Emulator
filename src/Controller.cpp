#include "../include/Controller.hpp"
#include "../include/RegisterAddresses.hpp"
#include <cstdint>

Controller::Controller()
{

}

uint8_t Controller::ReadReg(uint16_t addr)
{
    (void)addr;
    return 0x00;
}

void Controller::WriteReg(uint8_t data)
{
    (void)data;
}
