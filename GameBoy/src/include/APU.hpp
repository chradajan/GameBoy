#pragma once

#include <Channel1.hpp>
#include <Channel2.hpp>
#include <Channel3.hpp>
#include <Channel4.hpp>
#include <cstdint>
#include <fstream>

class APU
{
public:
    void Clock();
    void PowerOn(bool skipBootRom);

    void GetAudioSample(float* left, float* right);

    void ClockDIV(bool doubleSpeed);
    void ResetDIV(bool doubleSpeed);
    uint8_t GetDIV() const { return DIV_; }

    uint8_t Read(uint8_t ioAddr);
    void Write(uint8_t ioAddr, uint8_t data);

    void Serialize(std::ofstream& out);
    void Deserialize(std::ifstream& in);

private:
    float HPF(float input);
    void AdvanceFrameSequencer();

    // State
    bool apuEnabled_;
    float capacitor_;

    // APU DIV
    uint8_t divDivider_;
    uint8_t envelopeDivider_;
    uint8_t soundLengthDivider_;
    uint8_t ch1FreqDivider_;

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
    Channel1 channel1_;
    Channel2 channel2_;
    Channel3 channel3_;
    Channel4 channel4_;
};
