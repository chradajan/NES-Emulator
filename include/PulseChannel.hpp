#ifndef PULSECHANNEL_HPP
#define PULSECHANNEL_HPP

#include "AudioChannel.hpp"
#include <cstddef>
#include <cstdint>

class PulseChannel : public virtual AudioChannel
{
public:
    PulseChannel(bool onesComplement);
    void Reset() override;

    uint8_t GetOutput() override;
    void SetEnabled(bool enabled) override;

    void Clock() override;
    void HalfFrameClock() override;
    void QuarterFrameClock() override;

    void RegisterUpdate(uint16_t addr, uint8_t data) override;

private:
    static constexpr bool DUTY_CYCLE_SEQUENCE[4][8] = {
        {0, 1, 0, 0 ,0, 0, 0, 0},
        {0, 1, 1, 0, 0, 0, 0, 0},
        {0, 1, 1, 1, 1, 0, 0, 0},
        {1, 0, 0, 1, 1, 1, 1, 1}
    };

// Sequencer
private:
    size_t dutyCycleIndex_;
    size_t sequencerIndex_;

// Timer
private:
    uint8_t timerReloadLow_;
    uint8_t timerReloadHigh_;
    uint16_t timerReload_;
    int timer_;

    void SetPeriod();

// Envelope
private:
    bool silenced_;

    // Constant volume
    bool useConstantVolume_;
    uint8_t constantVolume_;

    // Envelope
    bool envelopeLooped_;
    bool envelopeStart_;
    uint8_t envelopeTimerReload_;
    int envelopeTimer_;
    uint8_t decayLevel_;

// Sweep
private:
    bool onesComplement_;
    bool sweepEnabled_;
    bool negate_;

    uint8_t sweepTimerReload_;
    int sweepTimer_;
    bool reloadSweepTimer_;

    int sweepShiftAmount_;
    uint16_t sweepTargetPeriod_;

    void SweepUpdate();
    void SetTargetPeriod();
};

#endif
