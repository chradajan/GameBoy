#pragma once

#include <Channel2.hpp>
#include <cstdint>

class APU
{
public:
    void Clock();
    void GetAudioSample(float* left, float* right);

    void ClockDIV(bool doubleSpeed);
    void ResetDIV(bool doubleSpeed);
    uint8_t GetDIV() const { return DIV_; }

    uint8_t Read(uint8_t ioAddr);
    void Write(uint8_t ioAddr, uint8_t data);

    void Reset();

private:
    float HPF(float input);

    // State
    bool apuEnabled_;
    uint8_t divCounter_;
    float capacitor_;

    // Control
    bool mix1Left_;
    bool mix1Right_;
    bool mix2Left_;
    bool mix2Right_;
    bool mix3Left_;
    bool mix3Right_;
    bool mix4Left_;
    bool mix4Right_;

    float leftVolume_;
    float rightVolume_;

    // Registers
    uint8_t DIV_;
    uint8_t NR50_;
    uint8_t NR51_;

    // Channels
    Channel2 channel2_;
};
