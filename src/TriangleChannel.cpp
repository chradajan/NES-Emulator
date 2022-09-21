#include "../include/TriangleChannel.hpp"
#include <cstddef>
#include <cstdint>
#include <fstream>

TriangleChannel::TriangleChannel()
{
    Reset();
}

void TriangleChannel::Reset()
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

uint8_t TriangleChannel::GetOutput()
{
    if (timerReload_ < 2)
    {
        return 0x00;
    }

    return TRIANGLE_WAVE_SEQUENCE[sequencerIndex_];
}

void TriangleChannel::SetEnabled(bool enabled)
{
    channelEnabled_ = enabled;

    if (!enabled)
    {
        lengthCounter_ = 0;
    }
}

void TriangleChannel::Clock()
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

void TriangleChannel::HalfFrameClock()
{
    QuarterFrameClock();

    if (!halt_ && (lengthCounter_ > 0))
    {
        --lengthCounter_;
    }
}

void TriangleChannel::QuarterFrameClock()
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

void TriangleChannel::RegisterUpdate(uint16_t addr, uint8_t data)
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
        case 3:  // $400B
            lengthCounter_ = channelEnabled_ ? LENGTH_COUNTER_LOOKUP_TABLE[(data & 0xF8) >> 3] : 0;
            timerReloadHigh_ = data & 0x07;
            reloadLinearCounterFlag_ = true;
            SetPeriod();
            break;
    }
}

void TriangleChannel::Serialize(std::ofstream& saveState)
{
    saveState.write((char*)this, sizeof(TriangleChannel));
}

void TriangleChannel::Deserialize(std::ifstream& saveState)
{
    saveState.read((char*)this, sizeof(TriangleChannel));
}

void TriangleChannel::SetPeriod()
{
    timerReload_ = ((timerReloadHigh_ << 8) | timerReloadLow_) + 1;
}
