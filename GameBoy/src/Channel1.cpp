#include <Channel1.hpp>

void Channel1::PowerOn(bool const skipBootRom)
{
    if (skipBootRom)
    {
        NR10_ = 0x80;
        NR11_ = 0xBF;
        NR12_ = 0xF3;
        NR13_ = 0xFF;
        NR14_ = 0xBF;
    }
    else
    {
        NR10_ = 0x00;
        NR11_ = 0x00;
        NR12_ = 0x00;
        NR13_ = 0x00;
        NR14_ = 0x00;
    }

    dutyStep_ = 0;
    dacEnabled_ = false;
    triggered_ = false;
}

void Channel1::Clock()
{
    ++periodDivider_;

    if (periodDivider_ == 0x0800)
    {
        periodDivider_ = GetPeriod();
        dutyStep_ = (dutyStep_ + 1) % 8;
    }
}

void Channel1::ClockEnvelope()
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

void Channel1::ClockLengthTimer()
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

void Channel1::ClockFrequencySweep()
{
    if (frequencySweepOverflow_ || (frequencySweepPace_ == 0))
    {
        return;
    }

    ++frequencySweepDivider_;

    if (frequencySweepDivider_ == frequencySweepPace_)
    {
        frequencySweepDivider_ = 0;
        uint_fast16_t currentPeriod = GetPeriod();
        uint_fast16_t diff = currentPeriod / (0x01 << GetFrequencySweepSlope());

        if (FrequencySweepAdditionMode())
        {
            uint_fast16_t updatedPeriod = currentPeriod + diff;

            if (updatedPeriod > 0x07FF)
            {
                frequencySweepOverflow_ = true;
            }
            else
            {
                SetPeriod(updatedPeriod);
            }
        }
        else
        {
            uint_fast16_t updatedPeriod = (diff > currentPeriod) ? 0x0000 : (currentPeriod - diff);
            SetPeriod(updatedPeriod);
        }

        if (reloadFrequencySweepPace_)
        {
            SetFrequencySweepPace();
        }
    }
}

float Channel1::GetSample() const
{
    if (!dacEnabled_ || !triggered_)
    {
        return 0.0;
    }

    uint_fast8_t volume = (frequencySweepOverflow_ || lengthTimerExpired_) ? 0x00 : currentVolume_;
    return ((volume * DUTY_CYCLE[GetDutyCycle()][dutyStep_]) / 7.5) - 1.0;
}

uint8_t Channel1::Read(uint8_t ioAddr) const
{
    switch (ioAddr)
    {
        case 0x10:  // NR10
            return NR10_ | 0x80;
        case 0x11:  // NR11
            return NR11_ | 0x3F;
        case 0x12:  // NR12
            return NR12_;
        case 0x13:  // NR13
            return 0xFF;
        case 0x14:  // NR14
            return (NR14_ & 0x40) | 0xBF;
        default:
            break;
    }

    return 0xFF;
}

void Channel1::Write(uint8_t ioAddr, uint8_t data)
{
    switch (ioAddr)
    {
        case 0x10:  // NR10
        {
            NR10_ = data;

            if (data & 0x70)
            {
                reloadFrequencySweepPace_ = true;
            }
            else
            {
                SetFrequencySweepPace();
                reloadFrequencySweepPace_ = false;
            }
            break;
        }
        case 0x11:  // NR11
            NR11_ = data;
            SetLengthCounter();
            break;
        case 0x12:  // NR12
            NR12_ = data;
            break;
        case 0x13:  // NR13
            NR13_ = data;
            break;
        case 0x14:  // NR14
            NR14_ = data;

            if (data & 0x80)
            {
                Trigger();
            }
            break;
        default:
            break;
    }
}

void Channel1::Trigger()
{
    SetFrequencySweepPace();
    reloadFrequencySweepPace_ = false;
    frequencySweepDivider_ = 0;
    frequencySweepOverflow_ = false;

    SetLengthCounter();
    lengthTimerExpired_ = false;

    currentVolume_ = (NR12_ & 0xF0) >> 4;
    increaseVolume_ = NR12_ & 0x08;
    volumeSweepPace_ = NR12_ & 0x07;
    volumeSweepDivider_ = 0;
    dacEnabled_ = (NR12_ & 0xF8) != 0x00;

    periodDivider_ = GetPeriod();

    triggered_ = true;
}

void Channel1::SetPeriod(uint16_t period)
{
    NR14_ = (NR14_ & 0xF8) | ((period & 0x0700) >> 8);
    NR13_ = period & 0x00FF;
}
