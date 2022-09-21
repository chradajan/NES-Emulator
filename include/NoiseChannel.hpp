#ifndef NOISECHANNEL_HPP
#define NOISECHANNEL_HPP

#include "AudioChannel.hpp"
#include <cstddef>
#include <cstdint>
#include <fstream>

class NoiseChannel : public virtual AudioChannel
{
public:
    NoiseChannel();
    void Reset() override;

    uint8_t GetOutput() override;
    void SetEnabled(bool enabled) override;

    void Clock() override;
    void HalfFrameClock() override;
    void QuarterFrameClock() override;

    void RegisterUpdate(uint16_t addr, uint8_t data) override;

    void Serialize(std::ofstream& saveState) override;
    void Deserialize(std::ifstream& saveState) override;

private:
    static constexpr int NOISE_TIMER_LOOKUP_TABLE[16] = {4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068};

// Control
private:
    bool mode_;
    uint16_t shiftRegister_;

// Envelope
private:
    // Constant volume
    bool useConstantVolume_;
    uint8_t constantVolume_;

    // Envelope
    bool envelopeLooped_;
    bool envelopeStart_;
    uint8_t envelopeTimerReload_;
    int envelopeTimer_;
    uint8_t decayLevel_;

// Timer
private:
    int timerReload_;
    int timer_;
};

#endif
