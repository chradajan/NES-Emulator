#ifndef APU_HPP
#define APU_HPP

#include <array>
#include <cstdint>

// Sampling
constexpr size_t AUDIO_SAMPLE_BUFFER_COUNT = 735 * 2;
constexpr size_t AUDIO_SAMPLE_BUFFER_SIZE = AUDIO_SAMPLE_BUFFER_COUNT * sizeof(float);

// Pulse channels
constexpr uint8_t PULSE_DUTY_CYCLE_MASK = 0xC0;
constexpr uint8_t PULSE_HALT_MASK = 0x20;
constexpr uint8_t PULSE_CONSTANT_VOLUME_MASK = 0x10;
constexpr uint8_t PULSE_VOLUME_MASK = 0x0F;

constexpr uint8_t PULSE_SWEEP_ENABLED_MASK = 0x80;
constexpr uint8_t PULSE_SWEEP_DIVIDER_PERIOD_MASK = 0x70;
constexpr uint8_t PULSE_SWEEP_NEGATE_FLAG_MASK = 0x08;
constexpr uint8_t PULSE_SWEEP_SHIFT_COUNT_MASK = 0x07;

constexpr uint8_t PULSE_LENGTH_COUNTER_MASK = 0xF8;
constexpr uint8_t PULSE_TIMER_HIGH_MASK = 0x07;

// Frame counter
constexpr uint8_t FRAME_COUNTER_SEQUENCER_MODE_MASK = 0x80;
constexpr uint8_t FRAME_COUNTER_INTERRUPT_MASK = 0x40;

// Status register
constexpr uint8_t ENABLE_DMC = 0x10;
constexpr uint8_t ENABLE_NOISE = 0x08;
constexpr uint8_t ENABLE_TRIANGLE = 0x04;
constexpr uint8_t ENABLE_PULSE1 = 0x02;
constexpr uint8_t ENABLE_PULSE0 = 0x01;

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
        void Reset();

        void Clock();
        void ClockLengthCounter();
        uint8_t Output();

        void RegUpdate(uint16_t addr, uint8_t data);
        void Silence();

    private:
        static constexpr bool dutyCycleSequence_[4][8] = {
            {0, 1, 0, 0 ,0, 0, 0, 0},
            {0, 1, 1, 0, 0, 0, 0, 0},
            {0, 1, 1, 1, 1, 0, 0, 0},
            {1, 0, 0, 1, 1, 1, 1, 1}
        };

        size_t dutyCycleIndex_;
        size_t sequencerIndex_;

        bool halt_;
        bool constantVolume_;
        uint8_t volume_;

        bool sweep_;
        uint8_t sweepDividerPeriod_;
        bool negate_;
        uint8_t sweepShiftCount_;

        uint8_t timerReloadLow_;
        uint8_t timerReloadHigh_;
        int timer_;

        int lengthCounter_;
        bool silence_;
    };

    PulseChannel pulseChannel_[2];
    std::array<float, 31> pulseTable_;

private:
    std::array<float, 203> tndTable_;
};

#endif
