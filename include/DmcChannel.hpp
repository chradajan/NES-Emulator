#ifndef DMCCHANNEL_HPP
#define DMCCHANNEL_HPP

#include <cstddef>
#include <cstdint>

class DmcChannel
{
public:
    DmcChannel();
    void Reset();

    uint16_t GetOutput();
    int GetBytesRemaining() { return bytesRemaining_; }
    void SetEnabled(bool enabled);

    void Clock();

    void RegisterUpdate(uint16_t addr, uint8_t data);

    bool IRQ() { return irq_; }

// Control
private:
    bool channelEnabled_;
    bool irqEnabled_;
    bool loop_;
    bool irq_;

// Samples
private:
    int bytesRemaining_;
};

#endif
