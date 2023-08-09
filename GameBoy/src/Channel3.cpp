#include <Channel3.hpp>
#include <type_traits>

static_assert(std::is_pod<Channel3>::value, "APU is not POD!");

void Channel3::PowerOn(bool const skipBootRom)
{
    if (skipBootRom)
    {
        NR30_ = 0x7F;
        NR31_ = 0xFF;
        NR32_ = 0x9F;
        NR33_ = 0xFF;
        NR34_ = 0xBF;
    }
    else
    {
        NR30_ = 0x00;
        NR31_ = 0x00;
        NR32_ = 0x00;
        NR33_ = 0x00;
        NR34_ = 0x00;
    }

    sampleIndex_ = 0;
    lastSample_ = 0.0;
    triggered_ = false;
}

float Channel3::Clock()
{
    for (uint_fast8_t i = 0; i < 2; ++i)
    {
        ++periodDivider_;

        if (periodDivider_ == 0x0800)
        {
            periodDivider_ = GetPeriod();
            sampleIndex_ = (sampleIndex_ + 1) % 32;

            delayPlayback_ = false;
        }
    }

    return GetSample();
}

void Channel3::ClockLengthTimer()
{
    if (lengthTimerExpired_ || !SoundLengthTimerEnabled())
    {
        return;
    }

    ++lengthCounter_;

    if (lengthCounter_ == 256)
    {
        lengthTimerExpired_ = true;
    }
}

float Channel3::GetSample()
{
    if (!DACEnabled() || !triggered_)
    {
        return 0.0;
    }
    else if (delayPlayback_)
    {
        return lastSample_;
    }

    uint_fast8_t volume = (SoundLengthTimerEnabled() && lengthTimerExpired_) ? 0 : Wave_RAM_[sampleIndex_];

    switch (GetOutputLevel())
    {
        case 0:
            volume = 0;
            break;
        case 1:
            break;
        case 2:
            volume >>= 1;
            break;
        case 3:
            volume >>= 2;
            break;
        default:
            break;
    }

    lastSample_ = (volume / 7.5) - 1.0;
    return lastSample_;
}

uint8_t Channel3::Read(uint8_t const ioAddr) const
{
    switch(ioAddr)
    {
        case 0x1A:  // NR30
            return NR30_ | 0x7F;
        case 0x1B:  // NR31
            return 0xFF;
        case 0x1C:  // NR32
            return NR32_ | 0x9F;
        case 0x1D:  // NR33
            return 0xFF;
        case 0x1E:  // NR34
            return NR34_ | 0xBF;
        case 0x30 ... 0x3F:  // Wave RAM
        {
            uint_fast8_t readIndex;

            if (Enabled())
            {
                readIndex = ((sampleIndex_ % 2 == 0)) ? sampleIndex_ : (sampleIndex_ - 1);
            }
            else
            {
                readIndex = (ioAddr - 0x30) * 2;
            }

            return (Wave_RAM_[readIndex] << 4) | (Wave_RAM_[readIndex + 1]);
        }
        default:
            break;
    }

    return 0xFF;
}

void Channel3::Write(uint8_t const ioAddr, uint8_t const data)
{
    switch(ioAddr)
    {
        case 0x1A:  // NR30
            NR30_ = data;;
            break;
        case 0x1B:  // NR31
            NR31_ = data;
            lengthCounter_ = data;
            break;
        case 0x1C:  // NR32
            NR32_ = data;
            break;
        case 0x1D:  // NR33
            NR33_ = data;
            break;
        case 0x1E:  // NR34
            NR34_ = data;

            if (data & 0x80)
            {
                Trigger();
            }

            break;
        case 0x30 ... 0x3F:  // Wave RAM
        {
            uint_fast8_t writeIndex = (ioAddr - 0x30) * 2;
            Wave_RAM_[writeIndex] = (data & 0xF0) >> 4;
            Wave_RAM_[writeIndex + 1] = (data & 0x0F);
            break;
        }
        default:
            break;
    }
}

void Channel3::Serialize(std::ofstream& out)
{
    out.write(reinterpret_cast<char*>(this), sizeof(*this));
}

void Channel3::Deserialize(std::ifstream& in)
{
    in.read(reinterpret_cast<char*>(this), sizeof(*this));
}

void Channel3::Trigger()
{
    sampleIndex_ = 0;
    delayPlayback_ = true;

    lengthCounter_ = NR31_;
    lengthTimerExpired_ = false;

    periodDivider_ = GetPeriod();

    triggered_ = true;
}
