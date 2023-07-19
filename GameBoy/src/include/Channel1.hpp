#pragma once

#include <cstdint>

class Channel1
{
public:
    void Clock();
    void ClockEnvelope();
    void ClockLengthTimer();
    void ClockFrequencySweep();

    bool Enabled() const { return dacEnabled_ && !frequencySweepOverflow_ && (!SoundLengthTimerEnabled() || !lengthTimerExpired_); }
    bool DACEnabled() const { return dacEnabled_; }

    float GetSample() const;

    uint8_t Read(uint8_t ioAddr) const;
    void Write(uint8_t ioAddr, uint8_t data);

    void Reset();

private:
    void Trigger();

    void SetFrequencySweepPace() { frequencySweepPace_ = (NR10_ & 0x70) >> 4; }
    bool FrequencySweepAdditionMode() const { return (NR10_ & 0x08) == 0x00; }
    uint8_t GetFrequencySweepSlope() const { return NR10_ & 0x07; }
    uint8_t GetDutyCycle() const { return (NR11_ & 0xC0) >> 6; }
    uint16_t GetPeriod() const { return ((NR14_ & 0x07) << 8) | NR13_; }
    void SetLengthCounter() { lengthCounter_ = NR11_ & 0x3F; }
    bool SoundLengthTimerEnabled() const { return NR14_ & 0x40; }

    void SetPeriod(uint16_t period);

    // Duty cycle patterns
    static constexpr int8_t DUTY_CYCLE[4][8] =
    {
        {1, 1, 1, 1, 1, 1, 1, -1},
        {1, 1, 1, 1, 1, 1, -1, -1},
        {1, 1, 1, 1, -1, -1, -1, -1},
        {1, 1, -1, -1, -1, -1, -1, -1}
    };

    // Registers
    uint8_t NR10_;
    uint8_t NR11_;
    uint8_t NR12_;
    uint8_t NR13_;
    uint8_t NR14_;

    // Frequency sweep
    uint8_t frequencySweepPace_;
    bool reloadFrequencySweepPace_;
    uint8_t frequencySweepDivider_;
    bool frequencySweepOverflow_;

    // Length timer
    uint8_t lengthCounter_;
    bool lengthTimerExpired_;

    // Duty cycle
    uint8_t dutyStep_;

    // Volume
    uint8_t currentVolume_;
    bool increaseVolume_;
    uint8_t volumeSweepPace_;
    uint8_t volumeSweepDivider_;

    // Period
    uint16_t periodDivider_;

    // DAC
    bool dacEnabled_;
};
