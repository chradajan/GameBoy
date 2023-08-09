#include <APU.hpp>
#include <cmath>
#include <fstream>
#include <vector>

static std::vector<float> LEFT_SAMPLE_BUFFER;
static std::vector<float> RIGHT_SAMPLE_BUFFER;

static float LAST_LEFT_SAMPLE = 0.0;
static float LAST_RIGHT_SAMPLE = 0.0;

static int DOWNSAMPLED_RATE = 44100;
static constexpr float DELTA_T = 1.0 / 1048576;  // Time between samples
static float TAU = 1.0 / (DOWNSAMPLED_RATE / 2);  // Time constant
static float ALPHA = DELTA_T / TAU;

static std::vector<float> LPF(std::vector<float> const& input, float& lastSample)
{
    std::vector<float> output;
    output.reserve(input.size());

    float out = lastSample;

    for (size_t i = 0; i < input.size(); ++i)
    {
        out += ALPHA * (input[i] - out);
        output.push_back(out);
    }

    if (!output.empty())
    {
        lastSample = output[output.size() - 1];
    }

    return output;
}

APU::APU() :
    monoAudio_(false),
    volume_(1.0),
    channel1Disabled_(false),
    channel2Disabled_(false),
    channel3Disabled_(false),
    channel4Disabled_(false),
    apuEnabled_(false)
{
}

