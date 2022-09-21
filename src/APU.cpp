#include "../include/APU.hpp"
#include "../include/DmcChannel.hpp"
#include "../include/NoiseChannel.hpp"
#include "../include/PulseChannel.hpp"
#include "../include/RegisterAddresses.hpp"
#include "../include/TriangleChannel.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

APU::APU()
{
    pulseTable_[0] = 0.0;

    for (size_t n = 1; n < 31; ++n)
    {
        pulseTable_[n] = 95.52 / ((8128.0 / n) + 100);
    }

    tndTable_[0] = 0.0;

    for (size_t n = 1; n < 203; ++n)
    {
        tndTable_[n] = 163.67 / ((24329.0 / n) + 100);
    }

    irq_ = false;
    clockAPU_ = false;
    frameCounterMode_ = false;
    irqInhibit_ = true;
    frameCounterTimer_ = 0;
    frameCounterResetCountdown_ = 0;

    CHANNELS_[0] = std::make_unique<PulseChannel>(true);
    CHANNELS_[1] = std::make_unique<PulseChannel>(false);
    CHANNELS_[2] = std::make_unique<TriangleChannel>();
    CHANNELS_[3] = std::make_unique<NoiseChannel>();
    dmcChannel_ = std::make_unique<DmcChannel>();

    pulseChannel1_ = dynamic_cast<PulseChannel*>(CHANNELS_[0].get());
    pulseChannel2_ = dynamic_cast<PulseChannel*>(CHANNELS_[1].get());
    triangleChannel_ = dynamic_cast<TriangleChannel*>(CHANNELS_[2].get());
    noiseChannel_ = dynamic_cast<NoiseChannel*>(CHANNELS_[3].get());
}

void APU::Clock()
{
    clockAPU_ = !clockAPU_;

    if (frameCounterResetCountdown_ > 0)
    {
        --frameCounterResetCountdown_;

        if (frameCounterResetCountdown_ == 0)
        {
            frameCounterTimer_ = 0;
        }
    }

    triangleChannel_->Clock();

    if (!clockAPU_)
    {
        return;
    }

    pulseChannel1_->Clock();
    pulseChannel2_->Clock();
    noiseChannel_->Clock();
    dmcChannel_->Clock();
    ClockFrameCounter();
}

void APU::Reset()
{
    for (std::unique_ptr<AudioChannel>& channel : CHANNELS_)
    {
        channel->Reset();
    }

    dmcChannel_->Reset();
}

int16_t APU::GetSample()
{
    float pulseOut = pulseTable_[pulseChannel1_->GetOutput() + pulseChannel2_->GetOutput()];
    float tndOut = tndTable_[(3 * triangleChannel_->GetOutput()) + (2 * noiseChannel_->GetOutput()) + dmcChannel_->GetOutput()];
    int16_t signedOut = ((pulseOut + tndOut) * 0xFFFF) - 0x8000;
    return signedOut;
}

uint8_t APU::ReadReg(uint16_t addr)
{
    uint8_t returnData = 0x00;

    if (addr == SND_CHN_ADDR)
    {
        returnData |= (pulseChannel1_->GetLengthCounter() > 0) ? 0x01 : 0x00;
        returnData |= (pulseChannel2_->GetLengthCounter() > 0) ? 0x02 : 0x00;
        returnData |= (triangleChannel_->GetLengthCounter() > 0) ? 0x04 : 0x00;
        returnData |= (noiseChannel_->GetLengthCounter() > 0) ? 0x08 : 0x00;
        returnData |= (dmcChannel_->GetBytesRemaining() > 0) ? 0x10 : 0x00;
        returnData |= irq_ ? 0x40 : 0x00;
        returnData |= dmcChannel_->IRQ() ? 0x80 : 0x00;
        irq_ = false;
    }

    return returnData;
}

