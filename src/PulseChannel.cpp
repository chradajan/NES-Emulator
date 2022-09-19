#include "../include/APU.hpp"
#include <cstdint>

bool DUTY_CYCLE_SEQUENCE[4][8] = {
    {0, 1, 0, 0 ,0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 1, 1, 1, 1, 1}
};

APU::PulseChannel::PulseChannel()
{
    Reset();
}

void APU::PulseChannel::SetNegateBehavior(NegateBehavior negateBehavior)
{
    negateBehavior_ = negateBehavior;
}

void APU::PulseChannel::Reset()
{
    dutyCycleIndex_ = 0;
    sequencerIndex_ = 0;
    lengthCounter_ = 0;
    halt_ = false;
    timerReloadLow_ = 0;
    timerReloadHigh_ = 0;
    timerReload_ = 0;
    timer_ = 0;
    silenced_ = true;
    constantVolume_ = false;
    volume_ = 0;
    sweepEnabled_ = false;
    negate_ = false;
    sweepDividerPeriod_ = 0;
    sweepShiftCount_ = 0;
}

uint8_t APU::PulseChannel::Output()
{
    if (!DUTY_CYCLE_SEQUENCE[dutyCycleIndex_][sequencerIndex_] || !silenced_ || (timer_ < 8))
    {
        return 0x00;
    }

    return volume_;
}

void APU::PulseChannel::Silence()
{
    lengthCounter_ = 0;
    silenced_ = true;
}

void APU::PulseChannel::Clock()
{
    if (timer_ == 0)
    {
        timer_ = timerReload_;
        sequencerIndex_ = (sequencerIndex_ + 1) % 8;
    }
    else
    {
        --timer_;
    }
}

void APU::PulseChannel::FrameCounterClock()
{
    if (halt_)
    {
        return;
    }
    
    if (lengthCounter_ == 0)
    {
        silenced_ = true;
    }
    else
    {
        --lengthCounter_;
    }
}

void APU::PulseChannel::RegisterUpdate(uint16_t addr, uint8_t data)
{
    int reg = addr & 0x03;

    switch (reg)
    {
        case 0:  // $4000, $4004
            dutyCycleIndex_ = (data & 0xC0) >> 6;
            halt_ = (data & 0x20) == 0x20;
            constantVolume_ = (data & 0x10) == 0x10;
            volume_ = data & 0x0F;
            break;
        case 1:  // $4001, $4005
            sweepEnabled_ = (data & 0x80) == 0x80;
            sweepDividerPeriod_ = (data & 0x70) >> 4;
            negate_ = (data & 0x08) == 0x08;
            sweepShiftCount_ = data & 0x07;
            break;
        case 2:  // $4002, $4006
            timerReloadLow_ = data;
            timerReload_ = (timerReloadHigh_ << 8) | timerReloadLow_;
            break;
        case 3:    // $4003, $4007
            lengthCounter_ = (data & 0xF8) >> 3;
            timerReloadHigh_ = data & 0x07;
            timerReload_ = (timerReloadHigh_ << 8) | timerReloadLow_;
            sequencerIndex_ = 0;
            silenced_ = (lengthCounter_ == 0);
            break;
    }
}
