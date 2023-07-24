#include <APU.hpp>

void APU::Clock()
{
    if (!apuEnabled_)
    {
        return;
    }

    channel1_.Clock();
    channel2_.Clock();
    channel3_.Clock();
    channel4_.Clock();
}

void APU::PowerOn(bool const skipBootRom)
{
    divDivider_ = 0;
    envelopeDivider_ = 0;
    soundLengthDivider_ = 0;
    ch1FreqDivider_ = 0;
    capacitor_ = 0.0;

    if (skipBootRom)
    {
        Write(0x24, 0x77);  // Initialize NR50
        Write(0x25, 0xF3);  // Initialize NR51
        Write(0x26, 0xF1);  // Initialize NR52
    }
    else
    {
        NR50_ = 0x00;
        NR51_ = 0x00;
        leftVolume_ = 0;
        rightVolume_ = 0;
        mix1Right_ = false;
        mix2Right_ = false;
        mix3Right_ = false;
        mix4Right_ = false;
        mix1Left_ = false;
        mix2Left_ = false;
        mix3Left_ = false;
        mix4Left_ = false;
        apuEnabled_ = false;
    }

    channel1_.PowerOn(skipBootRom);
    channel2_.PowerOn(skipBootRom);
    channel3_.PowerOn(skipBootRom);
    channel4_.PowerOn(skipBootRom);
}

void APU::GetAudioSample(float* left, float* right)
{
    bool allDACsDisabled = !channel1_.DACEnabled() && !channel2_.DACEnabled() && !channel4_.DACEnabled();

    if (!apuEnabled_ || allDACsDisabled)
    {
        *left = 0.0;
        *right = 0.0;
        return;
    }

    float channel1Sample = channel1_.GetSample();
    float channel2Sample = channel2_.GetSample();
    float channel3Sample = channel3_.GetSample();
    float channel4Sample = channel4_.GetSample();

    uint_fast8_t leftCount = 0;
    uint_fast8_t rightCount = 0;

    *left = 0.0;
    *right = 0.0;

    if (mix1Left_) { *left += channel1Sample; ++leftCount; }
    if (mix1Right_) { *right += channel1Sample; ++rightCount; }

    if (mix2Left_) { *left += channel2Sample; ++leftCount; }
    if (mix2Right_) { *right += channel2Sample; ++rightCount; }

    if (mix3Left_) { *left += channel3Sample; ++leftCount; }
    if (mix3Right_) { *right += channel3Sample; ++rightCount; }

    if (mix4Left_) { *left += channel4Sample; ++leftCount; }
    if (mix4Right_) { *right += channel4Sample; ++rightCount; }

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
        case 0x1A ... 0x1E:  // NR30 - NR34
            return channel3_.Read(ioAddr);
        case 0x20 ... 0x23:  // NR41 - NR44
            return channel4_.Read(ioAddr);
        case 0x24:  // NR50
            return NR50_;
        case 0x25:  // NR51
            return NR51_;
        case 0x26:  // NR52
            return 0x70 |
                   (apuEnabled_ ? 0x80 : 0x00) |
                   (channel4_.Enabled() ? 0x08 : 0x00) |
                   (channel3_.Enabled() ? 0x04 : 0x00) |
                   (channel2_.Enabled() ? 0x02 : 0x00) |
                   (channel1_.Enabled() ? 0x01 : 0x00);
        case 0x30 ... 0x3F:  // Wave RAM
            return channel3_.Read(ioAddr);
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
        case 0x1A ... 0x1E:  // NR30 - NR34
            channel3_.Write(ioAddr, data);
            break;
        case 0x20 ... 0x23:  // NR41 - NR44
            channel4_.Write(ioAddr, data);
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
        case 0x30 ... 0x3F:  // Wave RAM
            channel3_.Write(ioAddr, data);
            break;
        default:
            break;
        }
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
        channel4_.ClockEnvelope();
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
        channel3_.ClockLengthTimer();
        channel4_.ClockLengthTimer();
    }
}
