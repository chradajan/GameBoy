#include <Channel4.hpp>
#include <algorithm>
#include <type_traits>
#include <vector>
#include <iostream>

static_assert(std::is_pod<Channel4>::value, "APU is not POD!");

void Channel4::PowerOn(bool const skipBootRom)
{
    if (skipBootRom)
    {
        NR41_ = 0xFF;
        NR42_ = 0x00;
        NR43_ = 0x00;
        NR44_ = 0xBF;
    }
    else
    {
        NR41_ = 0x00;
        NR42_ = 0x00;
        NR43_ = 0x00;
        NR44_ = 0x00;
    }

    dacEnabled_ = false;
    triggered_ = false;
}

float Channel4::Clock()
{
    ++lsfrCounter_;

    if (lsfrCounter_ == lsfrDivider_)
    {
        lsfrCounter_ = 0;
        bool result = ((LFSR_ & 0x02) >> 1) ^ (LFSR_ & 0x01);

        if (result)
        {
            LFSR_ |= ShortMode() ? 0x8080 : 0x8000;
        }
        else
        {
            LFSR_ &= ShortMode() ? 0x7F7F : 0x7FFF;
        }

        LFSR_ >>= 1;
    }

    return GetSample();
}

void Channel4::ClockEnvelope()
{
    if (volumeSweepPace_ == 0)
    {
        return;
    }

    ++volumeSweepDivider_;

    if (volumeSweepDivider_ == volumeSweepPace_)
    {
        volumeSweepDivider_ = 0;

        if (increaseVolume_ && (currentVolume_ < 0x0F))
        {
            ++currentVolume_;
        }
        else if (!increaseVolume_ && (currentVolume_ > 0x00))
        {
            --currentVolume_;
        }
    }
}

void Channel4::ClockLengthTimer()
{
    if (lengthTimerExpired_ || !SoundLengthTimerEnabled())
    {
        return;
    }

    ++lengthCounter_;

    if (lengthCounter_ == 64)
    {
        lengthTimerExpired_ = true;
    }
}

float Channel4::GetSample() const
{
    if (!dacEnabled_ || !triggered_)
    {
        return 0.0;
    }

    uint_fast8_t volume = (SoundLengthTimerEnabled() && lengthTimerExpired_) ? 0 : currentVolume_;
    return ((volume * (LFSR_ & 0x01)) / 7.5) - 1.0;
}

uint8_t Channel4::Read(uint8_t ioAddr) const
{
    switch (ioAddr)
    {
        case 0x20:  // NR41
            return NR41_ | 0xC0;
        case 0x21:  // NR42
            return NR42_;
        case 0x22:  // NR43
            return NR43_;
        case 0x23:  // NR44
            return NR44_ | 0x3F;
        default:
            break;
    }

    return 0xFF;
}

void Channel4::Write(uint8_t ioAddr, uint8_t data)
{
    switch (ioAddr)
    {
        case 0x20:  // NR41
            NR41_ = data;
            SetLengthCounter();
            break;
        case 0x21:  // NR42
            NR42_ = data;
            SetNR42Data();
            break;
        case 0x22:  // NR43
            NR43_ = data;
            SetLsfrDivider();
            break;
        case 0x23:  // NR44
            NR44_ = data;

            if (data & 0x80)
            {
                Trigger();
            }

            break;
        default:
            break;
    }
}

void Channel4::Serialize(std::ofstream& out)
{
    out.write(reinterpret_cast<char*>(this), sizeof(*this));
}

void Channel4::Deserialize(std::ifstream& in)
{
    in.read(reinterpret_cast<char*>(this), sizeof(*this));
}

void Channel4::Trigger()
{
    SetLengthCounter();
    lengthTimerExpired_ = false;

    SetNR42Data();

    SetLsfrDivider();
    LFSR_ = 0xFFFF;

    triggered_ = true;
}

void Channel4::SetLsfrDivider()
{
    constexpr uint_fast32_t cpuFreq = 1048576;
    constexpr uint_fast32_t baseFreq = 262144;
    uint_fast8_t r = NR43_ & 0x07;
    uint_fast8_t s = (NR43_ & 0xF0) >> 4;

    if (r == 0)
    {
        lsfrDivider_ = cpuFreq / ((baseFreq * 2) / (0x01 << s));
    }
    else
    {
        lsfrDivider_ = cpuFreq / (baseFreq / r / (0x01 << s));
    }

    lsfrCounter_ = 0;
}

void Channel4::SetNR42Data()
{
    currentVolume_ = (NR42_ & 0xF0) >> 4;
    increaseVolume_ = NR42_ & 0x08;
    volumeSweepPace_ = NR42_ & 0x07;
    volumeSweepDivider_ = 0;
    dacEnabled_ = (NR42_ & 0xF8) != 0x00;
}
