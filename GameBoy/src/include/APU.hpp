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
    /// @brief Constructor to initialize GUI overrides.
    APU();

    /// @brief Clock each sound channel 1 m-cycle.
    void Clock();

    /// @brief Initialize the APU to its power on state.
    /// @param[in] skipBootRom Whether its being powered on into the boot ROM or straight into game.
    void PowerOn(bool skipBootRom);

    /// @brief Collect the current output of each channel and mix into a sample.
    /// @param[out] left Pointer to left audio output to set.
    /// @param[out] right Pointer to right audio output to set.
    void GetAudioSample(float* left, float* right);

    /// @brief Clock the DIV register and advance the frame sequencer if necessary.
    /// @param[in] doubleSpeed True if system is running in double speed mode. Used to determine when to advance frame sequencer.
    void ClockDIV(bool doubleSpeed);

    /// @brief Reset the DIV register and advance the frame sequencer if necessary.
    /// @param[in] doubleSpeed True if system is running in double speed mode. Used to determine whether to advance frame sequencer.
    void ResetDIV(bool doubleSpeed);

    /// @brief Get the DIV register.
    /// @return Current value of DIV.
    uint8_t GetDIV() const { return DIV_; }

    /// @brief Route read request to the appropiate APU register.
    /// @param[in] ioAddr Lower byte of address to read.
    /// @return Current value of selected register.
    uint8_t Read(uint8_t ioAddr);

    /// @brief Route write request to the appropriate APU register.
    /// @param[in] ioAddr Lower byte of address to write.
    /// @param[in] data Data to write to register.
    void Write(uint8_t ioAddr, uint8_t data);

    /// @brief Write the current state of the APU to disk.
    /// @param[in] out Stream to write state to.
    void Serialize(std::ofstream& out);

    /// @brief Restore the APU state to a previously serialized one.
    /// @param[in] in Stream to restore state from.
    void Deserialize(std::ifstream& in);

    /// @brief Set whether a specific sound channel should be mixed in to the APU output.
    /// @param channel Channel number to set (1-4).
    /// @param enabled True to enable channel, false to disable it.
    void EnableSoundChannel(int channel, bool enabled);

    /// @brief Choose whether to output
    /// @param[in] monoAudio True to use mono, false to use stereo.
    void SetMonoAudio(bool monoAudio) { monoAudio_ = monoAudio; }

    /// @brief Set the volume of the APU output.
    /// @param[in] volume Volume of output (between 0.0 and 1.0).
    void SetVolume(float volume) { volume_ = volume; }

private:
    /// @brief Simulate the APU's high pass filter that pulls output signal towards 0.
    /// @param[in] input Current APU output value.
    /// @return Output from high pass filter.
    float HPF(float input);

    /// @brief Clock the frame sequencer.Clocks the envelope, frequency sweep, and length timer of channels that support those.
    void AdvanceFrameSequencer();

    // GUI overrides
    bool monoAudio_;
    float volume_;

    bool channel1Disabled_;
    bool channel2Disabled_;
    bool channel3Disabled_;
    bool channel4Disabled_;

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
