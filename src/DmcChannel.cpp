#include "../include/DmcChannel.hpp"
#include <cstddef>
#include <cstdint>

DmcChannel::DmcChannel()
{
    Reset();
}

void DmcChannel::Reset()
{
    channelEnabled_ = false;
    irqEnabled_ = false;
    loop_ = false;
    irq_ = false;

    bytesRemaining_ = 0;
}

uint16_t DmcChannel::GetOutput()
{
    return 0;
}

void DmcChannel::SetEnabled(bool enabled)
{
    channelEnabled_ = enabled;
}

void DmcChannel::Clock()
{

}

void DmcChannel::RegisterUpdate(uint16_t addr, uint8_t data)
{
    int reg = addr & 0x03;

    switch (reg)
    {
        case 0:  // $4010
            break;
        case 1:  // $4011
            break;
        case 2:  // $4012
            break;
        case 3:  // $4013
            break;
    }
}
