#pragma once

#include <cstdint>

class Channel2
{
public:
    void Clock();
    void ApuDiv();

    float GetSample();

    uint8_t Read(uint8_t ioAddr);
    void Write(uint8_t ioAddr, uint8_t data);

    void Reset();

private:
    void Trigger();

    void SetDutyCycle() { dutyCycle_ = (NR21_ & 0xC0) >> 6; }
    void SetLengthTimer() { lengthTimer_ = NR21_ & 0x3F; }
    void SetPeriod() { period_ = ((NR24_ & 0x07) << 8) | NR23_; }
    bool SoundLengthEnable() { return NR24_ & 0x40; }
    bool DACEnabled() { return NR22_ & 0xF8; }

    // Registers
    uint8_t NR21_;  // Length timer & duty cycle
    uint8_t NR22_;  // Volume & envelope
    uint8_t NR23_;  // Period low
    uint8_t NR24_;  // Period high & control

    // State
    uint8_t dutyCycle_;
    uint8_t dutyStep_;

    uint8_t lengthTimer_;
    uint8_t lengthTimerDivider_;
    bool lengthTimerExpired_;

    uint8_t volume_;
    bool increaseEnvelope_;
    uint8_t sweepPace_;
    uint8_t envelopeDivider_;
    bool dacEnabled_;

    uint16_t period_;
    uint16_t periodCounter_;
};
