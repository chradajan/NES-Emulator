#include "../include/DmcChannel.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>

DmcChannel::DmcChannel()
{
    Reset();
}

void DmcChannel::Reset()
{
    irqEnabled_ = false;
    irq_ = false;

    timerReload_ = 0;
    timer_ = 0;

    shiftReg_ = 0;
    bitsRemaining_ = 0;
    outputLevel_ = 0;
    silence_ = true;

    sampleAddress_ = 0xC000;
    currentAddress_ = 0;
    sampleLength_ = 0;
    bytesRemaining_ = 0;
    sampleBuffer_ = 0;
    sampleBufferLoaded_ = false;
    loop_ = false;
}

uint8_t DmcChannel::GetOutput()
{
    return outputLevel_;
}

void DmcChannel::SetEnabled(bool enabled)
{
    irq_ = false;

    if (!enabled)
    {
        bytesRemaining_ = 0;
    }
    else if (bytesRemaining_ == 0)
    {
        currentAddress_ = sampleAddress_;
        bytesRemaining_ = sampleLength_;
    }
}

void DmcChannel::Clock()
{
    if (timer_ == 0)
    {
        if (!silence_)
        {
            if (((shiftReg_ & 0x01) == 0x01) && (outputLevel_ <= 125))
            {
                if (outputLevel_ <= 125)
                {
                    outputLevel_ += 2;
                }
            }
            else
            {
                if (outputLevel_ >= 2)
                {
                    outputLevel_ -= 2;
                }
            }
        }

        shiftReg_ >>= 1;

        if (bitsRemaining_ > 0)
        {
            --bitsRemaining_;
        }

        if (bitsRemaining_ == 0)
        {
            bitsRemaining_ = 8;
            silence_ = !sampleBufferLoaded_;

            if (sampleBufferLoaded_)
            {
                shiftReg_ = sampleBuffer_;
                sampleBufferLoaded_ = false;
            }
        }

        timer_ = timerReload_;
    }
    else
    {
        --timer_;
    }
}

void DmcChannel::RegisterUpdate(uint16_t addr, uint8_t data)
{
    int reg = addr & 0x03;

    switch (reg)
    {
        case 0:  // $4010
            irqEnabled_ = (data & 0x80) == 0x80;
            loop_ = (data & 0x40) == 0x40;
            timerReload_ = RATE_LOOKUP_TABLE[data & 0x0F] / 2;
            break;
        case 1:  // $4011
            outputLevel_ = (data & 0x7F);
            break;
        case 2:  // $4012
            sampleAddress_ = 0xC000 | (data << 6);
            break;
        case 3:  // $4013
            sampleLength_ = (data << 4) | 0x01;
            break;
    }
}

std::optional<uint16_t> DmcChannel::RequestSample()
{
    if (!sampleBufferLoaded_ && (bytesRemaining_ > 0))
    {
        return currentAddress_;
    }

    return {};
}

void DmcChannel::SetSample(uint8_t sample)
{
    sampleBuffer_ = sample;
    sampleBufferLoaded_ = true;
    currentAddress_ = (currentAddress_ == 0xFFFF) ? 0x8000 : (currentAddress_ + 1);
    --bytesRemaining_;

    if (bytesRemaining_ == 0)
    {
        if (loop_)
        {
            currentAddress_ = sampleAddress_;
            bytesRemaining_ = sampleLength_;
        }
        else if (irqEnabled_)
        {
            irq_ = true;
        }
    }
}
