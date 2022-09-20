#include "../include/APU.hpp"
#include <cstdint>

uint8_t TRIANGLE_WAVE_SEQUENCE[32] = {
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

APU::TriangleChannel::TriangleChannel()
{
    Reset();
}

void APU::TriangleChannel::Reset()
{
    channelEnabled_ = false;

    sequencerIndex_ = 0;

    lengthCounter_ = 0;
    halt_ = false;

    linearCounterControlFlag_ = false;
    reloadLinearCounterFlag_ = false;
    linearCounterReload_ = 0;
    linearCounter_ = 0;

    timerReloadLow_ = 0;
    timerReloadHigh_ = 0;
    timerReload_ = 0;
    timer_ = 0;
}

uint8_t APU::TriangleChannel::Output()
{
    if (timerReload_ < 2)
    {
        return 0x00;
    }

    return TRIANGLE_WAVE_SEQUENCE[sequencerIndex_];
}

void APU::TriangleChannel::Toggle(bool enabled)
{
    channelEnabled_ = enabled;

    if (!enabled)
    {
        lengthCounter_ = 0;
    }
}

void APU::TriangleChannel::Clock()
{
    if (timer_ == 0)
    {
        timer_ = timerReload_;

        if ((lengthCounter_ > 0) && (linearCounter_ > 0))
        {
            sequencerIndex_ = (sequencerIndex_ + 1) % 32;
        }
    }
    else
    {
        --timer_;
    }
}

void APU::TriangleChannel::HalfFrameClock()
{
    QuarterFrameClock();

    if (!halt_ && (lengthCounter_ > 0))
    {
        --lengthCounter_;
    }
}

void APU::TriangleChannel::QuarterFrameClock()
{
    if (reloadLinearCounterFlag_)
    {
        linearCounter_ = linearCounterReload_;
    }
    else if (linearCounter_ > 0)
    {
        --linearCounter_;
    }

    if (!linearCounterControlFlag_)
    {
        reloadLinearCounterFlag_ = false;
    }
}

void APU::TriangleChannel::RegisterUpdate(uint16_t addr, uint8_t data)
{
    int reg = addr & 0x03;

    switch (reg)
    {
        case 0:  // $4008
            linearCounterControlFlag_ = (data & 0x80) == 0x80;
            halt_ = linearCounterControlFlag_;
            linearCounterReload_ = (data & 0x7F);
            break;
        case 2:  // $400A
            timerReloadLow_ = data;
            SetPeriod();
            break;
        case 3:    // $400B
            lengthCounter_ = channelEnabled_ ? LENGTH_COUNTER_LOOKUP_TABLE[(data & 0xF8) >> 3] : 0;
            timerReloadHigh_ = data & 0x07;
            reloadLinearCounterFlag_ = true;
            SetPeriod();
            break;
    }
}

void APU::TriangleChannel::SetPeriod()
{
    timerReload_ = ((timerReloadHigh_ << 8) | timerReloadLow_) + 1;
}