void APU::Clock()
{
    if (!apuEnabled_)
    {
        LEFT_SAMPLE_BUFFER.push_back(0.0);
        RIGHT_SAMPLE_BUFFER.push_back(0.0);
        return;
    }

    float channel1Sample = channel1_.Clock();
    float channel2Sample = channel2_.Clock();
    float channel3Sample = channel3_.Clock();
    float channel4Sample = channel4_.Clock();

    uint_fast8_t leftCount = 0;
    uint_fast8_t rightCount = 0;

    float left_sample = 0.0;
    float right_sample = 0.0;

    if (!channel1Disabled_)
    {
        if (monoAudio_ && (mix1Left_ || mix1Right_))
        {
            left_sample += channel1Sample;
            ++leftCount;

            right_sample += channel1Sample;
            ++rightCount;
        }
        else
        {
            if (mix1Left_) { left_sample += channel1Sample; ++leftCount; }
            if (mix1Right_) { right_sample += channel1Sample; ++rightCount; }
        }
    }

    if (!channel2Disabled_)
    {
        if (monoAudio_ && (mix2Left_ || mix2Right_))
        {
            left_sample += channel2Sample;
            ++leftCount;

            right_sample += channel2Sample;
            ++rightCount;
        }
        else
        {
            if (mix2Left_) { left_sample += channel2Sample; ++leftCount; }
            if (mix2Right_) { right_sample += channel2Sample; ++rightCount; }
        }
    }

    if (!channel3Disabled_)
    {
        if (monoAudio_ && (mix3Left_ || mix3Right_))
        {
            left_sample += channel3Sample;
            ++leftCount;

            right_sample += channel3Sample;
            ++rightCount;
        }
        else
        {
            if (mix3Left_) { left_sample += channel3Sample; ++leftCount; }
            if (mix3Right_) { right_sample += channel3Sample; ++rightCount; }
        }
    }

    if (!channel4Disabled_)
    {
        if (monoAudio_ && (mix4Left_ || mix4Right_))
        {
            left_sample += channel4Sample;
            ++leftCount;

            right_sample += channel4Sample;
            ++rightCount;
        }
        else
        {
            if (mix4Left_) { left_sample += channel4Sample; ++leftCount; }
            if (mix4Right_) { right_sample += channel4Sample; ++rightCount; }
        }
    }

    if (leftCount > 0)
    {
        left_sample /= leftCount;
    }

    if (rightCount > 0)
    {
        right_sample /= rightCount;
    }

    if (!monoAudio_)
    {
        left_sample *= leftVolume_;
        right_sample *= rightVolume_;
    }

    left_sample = HPF(left_sample) * volume_;
    right_sample = HPF(right_sample) * volume_;

    LEFT_SAMPLE_BUFFER.push_back(left_sample);
    RIGHT_SAMPLE_BUFFER.push_back(right_sample);
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

void APU::SetSampleRate(int const sampleRate)
{
    DOWNSAMPLED_RATE = sampleRate;
    TAU = 1.0 / (DOWNSAMPLED_RATE / 2);
    ALPHA = DELTA_T / TAU;
    LEFT_SAMPLE_BUFFER.clear();
    RIGHT_SAMPLE_BUFFER.clear();
}

void APU::DrainSampleBuffer(float* buffer, int count)
{
    auto filteredLeftBuffer = LPF(LEFT_SAMPLE_BUFFER, LAST_LEFT_SAMPLE);
    auto filteredRightBuffer = LPF(RIGHT_SAMPLE_BUFFER, LAST_RIGHT_SAMPLE);

    int numSamples = count / 2;
    float downsampleDivider = static_cast<float>(LEFT_SAMPLE_BUFFER.size()) / numSamples;

    for (int output_index = 0; output_index < count; output_index += 2)
    {
        size_t index = std::round((output_index / 2) * downsampleDivider);

        if (index >= filteredLeftBuffer.size())
        {
            buffer[output_index] = 0.0;
            buffer[output_index + 1] = 0.0;
        }
        else
        {
            buffer[output_index] = filteredLeftBuffer[index];
            buffer[output_index + 1] = filteredRightBuffer[index];
        }
    }

    LEFT_SAMPLE_BUFFER.clear();
    RIGHT_SAMPLE_BUFFER.clear();
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
            NR50_ = data;
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

void APU::Serialize(std::ofstream& out)
{
    out.write(reinterpret_cast<char*>(&apuEnabled_), sizeof(apuEnabled_));
    out.write(reinterpret_cast<char*>(&capacitor_), sizeof(capacitor_));

    out.write(reinterpret_cast<char*>(&divDivider_), sizeof(divDivider_));
    out.write(reinterpret_cast<char*>(&envelopeDivider_), sizeof(envelopeDivider_));
    out.write(reinterpret_cast<char*>(&soundLengthDivider_), sizeof(soundLengthDivider_));
    out.write(reinterpret_cast<char*>(&ch1FreqDivider_), sizeof(ch1FreqDivider_));

    out.write(reinterpret_cast<char*>(&mix1Left_), sizeof(mix1Left_));
    out.write(reinterpret_cast<char*>(&mix1Right_), sizeof(mix1Right_));
    out.write(reinterpret_cast<char*>(&mix2Left_), sizeof(mix2Left_));
    out.write(reinterpret_cast<char*>(&mix2Right_), sizeof(mix2Right_));
    out.write(reinterpret_cast<char*>(&mix3Left_), sizeof(mix3Left_));
    out.write(reinterpret_cast<char*>(&mix3Right_), sizeof(mix3Right_));
    out.write(reinterpret_cast<char*>(&mix4Left_), sizeof(mix4Left_));
    out.write(reinterpret_cast<char*>(&mix4Right_), sizeof(mix4Right_));

    out.write(reinterpret_cast<char*>(&leftVolume_), sizeof(leftVolume_));
    out.write(reinterpret_cast<char*>(&rightVolume_), sizeof(rightVolume_));

    out.write(reinterpret_cast<char*>(&DIV_), sizeof(DIV_));
    out.write(reinterpret_cast<char*>(&NR50_), sizeof(NR50_));
    out.write(reinterpret_cast<char*>(&NR51_), sizeof(NR51_));

    channel1_.Serialize(out);
    channel2_.Serialize(out);
    channel3_.Serialize(out);
    channel4_.Serialize(out);
}

void APU::Deserialize(std::ifstream& in)
{
    in.read(reinterpret_cast<char*>(&apuEnabled_), sizeof(apuEnabled_));
    in.read(reinterpret_cast<char*>(&capacitor_), sizeof(capacitor_));

    in.read(reinterpret_cast<char*>(&divDivider_), sizeof(divDivider_));
    in.read(reinterpret_cast<char*>(&envelopeDivider_), sizeof(envelopeDivider_));
    in.read(reinterpret_cast<char*>(&soundLengthDivider_), sizeof(soundLengthDivider_));
    in.read(reinterpret_cast<char*>(&ch1FreqDivider_), sizeof(ch1FreqDivider_));

    in.read(reinterpret_cast<char*>(&mix1Left_), sizeof(mix1Left_));
    in.read(reinterpret_cast<char*>(&mix1Right_), sizeof(mix1Right_));
    in.read(reinterpret_cast<char*>(&mix2Left_), sizeof(mix2Left_));
    in.read(reinterpret_cast<char*>(&mix2Right_), sizeof(mix2Right_));
    in.read(reinterpret_cast<char*>(&mix3Left_), sizeof(mix3Left_));
    in.read(reinterpret_cast<char*>(&mix3Right_), sizeof(mix3Right_));
    in.read(reinterpret_cast<char*>(&mix4Left_), sizeof(mix4Left_));
    in.read(reinterpret_cast<char*>(&mix4Right_), sizeof(mix4Right_));

    in.read(reinterpret_cast<char*>(&leftVolume_), sizeof(leftVolume_));
    in.read(reinterpret_cast<char*>(&rightVolume_), sizeof(rightVolume_));

    in.read(reinterpret_cast<char*>(&DIV_), sizeof(DIV_));
    in.read(reinterpret_cast<char*>(&NR50_), sizeof(NR50_));
    in.read(reinterpret_cast<char*>(&NR51_), sizeof(NR51_));

    channel1_.Deserialize(in);
    channel2_.Deserialize(in);
    channel3_.Deserialize(in);
    channel4_.Deserialize(in);
}

void APU::EnableSoundChannel(int const channel, bool const enabled)
{
    switch (channel)
    {
        case 1:
            channel1Disabled_ = !enabled;
            break;
        case 2:
            channel2Disabled_ = !enabled;
            break;
        case 3:
            channel3Disabled_ = !enabled;
            break;
        case 4:
            channel4Disabled_ = !enabled;
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
