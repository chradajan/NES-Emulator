#ifndef TRIANGLECHANNEL_HPP
#define TRIANGLECHANNEL_HPP

#include "AudioChannel.hpp"
#include <cstddef>
#include <cstdint>

class TriangleChannel : public virtual AudioChannel
{
public:
    TriangleChannel();
    void Reset() override;

    uint16_t GetOutput() override;
    void SetEnabled(bool enabled) override;

    void Clock() override;
    void HalfFrameClock() override;
    void QuarterFrameClock() override;

    void RegisterUpdate(uint16_t addr, uint8_t data) override;

private:
    static constexpr uint16_t TRIANGLE_WAVE_SEQUENCE[32] = {
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

// Sequencer
private:
    size_t sequencerIndex_;

// Length counter
private:
    int lengthCounter_;
    bool halt_;

// Linear counter
private:
    bool linearCounterControlFlag_;
    bool reloadLinearCounterFlag_;
    uint8_t linearCounterReload_;
    int linearCounter_;

// Timer
private:
    uint8_t timerReloadLow_;
    uint8_t timerReloadHigh_;
    uint16_t timerReload_;
    int timer_;

    void SetPeriod();
};

#endif
