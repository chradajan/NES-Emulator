#include "../include/APU.hpp"
#include "../include/RegisterAddresses.hpp"
#include <cstdint>

// APU

APU::APU()
{
    pulseTable_[0] = 0.0;

    for (size_t n = 1; n < 31; ++n)
    {
        pulseTable_[n] = 95.52 / ((8128.0 / n) + 100);
    }

    tndTable_[0] = 0.0;

    for (size_t n = 1; n < 203; ++n)
    {
        tndTable_[n] = 163.67 / ((24329.0 / n) + 100);
    }

    irq_ = false;
    clockAPU_ = false;
    frameCounterMode_ = false;
    irqInhibit_ = true;
    frameCounterTimer_ = 0;
    frameCounterResetCountdown_ = 0;
}

void APU::Clock()
{
    clockAPU_ = !clockAPU_;

    if (frameCounterResetCountdown_ > 0)
    {
        --frameCounterResetCountdown_;

        if (frameCounterResetCountdown_ == 0)
        {
            frameCounterTimer_ = 0;
        }
    }

    if (!clockAPU_)
    {
        return;
    }

    pulseChannel_[0].Clock();
    pulseChannel_[1].Clock();

    ++frameCounterTimer_;

    switch (frameCounterTimer_)
    {
        case 3728:
            break;
        case 7456:
            pulseChannel_[0].ClockLengthCounter();
            pulseChannel_[1].ClockLengthCounter();
            break;
        case 11185:
            break;
        case 14915:
            if (!frameCounterMode_)
            {
                if (!irqInhibit_)
                {
                    irq_ = true;
                }

                pulseChannel_[0].ClockLengthCounter();
                pulseChannel_[1].ClockLengthCounter();
                frameCounterResetCountdown_ = 1;
            }
            break;
        case 18641:
            pulseChannel_[0].ClockLengthCounter();
            pulseChannel_[1].ClockLengthCounter();
            frameCounterResetCountdown_ = 1;
            break;
        default:
            break;
    }
}

void APU::Reset()
{

}

float APU::Output()
{
    return pulseTable_[pulseChannel_[0].Output() + pulseChannel_[1].Output()];
}

uint8_t APU::ReadReg(uint16_t addr)
{
    (void)addr;
    return 0x00;
}

void APU::WriteReg(uint16_t addr, uint8_t data)
{
    switch (addr)
    {
        case SQ1_VOL_ADDR:
        case SQ1_SWEEP_ADDR:
        case SQ1_LO_ADDR:
        case SQ1_HI_ADDR:
        case SQ2_VOL_ADDR:
        case SQ2_SWEEP_ADDR:
        case SQ2_LO_ADDR:
        case SQ2_HI_ADDR:
            pulseChannel_[(addr & 0x0004) >> 2].RegUpdate(addr, data);
            break;
        case SND_CHN_ADDR:
            if ((data & ENABLE_PULSE1) != ENABLE_PULSE1)
            {
                pulseChannel_[1].Silence();
            }

            if ((data & ENABLE_PULSE0) != ENABLE_PULSE0)
            {
                pulseChannel_[0].Silence();
            }
            break;
        case FRAME_COUNTER_ADDR:
            frameCounterMode_ = (data & FRAME_COUNTER_SEQUENCER_MODE_MASK) == FRAME_COUNTER_SEQUENCER_MODE_MASK;
            irqInhibit_ = (data & FRAME_COUNTER_INTERRUPT_MASK) == FRAME_COUNTER_INTERRUPT_MASK;

            if (clockAPU_)
            {
                // Write occurred between APU cycles.
                frameCounterResetCountdown_ = 5;
            }
            else
            {
                // Write occurred during APU cycle.
                frameCounterResetCountdown_ = 4;
            }

            if (frameCounterMode_)
            {
                pulseChannel_[0].Clock();
                pulseChannel_[1].Clock();
            }
            break;
    }
}

// PulseChannel

APU::PulseChannel::PulseChannel()
{
    dutyCycleIndex_ = 0;
    sequencerIndex_ = 0;
    halt_ = false;
    constantVolume_ = false;
    volume_ = 0;
    sweep_ = false;
    sweepDividerPeriod_ = 0;
    negate_ = false;
    sweepShiftCount_ = false;
    timerReloadLow_ = 0;
    timerReloadHigh_ = 0;
    timer_ = 0;
    lengthCounter_ = 0;
    silence_ = true;
}

void APU::PulseChannel::Reset()
{

}

void APU::PulseChannel::Clock()
{
    if (timer_ == 0)
    {
        timer_ = (timerReloadHigh_ << 8) | timerReloadLow_;
        sequencerIndex_ = (sequencerIndex_ + 1) % 8;
    }
    else
    {
        --timer_;
    }
}

void APU::PulseChannel::ClockLengthCounter()
{
    if (halt_)
    {
        return;
    }
    else if (lengthCounter_ == 0)
    {
        silence_ = true;
    }
    else if (lengthCounter_ > 0)
    {
        --lengthCounter_;
    }
}

uint8_t APU::PulseChannel::Output()
{
    if (!dutyCycleSequence_[dutyCycleIndex_][sequencerIndex_] || !silence_ || (timer_ < 8))
    {
        return 0x00;
    }

    return volume_;
}

void APU::PulseChannel::RegUpdate(uint16_t addr, uint8_t data)
{
    int reg = addr & 0x03;

    switch (reg)
    {
        case 0:
            dutyCycleIndex_ = (data & PULSE_DUTY_CYCLE_MASK) >> 6;
            halt_ = (data & PULSE_HALT_MASK) == PULSE_HALT_MASK;
            constantVolume_ = (data & PULSE_CONSTANT_VOLUME_MASK) == PULSE_CONSTANT_VOLUME_MASK;
            volume_ = data & PULSE_VOLUME_MASK;
            break;
        case 1:
            sweep_ = (data & PULSE_SWEEP_ENABLED_MASK) == PULSE_SWEEP_ENABLED_MASK;
            sweepDividerPeriod_ = (data & PULSE_SWEEP_DIVIDER_PERIOD_MASK) >> 4;
            negate_ = (data & PULSE_SWEEP_NEGATE_FLAG_MASK) == PULSE_SWEEP_NEGATE_FLAG_MASK;
            sweepShiftCount_ = data & PULSE_SWEEP_SHIFT_COUNT_MASK;
            break;
        case 2:
            timerReloadLow_ = data;
            break;
        case 3:
            lengthCounter_ = (data & PULSE_LENGTH_COUNTER_MASK) >> 3;
            timerReloadHigh_ = data & PULSE_TIMER_HIGH_MASK;
            sequencerIndex_ = 0;
            silence_ = false;
            break;
    }
}

void APU::PulseChannel::Silence()
{
    lengthCounter_ = 0;
    silence_ = true;
}
