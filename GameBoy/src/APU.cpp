#include <APU.hpp>

void APU::Clock()
{
    if (!apuEnabled_)
    {
        return;
    }

    channel2_.Clock();
}

void APU::GetAudioSample(float* left, float* right)
{
    bool allDACsDisabled = !channel2_.DACEnabled();

    if (!apuEnabled_ || allDACsDisabled)
    {
        *left = 0.0;
        *right = 0.0;
        return;
    }

    float channel2Sample = channel2_.GetSample();

    uint_fast8_t leftCount = 0;
    uint_fast8_t rightCount = 0;

    *left = 0.0;
    *right = 0.0;

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
    ++divCounter_;

    if (divCounter_ == 64)
    {
        bool wasApuDivBitSet = DIV_ & (doubleSpeed ? 0x20 : 0x10);

        ++DIV_;
        divCounter_ = 0;
        bool isApuDivBitSet = DIV_ & (doubleSpeed ? 0x20 : 0x10);

        if (wasApuDivBitSet && !isApuDivBitSet)
        {
            channel2_.ApuDiv();
        }
    }
}

void APU::ResetDIV(bool const doubleSpeed)
{
    if (DIV_ & (doubleSpeed ? 0x20 : 0x10))
    {
        channel2_.ApuDiv();
    }

    DIV_ = 0;
    divCounter_ = 0;
}

uint8_t APU::Read(uint8_t ioAddr)
{
    switch (ioAddr)
    {
        case 0x16 ... 0x19:  // NR21 - NR24
            return channel2_.Read(ioAddr);
        case 0x24:  // NR50
            return NR50_;
        case 0x25:  // NR51
            return NR51_;
        case 0x26:  // NR52
            return (apuEnabled_ ? 0x80 : 0x00) | 0x70 | 0x08 | 0x04 | (channel2_.Enabled() ? 0x02 : 0x00) | 0x01;
        default:
            return 0xFF;
    }
}

void APU::Write(uint8_t ioAddr, uint8_t data)
{
    switch (ioAddr)
    {
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
    divCounter_ = 0;
    capacitor_ = 0.0;

    Write(0x24, 0x77);  // Initialize NR50
    Write(0x25, 0xF3);  // Initialize NR51
    Write(0x26, 0xF1);  // Initialize NR52

    channel2_.Reset();
}

float APU::HPF(float input)
{
    float output = input - capacitor_;
    capacitor_ = input - (output * 0.996);
    return output;
}
