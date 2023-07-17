#include <Channel2.hpp>

constexpr bool DUTY_CYCLE[4][8] = {
    {1, 1, 1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 0, 0},
    {1, 1, 1, 1, 0, 0, 0, 0},
    {1, 1, 0, 0, 0, 0, 0, 0}
};

void Channel2::Clock()
{
    ++periodCounter_;

    if (periodCounter_ == 0x800)
    {
        dutyCycleIndex_ = (dutyCycleIndex_ + 1) & 0x03;
        periodCounter_ = period_;
    }
}

void Channel2::ApuDiv()
{
    ++lengthTimerDivider_;
    ++envelopeDivider_;

    if (lengthTimerDivider_ == 2)  // 256 Hz
    {
        lengthTimerDivider_ = 0;
        ++lengthTimer_;

        if (lengthTimer_ == 64)
        {
            SetLengthTimer();

            if (SoundLengthEnable())
            {
                lengthTimerExpired_ = true;
            }
        }
    }

    if (envelopeDivider_ == 8)  // 64 Hz
    {
        envelopeDivider_ = 0;

        if (increaseEnvelope_)
        {
            if (volume_ + sweepPace_ > 0x0F)
            {
                volume_ = 0x0F;
            }
            else
            {
                volume_ += sweepPace_;
            }
        }
        else
        {
            if (sweepPace_ > volume_)
            {
                volume_ = 0x00;
            }
            else
            {
                volume_ -= sweepPace_;
            }
        }
    }
}

uint8_t Channel2::GetSample()
{
    if (dacEnabled_ && !lengthTimerExpired_ && DUTY_CYCLE[dutyCycle_][dutyCycleIndex_])
    {
        return volume_;
    }

    return 0x00;
}

void Channel2::Reset()
{
    NR21_ = 0x3F;
    NR22_ = 0x00;
    NR23_ = 0xFF;
    NR24_ = 0xBF;

    dacEnabled_ = false;
}

uint8_t Channel2::Read(uint8_t ioAddr)
{
    switch (ioAddr)
    {
        case 0x16:  // NR21
            return (NR21_ & 0xC0) | 0x3F;
        case 0x17:  // NR22
            return NR22_;
        case 0x18:  // NR23
            return 0xFF;
        case 0x19:  // NR24
            return (NR24_ & 0x40) | 0xBF;
        default:
            return 0xFF;
    }
}

void Channel2::Write(uint8_t ioAddr, uint8_t data)
{
    switch (ioAddr)
    {
        case 0x16:  // NR21
            NR21_ = data;
            SetDutyCycle();
            SetLengthTimer();
            break;
        case 0x17:  // NR22
            NR22_ = data;
            break;
        case 0x18:  // NR23
            NR23_ = data;
            SetPeriod();
            break;
        case 0x19:  // NR24
            NR24_ = data;
            SetPeriod();

            if (data & 0x80)
            {
                Trigger();
            }

            break;
        default:
            break;
    }
}

void Channel2::Trigger()
{
    SetDutyCycle();
    dutyCycleIndex_ = 0;

    SetLengthTimer();
    lengthTimerDivider_ = 0;
    lengthTimerExpired_ = false;

    volume_ = NR22_ & 0xF0;
    increaseEnvelope_ = NR22_ & 0x08;
    sweepPace_ = NR22_ & 0x07;
    envelopeDivider_ = 0;
    dacEnabled_ = volume_ || increaseEnvelope_;

    SetPeriod();
    periodCounter_ = period_;
}
