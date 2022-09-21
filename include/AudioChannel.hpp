#ifndef AUDIOCHANNEL_HPP
#define AUDIOCHANNEL_HPP

#include <cstdint>

static const int LENGTH_COUNTER_LOOKUP_TABLE[32] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14, 12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

class AudioChannel
{
public:
    virtual ~AudioChannel() {}
    virtual void Reset() = 0;

    virtual uint16_t GetOutput() = 0;
    virtual void SetEnabled(bool enabled) = 0;

    virtual void Clock() = 0;
    virtual void HalfFrameClock() = 0;
    virtual void QuarterFrameClock() = 0;

    virtual void RegisterUpdate(uint16_t addr, uint8_t data);

protected:
    bool channelEnabled_;
};

#endif
