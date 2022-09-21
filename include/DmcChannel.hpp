#ifndef DMCCHANNEL_HPP
#define DMCCHANNEL_HPP

#include <cstddef>
#include <cstdint>
#include <optional>

class DmcChannel
{
public:
    DmcChannel();
    void Reset();

    uint8_t GetOutput();
    int GetBytesRemaining() { return bytesRemaining_; }
    void SetEnabled(bool enabled);

    void Clock();

    void RegisterUpdate(uint16_t addr, uint8_t data);

    bool IRQ() { return irq_; }
    std::optional<uint16_t> RequestSample();
    void SetSample(uint8_t sample);

private:
    static constexpr int RATE_LOOKUP_TABLE[16] = {428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54};

// IRQ
private:
    bool irqEnabled_;
    bool irq_;

// Timer
private:
    int timerReload_;
    int timer_;

// Output unit
private:
    uint8_t shiftReg_;
    int bitsRemaining_;
    uint8_t outputLevel_;
    bool silence_;

// Memory reader
private:
    uint16_t sampleAddress_;
    uint16_t currentAddress_;
    uint16_t sampleLength_;
    int bytesRemaining_;
    uint8_t sampleBuffer_;
    bool sampleBufferLoaded_;
    bool loop_;

};

#endif
