#include <APU.hpp>

void APU::Clock()
{
    if (!apuEnabled_)
    {
        return;
    }

    channel1_.Clock();
    channel2_.Clock();
}

void APU::GetAudioSample(float* left, float* right)
{
    bool allDACsDisabled = !channel1_.DACEnabled() && !channel2_.DACEnabled();

    if (!apuEnabled_ || allDACsDisabled)
    {
        *left = 0.0;
        *right = 0.0;
        return;
    }

    float channel1Sample = channel1_.GetSample();
    float channel2Sample = channel2_.GetSample();

    uint_fast8_t leftCount = 0;
    uint_fast8_t rightCount = 0;

    *left = 0.0;
    *right = 0.0;

    if (mix1Left_) { *left += channel1Sample; ++leftCount; }
    if (mix1Right_) { *right += channel1Sample; ++rightCount; }

    if (mix2Left_) { *left += channel2Sample; ++leftCount; }
    if (mix2Right_) { *right += channel2Sample; ++rightCount; }

    if (leftCount > 0)
    {
        *left /= leftCount;
    }

    if (rightCount > 0)
    {
        *right /= rightCount;
    }

    *left *= leftVolume_;
    *right *= rightVolume_;

    *left = HPF(*left);
    *right = HPF(*right);
}

void APU::ClockDIV(bool const doubleSpeed)
{
    ++divDivider_;

    if (divDivider_ == 64)
    {
        divDivider_ = 0;
        bool wasApuDivBitSet = DIV_++ & (doubleSpeed ? 0x20 : 0x10);
        bool isApuDivBitSet = DIV_ & (doubleSpeed ? 0x20 : 0x10);

        if (wasApuDivBitSet && !isApuDivBitSet)
        {
            AdvanceFrameSequencer();
        }
    }
}

void APU::ResetDIV(bool const doubleSpeed)
{
    if (DIV_ & (doubleSpeed ? 0x20 : 0x10))
    {
        AdvanceFrameSequencer();
    }

    DIV_ = 0;
}

uint8_t APU::Read(uint8_t ioAddr)
{
    switch (ioAddr)
    {
        case 0x10 ... 0x14:  // NR10 - NR14
            return channel1_.Read(ioAddr);
        case 0x16 ... 0x19:  // NR21 - NR24
            return channel2_.Read(ioAddr);
        case 0x24:  // NR50
            return NR50_;
        case 0x25:  // NR51
            return NR51_;
        case 0x26:  // NR52
            return 0x70 |
                   (apuEnabled_ ? 0x80 : 0x00) |
                   0x08 |
                   0x04 |
                   (channel2_.Enabled() ? 0x02 : 0x00) |
                   (channel1_.Enabled() ? 0x01 : 0x00);
        default:
            return 0xFF;
    }
}

void APU::Write(uint8_t ioAddr, uint8_t data)
{
    switch (ioAddr)
    {
        case 0x10 ... 0x14:  // NR10 - NR14
            channel1_.Write(ioAddr, data);
            break;
        case 0x16 ... 0x19:  // NR21 - NR24
            channel2_.Write(ioAddr, data);
            break;
        case 0x24:  // NR50
        {
            uint_fast8_t left = (data & 0x70) >> 4;
            uint_fast8_t right = data & 0x07;
            leftVolume_ = (0.1286 * left) + 0.1;
            rightVolume_ = (0.1286 * right) + 0.1;
            break;
        }
        case 0x25:  // NR51
        {
            mix1Right_ = data & 0x01;
            mix2Right_ = data & 0x02;
            mix3Right_ = data & 0x04;
            mix4Right_ = data & 0x08;
            mix1Left_ = data & 0x10;
            mix2Left_ = data & 0x20;
            mix3Left_ = data & 0x40;
            mix4Left_ = data & 0x80;
            NR51_ = data;
            break;
        }
        case 0x26:  // NR52
            apuEnabled_ = data & 0x80;
            break;
        default:
            break;
        }
}

void APU::Reset()
{
    divDivider_ = 0;
    envelopeDivider_ = 0;
    soundLengthDivider_ = 0;
    ch1FreqDivider_ = 0;
    capacitor_ = 0.0;

    Write(0x24, 0x77);  // Initialize NR50
    Write(0x25, 0xF3);  // Initialize NR51
    Write(0x26, 0xF1);  // Initialize NR52

    channel1_.Reset();
    channel2_.Reset();
}

float APU::HPF(float input)
{
    float output = input - capacitor_;
    capacitor_ = input - (output * 0.996);
    return output;
}

void APU::AdvanceFrameSequencer()
{
    ++envelopeDivider_;
    ++soundLengthDivider_;
    ++ch1FreqDivider_;

    if (envelopeDivider_ == 8)  // 64 Hz
    {
        envelopeDivider_ = 0;
        channel1_.ClockEnvelope();
        channel2_.ClockEnvelope();
    }

    if (ch1FreqDivider_ == 4)  // 128 Hz
    {
        ch1FreqDivider_ = 0;
        channel1_.ClockFrequencySweep();
    }

    if (soundLengthDivider_ == 2)  // 256 Hz
    {
        soundLengthDivider_ = 0;
        channel1_.ClockLengthTimer();
        channel2_.ClockLengthTimer();
    }
}