void APU::WriteReg(uint16_t addr, uint8_t data)
{
    switch (addr)
    {
        case SQ1_VOL_ADDR:
        case SQ1_SWEEP_ADDR:
        case SQ1_LO_ADDR:
        case SQ1_HI_ADDR:
            pulseChannel1_->RegisterUpdate(addr, data);
            break;
        case SQ2_VOL_ADDR:
        case SQ2_SWEEP_ADDR:
        case SQ2_LO_ADDR:
        case SQ2_HI_ADDR:
            pulseChannel2_->RegisterUpdate(addr, data);
            break;
        case TRI_LINEAR_ADDR:
        case TRI_LO_ADDR:
        case TRI_HI_ADDR:
            triangleChannel_->RegisterUpdate(addr, data);
            break;
        case NOISE_VOL_ADDR:
        case NOISE_LO_ADDR:
        case NOISE_HI_ADDR:
            noiseChannel_->RegisterUpdate(addr, data);
            break;
        case DMC_FREQ_ADDR:
        case DMC_RAW_ADDR:
        case DMC_START_ADDR:
        case DMC_LEN_ADDR:
            dmcChannel_->RegisterUpdate(addr, data);
            break;
        case SND_CHN_ADDR:
            pulseChannel1_->SetEnabled((data & 0x01) == 0x01);
            pulseChannel2_->SetEnabled((data & 0x02) == 0x02);
            triangleChannel_->SetEnabled((data & 0x04) == 0x04);
            noiseChannel_->SetEnabled((data & 0x08) == 0x08);
            dmcChannel_->SetEnabled((data & 0x10) == 0x10);
            break;
        case FRAME_COUNTER_ADDR:
            frameCounterMode_ = (data & 0x80) == 0x80;
            irqInhibit_ = (data & 0x40) == 0x40;

            if (clockAPU_)
            {
                // Write occurred between APU cycles.
                frameCounterResetCountdown_ = 5;
            }
            else
            {
                // Write occurred during APU cycle.
                frameCounterResetCountdown_ = 4;
            }

            if (frameCounterMode_)
            {
                HalfFrameClock();
            }
            break;
    }
}

bool APU::IRQ()
{
    return irq_;
}

std::optional<uint16_t> APU::DmcRequestSample()
{
    return dmcChannel_->RequestSample();
}

void APU::SetDmcSample(uint8_t sample)
{
    dmcChannel_->SetSample(sample);
}

void APU::Serialize(std::ofstream& saveState)
{
    saveState.write((char*)&irq_, sizeof(irq_));
    saveState.write((char*)&clockAPU_, sizeof(clockAPU_));
    saveState.write((char*)&frameCounterMode_, sizeof(frameCounterMode_));
    saveState.write((char*)&irqInhibit_, sizeof(irqInhibit_));
    saveState.write((char*)&frameCounterTimer_, sizeof(frameCounterTimer_));
    saveState.write((char*)&frameCounterResetCountdown_, sizeof(frameCounterResetCountdown_));

    for (std::unique_ptr<AudioChannel>& channel : CHANNELS_)
    {
        channel->Serialize(saveState);
    }

    dmcChannel_->Serialize(saveState);
}

void APU::Deserialize(std::ifstream& saveState)
{
    saveState.read((char*)&irq_, sizeof(irq_));
    saveState.read((char*)&clockAPU_, sizeof(clockAPU_));
    saveState.read((char*)&frameCounterMode_, sizeof(frameCounterMode_));
    saveState.read((char*)&irqInhibit_, sizeof(irqInhibit_));
    saveState.read((char*)&frameCounterTimer_, sizeof(frameCounterTimer_));
    saveState.read((char*)&frameCounterResetCountdown_, sizeof(frameCounterResetCountdown_));

    for (std::unique_ptr<AudioChannel>& channel : CHANNELS_)
    {
        channel->Deserialize(saveState);
    }

    dmcChannel_->Deserialize(saveState);
}

void APU::ClockFrameCounter()
{
    ++frameCounterTimer_;

    switch (frameCounterTimer_)
    {
        case 3728:
            QuarterFrameClock();
            break;
        case 7456:
            HalfFrameClock();
            break;
        case 11185:
            QuarterFrameClock();
            break;
        case 14915:
            if (!frameCounterMode_)
            {
                if (!irqInhibit_)
                {
                    irq_ = true;
                }

                HalfFrameClock();
                frameCounterResetCountdown_ = 1;
            }
            break;
        case 18641:
            HalfFrameClock();
            frameCounterResetCountdown_ = 1;
            break;
        default:
            break;
    }
}

void APU::HalfFrameClock()
{
    for (std::unique_ptr<AudioChannel>& channel : CHANNELS_)
    {
        channel->HalfFrameClock();
    }
}

void APU::QuarterFrameClock()
{
    for (std::unique_ptr<AudioChannel>& channel : CHANNELS_)
    {
        channel->QuarterFrameClock();
    }
}
