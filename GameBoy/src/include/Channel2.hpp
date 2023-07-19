#pragma once

#include <cstdint>

class Channel2
{
public:
    void Clock();
    void ClockEnvelope();
    void ClockLengthTimer();

    bool Enabled() const { return dacEnabled_ && (!SoundLengthTimerEnabled() || !lengthTimerExpired_); }
    bool DACEnabled() const { return dacEnabled_; }

    float GetSample() const;

    uint8_t Read(uint8_t ioAddr) const;
    void Write(uint8_t ioAddr, uint8_t data);

    void Reset();

private:
    void Trigger();

    uint8_t GetDutyCycle() const { return (NR21_ & 0xC0) >> 6; }
    uint16_t GetPeriod() const { return ((NR24_ & 0x07) << 8) | NR23_; }
    void SetLengthCounter() { lengthCounter_ = NR21_ & 0x3F; }
    bool SoundLengthTimerEnabled() const { return NR24_ & 0x40; }

    // Duty cycle patterns
    static constexpr int8_t DUTY_CYCLE[4][8] =
    {
        {1, 1, 1, 1, 1, 1, 1, -1},
        {1, 1, 1, 1, 1, 1, -1, -1},
        {1, 1, 1, 1, -1, -1, -1, -1},
        {1, 1, -1, -1, -1, -1, -1, -1}
    };

    // Registers
    uint8_t NR21_;
    uint8_t NR22_;
    uint8_t NR23_;
    uint8_t NR24_;

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
