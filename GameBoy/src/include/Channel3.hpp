#pragma once

#include <array>
#include <cstdint>

class Channel3
{
public:
    void Clock();
    void ClockLengthTimer();

    bool Enabled() const { return DACEnabled() && (!SoundLengthTimerEnabled() || !lengthTimerExpired_); }
    bool DACEnabled() const { return NR30_ & 0x80; }

    float GetSample();

    uint8_t Read(uint8_t ioAddr) const;
    void Write(uint8_t ioAddr, uint8_t data);

    void Reset();

private:
    void Trigger();

    bool SoundLengthTimerEnabled() const { return NR34_ & 0x40; }
    uint8_t GetOutputLevel() const { return (NR32_ & 0x60) >> 5; }
    uint16_t GetPeriod() const { return ((NR34_ & 0x07) << 8) | NR33_; }

    // Registers
    uint8_t NR30_;
    uint8_t NR31_;
    uint8_t NR32_;
    uint8_t NR33_;
    uint8_t NR34_;

    // Samples
    std::array<uint8_t, 32> Wave_RAM_;
    uint8_t sampleIndex_;
    float lastSample_;
    bool delayPlayback_;

    // Length timer
    uint16_t lengthCounter_;
    bool lengthTimerExpired_;

    // Period
    uint16_t periodDivider_;

    // State
    bool triggered_;
};
