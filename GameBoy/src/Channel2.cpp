#include <Channel2.hpp>

void Channel2::Clock()
{
    ++periodDivider_;

    if (periodDivider_ == 0x0800)
    {
        periodDivider_ = GetPeriod();
        dutyStep_ = (dutyStep_ + 1) % 8;
    }
}

void Channel2::ClockEnvelope()
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

void Channel2::ClockLengthTimer()
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

float Channel2::GetSample() const
{
    if (!dacEnabled_)
    {
        return 0.0;
    }

    uint_fast8_t volume = (SoundLengthTimerEnabled() && lengthTimerExpired_) ? 0x00 : currentVolume_;
    return ((volume * DUTY_CYCLE[GetDutyCycle()][dutyStep_]) / 7.5) - 1.0;
}

uint8_t Channel2::Read(uint8_t ioAddr) const
{
    switch (ioAddr)
    {
        case 0x16:  // NR21
            return NR21_ | 0x3F;
        case 0x17:  // NR22
            return NR22_;
        case 0x18:  // NR23
            return 0xFF;
        case 0x19:  // NR24
            return (NR24_ & 0x40) | 0xBF;
        default:
            break;
    }

    return 0xFF;
}

void Channel2::Write(uint8_t ioAddr, uint8_t data)
{
    switch (ioAddr)
    {
        case 0x16:  // NR21
            NR21_ = data;
            SetLengthCounter();
            break;
        case 0x17:  // NR22
            NR22_ = data;
            break;
        case 0x18:  // NR23
            NR23_ = data;
            break;
        case 0x19:  // NR24
            NR24_ = data;

            if (data & 0x80)
            {
                Trigger();
            }
            break;
        default:
            break;
    }
}

void Channel2::Reset()
{
    NR21_ = 0x00;
    NR22_ = 0x00;
    NR23_ = 0x00;
    NR24_ = 0x00;
    dutyStep_ = 0;
    dacEnabled_ = false;
}

void Channel2::Trigger()
{
    SetLengthCounter();
    lengthTimerExpired_ = false;

    currentVolume_ = (NR22_ & 0xF0) >> 4;
    increaseVolume_ = NR22_ & 0x08;
    volumeSweepPace_ = NR22_ & 0x07;
    volumeSweepDivider_ = 0;
    dacEnabled_ = (NR22_ & 0xF8) != 0x00;

    periodDivider_ = GetPeriod();
}
