#ifndef APU_HPP
#define APU_HPP

#include <array>
#include <cstdint>

// Sampling
constexpr size_t AUDIO_SAMPLE_BUFFER_COUNT = 735 * 2;
constexpr size_t AUDIO_SAMPLE_BUFFER_SIZE = AUDIO_SAMPLE_BUFFER_COUNT * sizeof(float);

// Length counters
extern int LENGTH_COUNTER_LOOKUP_TABLE[32];

// Pulse Channels
extern bool DUTY_CYCLE_SEQUENCE[4][8];
enum class NegateBehavior { OnesComplement, TwosComplement };

class APU
{
public:
    APU();
    ~APU() = default;

    void Clock();
    void Reset();

    float Output();

    uint8_t ReadReg(uint16_t addr);
    void WriteReg(uint16_t addr, uint8_t data);

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

// Pulse channels
private:
    class PulseChannel
    {
    public:
        PulseChannel();
        void SetNegateBehavior(NegateBehavior negateBehavior);
        void Reset();

        uint8_t Output();
        void Toggle(bool enabled);

        void Clock();
        void HalfFrameClock();
        void QuarterFrameClock();
        void RegisterUpdate(uint16_t addr, uint8_t data);

    private:
        bool channelEnabled_;

    // Duty cycle
    private:
        size_t dutyCycleIndex_;
        size_t sequencerIndex_;

    // Length counter
    private:
        int lengthCounter_;
        bool halt_;

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
        NegateBehavior negateBehavior_;
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

    PulseChannel pulseChannel1_;
    PulseChannel pulseChannel2_;
    std::array<float, 31> pulseTable_;

private:
    std::array<float, 203> tndTable_;
};

#endif
