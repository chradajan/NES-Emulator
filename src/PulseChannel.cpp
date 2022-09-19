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
    channelEnabled_ = false;

    dutyCycleIndex_ = 0;
    sequencerIndex_ = 0;

    lengthCounter_ = 0;
    halt_ = false;

    timerReloadLow_ = 0;
    timerReloadHigh_ = 0;
    timerReload_ = 0;
    timer_ = 0;

    silenced_ = true;

    useConstantVolume_ = false;
    constantVolume_ = 0;

    envelopeLooped_ = false;
    envelopeStart_ = false;

    envelopeTimerReload_ = 0;
    envelopeTimer_ = 0;
    decayLevel_ = 0;

    sweepEnabled_ = false;
    negate_ = false;

    sweepTimerReload_ = 0;
    sweepTimer_ = 0;
    reloadSweepTimer_ = 0;

    sweepShiftAmount_ = 0;
    sweepTargetPeriod_ = 0;
}

uint8_t APU::PulseChannel::Output()
{
    if (!DUTY_CYCLE_SEQUENCE[dutyCycleIndex_][sequencerIndex_] || silenced_ || !channelEnabled_)
    {
        return 0;
    }

    return useConstantVolume_ ? constantVolume_ : decayLevel_;
}

void APU::PulseChannel::Toggle(bool enabled)
{
    channelEnabled_ = enabled;

    if (!enabled)
    {
        lengthCounter_ = 0;
    }
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

void APU::PulseChannel::HalfFrameClock()
{
    QuarterFrameClock();
    SweepUpdate();

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

void APU::PulseChannel::QuarterFrameClock()
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

void APU::PulseChannel::RegisterUpdate(uint16_t addr, uint8_t data)
{
    int reg = addr & 0x03;

    switch (reg)
    {
        case 0:  // $4000, $4004
            dutyCycleIndex_ = (data & 0xC0) >> 6;
            halt_ = (data & 0x20) == 0x20;
            envelopeLooped_ = halt_;
            useConstantVolume_ = (data & 0x10) == 0x10;
            constantVolume_ = data & 0x0F;
            envelopeTimerReload_ = constantVolume_;
            break;
        case 1:  // $4001, $4005
            sweepEnabled_ = (data & 0x80) == 0x80;
            sweepTimerReload_ = (data & 0x70) >> 4;
            negate_ = (data & 0x08) == 0x08;
            sweepShiftAmount_ = data & 0x07;
            reloadSweepTimer_ = true;
            break;
        case 2:  // $4002, $4006
            timerReloadLow_ = data;
            SetPeriod();
            break;
        case 3:    // $4003, $4007
            lengthCounter_ = LENGTH_COUNTER_LOOKUP_TABLE[(data & 0xF8) >> 3];
            timerReloadHigh_ = data & 0x07;
            SetPeriod();
            sequencerIndex_ = 0;
            silenced_ = false;
            envelopeStart_ = true;
            break;
    }
}

void APU::PulseChannel::SetPeriod()
{
    timerReload_ = (timerReloadHigh_ << 8) | timerReloadLow_;

    if (timerReload_ < 8)
    {
        silenced_ = true;
    }

    SetTargetPeriod();
}

void APU::PulseChannel::SweepUpdate()
{
    if (sweepEnabled_ && !silenced_ && (sweepTimer_ == 0))
    {
        timerReload_ = sweepTargetPeriod_;
        SetTargetPeriod();
    }

    if (reloadSweepTimer_ || (sweepTimer_ == 0))
    {
        sweepTimer_ = sweepTimerReload_;
        reloadSweepTimer_ = false;
    }
    else
    {
        --sweepTimer_;
    }
}

void APU::PulseChannel::SetTargetPeriod()
{
    uint16_t changeAmount = timerReload_ >> sweepShiftAmount_;

    if (negate_)
    {
        if (negateBehavior_ == NegateBehavior::OnesComplement)
        {
            ++changeAmount;
        }

        if (changeAmount > timerReload_)
        {
            sweepTargetPeriod_ = 0;
        }
        else
        {
            sweepTargetPeriod_ = timerReload_ - changeAmount;
        }
    }
    else
    {
        sweepTargetPeriod_ = timerReload_ + changeAmount;
    }

    if (sweepTargetPeriod_ > 0x7FF)
    {
        silenced_ = true;
    }
}
