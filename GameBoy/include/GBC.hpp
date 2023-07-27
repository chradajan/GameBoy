#pragma once

#include <cstdint>

extern "C"
{
/// @brief Initialize the Game Boy before use.
/// @param[in] frameBuffer Frame buffer to write pixel data to. Must be 69120 bytes (width * height * bpp = 160 * 144 * 3).
/// @param[in] savePath Path to directory to store save data for games with battery-backed SRAM.
/// @param[in] bootRomPath Path of Game Boy Color boot ROM file. If not provided, boot up sequence will be skipped.
void Initialize(uint8_t* frameBuffer,
                char* savePath,
                char* bootRomPath);

/// @brief Update the game loaded into the Game Boy.
/// @param[in] romPath Path to .gb or .gbc file to load.
void InsertCartridge(char* romPath);

/// @brief Load up the game ROM and boot ROM (if they are provided),and reset the Game Boy to its initial power up state.
void PowerOn();

/// @brief Unload the current game ROM and create a save file if its battery-backed.
void PowerOff();

/// @brief Check if a frame is ready to be rendered to the screen.
/// @return True if a full frame is loaded up in the frame buffer.
bool FrameReady();

/// @brief Run the Game Boy and collect the specified number of audio samples. If a frame is ready to be displayed while collecting
///        samples, call the frame ready callback.
/// @param buffer Buffer to write 2-channel 32-bit float PCM samples to.
/// @param numSamples Number of samples to collect.
void CollectAudioSamples(float* buffer, size_t numSamples);

/// @brief Specify a callback function used for rendering the frame buffer.
/// @param[in] callback Function to call anytime the frame buffer is ready to be rendered.
void SetFrameReadyCallback(void(*callback)());

/// @brief Update the Joypad register based on which buttons are currently pressed.
/// @param[in] down True if the down button is currently pressed.
/// @param[in] up True if the up button is currently pressed.
/// @param[in] left True if the left button is currently pressed.
/// @param[in] right True if the right button is currently pressed.
/// @param[in] start True if the start button is currently pressed.
/// @param[in] select True if the select button is currently pressed.
/// @param[in] b True if the b button is currently pressed.
/// @param[in] a True if the a button is currently pressed.
void SetInputs(bool down, bool up, bool left, bool right, bool start, bool select, bool b, bool a);

/// @brief Change how fast the emulated CPU runs to alter emulation speed.
/// @param multiplier Clock speed multiplier.
void SetClockMultiplier(float multiplier);
}
