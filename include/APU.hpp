#ifndef APU_HPP
#define APU_HPP

#include <array>
#include <cstdint>
#include <memory>

class AudioChannel;
class DmcChannel;
class NoiseChannel;
class PulseChannel;
class TriangleChannel;

class APU
{
public:
    APU();
    ~APU() = default;

    void Clock();
    void Reset();

    int16_t GetSample();

    uint8_t ReadReg(uint16_t addr);
    void WriteReg(uint16_t addr, uint8_t data);

    bool IRQ();

// State
private:
    bool irq_;
    bool clockAPU_;

// Frame Counter
private:
    // True:  5-step sequence
    // False: 4-step sequence
    bool frameCounterMode_;
    bool irqInhibit_;

    int frameCounterTimer_;
    int frameCounterResetCountdown_;

    void ClockFrameCounter();

// Channels
private:
    std::array<std::unique_ptr<AudioChannel>, 4> CHANNELS_;
    PulseChannel* pulseChannel1_;
    PulseChannel* pulseChannel2_;
    TriangleChannel* triangleChannel_;
    NoiseChannel* noiseChannel_;

    std::unique_ptr<DmcChannel> dmcChannel_;

    void HalfFrameClock();
    void QuarterFrameClock();

// Lookup tables
private:
    std::array<float, 31> pulseTable_;
    std::array<float, 203> tndTable_;
};

#endif
