#include "../include/APU.hpp"
#include "../include/RegisterAddresses.hpp"
#include <cstdint>

// APU

int LENGTH_COUNTER_LOOKUP_TABLE[32] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14, 12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

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

    pulseChannel1_.SetNegateBehavior(NegateBehavior::OnesComplement);
    pulseChannel2_.SetNegateBehavior(NegateBehavior::TwosComplement);
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

    pulseChannel1_.Clock();
    pulseChannel2_.Clock();

    ++frameCounterTimer_;

    switch (frameCounterTimer_)
    {
        case 3728:
            pulseChannel1_.QuarterFrameClock();
            pulseChannel2_.QuarterFrameClock();
            break;
        case 7456:
            pulseChannel1_.HalfFrameClock();
            pulseChannel2_.HalfFrameClock();
            break;
        case 11185:
            pulseChannel1_.QuarterFrameClock();
            pulseChannel2_.QuarterFrameClock();
            break;
        case 14915:
            if (!frameCounterMode_)
            {
                if (!irqInhibit_)
                {
                    irq_ = true;
                }

                pulseChannel1_.HalfFrameClock();
                pulseChannel2_.HalfFrameClock();
                frameCounterResetCountdown_ = 1;
            }
            break;
        case 18641:
            pulseChannel1_.HalfFrameClock();
            pulseChannel2_.HalfFrameClock();
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
    return pulseTable_[pulseChannel1_.Output() + pulseChannel2_.Output()];
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
            pulseChannel1_.RegisterUpdate(addr, data);
            break;
        case SQ2_VOL_ADDR:
        case SQ2_SWEEP_ADDR:
        case SQ2_LO_ADDR:
        case SQ2_HI_ADDR:
            pulseChannel2_.RegisterUpdate(addr, data);
            break;
        case SND_CHN_ADDR:
            pulseChannel1_.Toggle((data & 0x01) == 0x01);
            pulseChannel2_.Toggle((data & 0x02) == 0x02);
            break;
        case FRAME_COUNTER_ADDR:
            frameCounterMode_ = (data & 0x80) == 0x80;
            irqInhibit_ = (data & 0x40) == 0x40;

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
                // TODO: Clock units at start of five-step sequence?
            }
            break;
    }
}
