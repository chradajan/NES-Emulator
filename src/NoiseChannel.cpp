#include "../include/APU.hpp"
#include <cstdint>

int NOISE_TIMER_LOOKUP_TABLE[16] = {4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068};

APU::NoiseChannel::NoiseChannel()
{
    Reset();
}

void APU::NoiseChannel::Reset()
{
    channelEnabled_ = false;

    mode_ = false;
    shiftRegister_ = 0x0001;

    lengthCounter_ = 0;
    halt_ = false;

    useConstantVolume_ = false;
    constantVolume_ = 0;

    envelopeLooped_ = false;
    envelopeStart_ = false;
    envelopeTimerReload_ = 0;
    envelopeTimer_ = 0;
    decayLevel_ = 0;

    timerReload_ = 0;
    timer_ = 0;
}

uint8_t APU::NoiseChannel::Output()
{
    if (((shiftRegister_ & 0x01) == 0x01) || (lengthCounter_ == 0))
    {
        return 0x00;
    }

    return useConstantVolume_ ? constantVolume_ : decayLevel_;
}

void APU::NoiseChannel::Toggle(bool enabled)
{
    channelEnabled_ = enabled;

    if (!enabled)
    {
        lengthCounter_ = 0;
    }
}

void APU::NoiseChannel::Clock()
{
    if (timer_ == 0)
    {
        timer_ = timerReload_;

        uint16_t feedbackXorBit = mode_ ? ((shiftRegister_ & 0x0040) >> 6) : ((shiftRegister_ & 0x0002) >> 1);
        bool feedback = ((shiftRegister_ & 0x01) ^ feedbackXorBit) == 0x0001;
        shiftRegister_ >>= 1;

        if (feedback)
        {
            shiftRegister_ |= 0x4000;
        }
    }
    else
    {
        --timer_;
    }
}

void APU::NoiseChannel::HalfFrameClock()
{
    QuarterFrameClock();

    if (!halt_ && (lengthCounter_ > 0))
    {
        --lengthCounter_;
    }
}

void APU::NoiseChannel::QuarterFrameClock()
{
    if (envelopeStart_)
    {
        envelopeStart_ = false;
        decayLevel_ = 15;
        envelopeTimer_ = envelopeTimerReload_;
    }
    else if (envelopeTimer_ == 0)
    {
        envelopeTimer_ = envelopeTimerReload_;

        if (decayLevel_ == 0)
        {
            decayLevel_ = envelopeLooped_ ? 15 : 0;
        }
        else
        {
            --decayLevel_;
        }
    }
    else
    {
        --envelopeTimer_;
    }
}

void APU::NoiseChannel::RegisterUpdate(uint16_t addr, uint8_t data)
{
    int reg = addr & 0x03;

    switch (reg)
    {
        case 0:  // $400C
            halt_ = (data & 0x20) == 0x20;
            envelopeLooped_ = halt_;
            useConstantVolume_ = (data & 0x10) == 0x10;
            constantVolume_ = data & 0x0F;
            envelopeTimerReload_ = constantVolume_;
            break;
        case 2:  // $400E
            mode_ = (data & 0x80) == 0x80;
            timerReload_ = NOISE_TIMER_LOOKUP_TABLE[data & 0x0F];
            break;
        case 3:    // $400F
            lengthCounter_ = channelEnabled_ ? LENGTH_COUNTER_LOOKUP_TABLE[(data & 0xF8) >> 3] : 0;
            envelopeStart_ = true;
            break;
    }
}
