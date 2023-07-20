#pragma once

#include <cstdint>

class Channel4
{
public:
    void Clock();
    void ClockEnvelope();
    void ClockLengthTimer();

    bool Enabled() const { return dacEnabled_ && !lengthTimerExpired_; }
    bool DACEnabled() const { return dacEnabled_; }

    float GetSample() const;

    uint8_t Read(uint8_t ioAddr) const;
    void Write(uint8_t ioAddr, uint8_t data);

    void Reset();

private:
    void Trigger();

    void SetLengthCounter() { lengthCounter_ = NR41_ & 0x3F; }
    bool SoundLengthTimerEnabled() const { return NR44_ & 0x40; }
    bool ShortMode() const { return NR43_ & 0x08; }
    void SetLsfrDivider();

    // Registers
    uint8_t NR41_;
    uint8_t NR42_;
    uint8_t NR43_;
    uint8_t NR44_;

    // Length timer
    uint8_t lengthCounter_;
    bool lengthTimerExpired_;

    // Volume
    uint8_t currentVolume_;
    bool increaseVolume_;
    uint8_t volumeSweepPace_;
    uint8_t volumeSweepDivider_;

    // Shift register
    uint16_t LFSR_;
    uint16_t lsfrCounter_;
    uint16_t lsfrDivider_;
    bool shortMode_;

    // DAC
    bool dacEnabled_;

    // State
    bool triggered_;
};
