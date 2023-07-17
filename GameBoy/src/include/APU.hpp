#pragma once

#include <Channel2.hpp>
#include <cstdint>

class APU
{
public:
    void Clock();
    float GetSample();

    void ClockDIV(bool doubleSpeed);
    void ResetDIV(bool doubleSpeed);
    uint8_t GetDIV() const { return DIV_; }

    void Reset();

    uint8_t Read(uint8_t ioAddr);
    void Write(uint8_t ioAddr, uint8_t data);

private:
    // State
    uint8_t divCounter_;

    // Registers
    uint8_t DIV_;

    // Channels
    Channel2 channel2_;
};
