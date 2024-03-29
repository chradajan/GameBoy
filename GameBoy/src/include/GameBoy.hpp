#pragma once

#include <Cartridge/Cartridge.hpp>
#include <APU.hpp>
#include <CPU.hpp>
#include <PPU.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace IO
{
/// @brief Enum of I/O Register addresses. Only use lower byte since upper byte is 0xFF for all addresses.
enum : uint8_t
{
    // Joypad
    JOYP = 0x00, // Joypad input

    // Serial transfer
    SB = 0x01, // Serial transfer data
    SC = 0x02, // Serial transfer control

    // Timer
    DIV = 0x04,  // Divider register
    TIMA = 0x05, // Timer counter
    TMA = 0x06,  // Timer modulo
    TAC = 0x07,  // Timer control

    // Interrupts
    IF = 0x0F, // Interrupt flag

    // Sound
    NR10 = 0x10, // Sound channel 1 sweep
    NR11 = 0x11, // Sound channel 1 length timer & duty cycle
    NR12 = 0x12, // Sound channel 1 volume & envelope
    NR13 = 0x13, // Sound channel 1 period low
    NR14 = 0x14, // Sound channel 1 period high & sound control

    NR21 = 0x16, // Sound channel 2 length timer & duty cycle
    NR22 = 0x17, // Sound channel 2 volume & envelope
    NR23 = 0x18, // Sound channel 2 period low
    NR24 = 0x19, // Sound channel 2 period high & control

    NR30 = 0x1A, // Sound channel 3 DAC enable
    NR31 = 0x1B, // Sound channel 3 length timer
    NR32 = 0x1C, // Sound channel 3 output level
    NR33 = 0x1D, // Sound channel 3 period low
    NR34 = 0x1E, // Sound channel 3 period high & control

    NR41 = 0x20, // Sound channel 4 length timer
    NR42 = 0x21, // Sound channel 4 volume & envelope
    NR43 = 0x22, // Sound channel 4 frequency & randomness
    NR44 = 0x23, // Sound channel 4 control

    NR50 = 0x24, // Master volume & VIN panning
    NR51 = 0x25, // Sound panning
    NR52 = 0x26, // Sound on/off

    WAVE_RAM_START = 0x30, // First index of Wave RAM
    WAVE_RAM_END = 0x3F,   // Last index of Wave RAM

    // OAM
    DMA = 0x46, // OAM DMA source address

    // Speed switch
    KEY1 = 0x4D, // Prepare speed switch (CGB mode only)

    // Boot ROM
    UNMAP_BOOT_ROM = 0x50, // Boot ROM disable

    // VRAM DMA
    HDMA1 = 0x51, // VRAM DMA source high (CGB mode only)
    HDMA2 = 0x52, // VRAM DMA source low (CGB mode only)
    HDMA3 = 0x53, // VRAM DMA destination high (CGB mode only)
    HDMA4 = 0x54, // VRAM DMA destination low (CGB mode only)
    HDMA5 = 0x55, // VRAM DMA length/mode/start (CGB mode only)

    // Infrared
    RP = 0x56, // Infrared communication port (CGB mode only)

    // WRAM
    SVBK = 0x70, // WRAM bank

    // Undocumented
    ff72 = 0x72,  // Fully readable/writable (CGB mode only)
    ff73 = 0x73,  // Fully readable/writable (CGB mode only)
    ff74 = 0x74,  // Fully readable/writable (CGB mode only)
    ff75 = 0x75,  // Bits 4, 5, and 6 are readable/writable (CGB mode only)
    PCM12 = 0x76, // Copies sound channel 1 and 2's PCM amplitudes
    PCM34 = 0x77, // Copies sound channel 3 and 4's PCM amplitudes
};
}  // namespace IO_REG

namespace INT_SRC
{
/// @brief Enum of interrupt source masks.
enum : uint8_t
{
    VBLANK = 0x01,
    LCD_STAT = 0x02,
    TIMER = 0x04,
    SERIAL = 0x08,
    JOYPAD = 0x10,
};
}  // namespace INT_SRC

class GameBoy
{
public:
    /// @brief GameBoy constructor. Handles creation of all components.
    GameBoy();

    void Initialize(uint8_t* frameBuffer);

    /// @brief Load cartridge data from GameBoy ROM.
    /// @param[in] romPath Path to gb ROM file.
    /// @param[in] saveDirectory Directory to save and load cartridge SRAM from.
    /// @param[out] romName ROM name from cartridge header.
    /// @return True if ROM was successfully loaded.
    bool InsertCartridge(std::filesystem::path romPath, std::filesystem::path saveDirectory, char* romName);

    /// @brief Prepare the GameBoy to be run.
    /// @param[in] bootRomPath Path to boot ROM.
    void PowerOn(std::filesystem::path bootRomPath);

    /// @brief Run the Game Boy for some number of machine cycles. Return early if the frame buffer is ready to be displayed.
    /// @param[in] numCycles Number of machine cycles to run it for.
    /// @pre Initialize, InsertCartridge, and PowerOn must have been called.
    /// @return Pair consisting of how many cycles were actually run and whether the screen should be refreshed.
    std::pair<int, bool> Clock(int numCycles);

    bool FrameReady() { return ppu_.FrameReady(); }

    /// @brief Set the sample rate used for audio playback. This is used to set parameters of the low pass filter.
    /// @param sampleRate Sample rate to downsample to.
    void SetSampleRate(int sampleRate) { apu_.SetSampleRate(sampleRate); }

    /// @brief Apply low pass filters and downsampling to sample buffers and fill playback buffer.
    /// @param buffer Pointer to buffer to fill.
    /// @param count Buffer size. Number of samples to provide is half of this due to stereo playback.
    void DrainSampleBuffer(float* buffer, int count) { apu_.DrainSampleBuffer(buffer, count); };

    /// @brief Update which buttons are currently being pressed.
    /// @param[in] down True if down is currently pressed.
    /// @param[in] up True if up is currently pressed.
    /// @param[in] left True if left is currently pressed.
    /// @param[in] right True if right is currently pressed.
    /// @param[in] start True if start is currently pressed.
    /// @param[in] select True if select is currently pressed.
    /// @param[in] b True if b is currently pressed.
    /// @param[in] a True if a is currently pressed.
    void SetButtons(bool down, bool up, bool left, bool right, bool start, bool select, bool b, bool a)
    {
        buttons_ = {down, up, left, right, start, select, b, a};
    }

    bool IsSerializable() const;
    void Serialize(std::ofstream& out);
    void Deserialize(std::ifstream& in);

    /// @brief Set whether a specific sound channel should be mixed in to the APU output.
    /// @param channel Channel number to set (1-4).
    /// @param enabled True to enable channel, false to disable it.
    void EnableSoundChannel(int channel, bool enabled) { apu_.EnableSoundChannel(channel, enabled); }

    /// @brief Choose whether to output
    /// @param monoAudio True to use mono, false to use stereo.
    void SetMonoAudio(bool monoAudio) { apu_.SetMonoAudio(monoAudio); }

    /// @brief Set the volume of the APU output.
    /// @param volume Volume of output (between 0.0 and 1.0).
    void SetVolume(float volume) { apu_.SetVolume(volume); }

    /// @brief Use custom DMG palettes when playing GB games. Toggle through GUI.
    /// @param useDmgColors True if DMG colors should be used.
    void PreferDmgColors(bool useDmgColors) { ppu_.PreferDmgColors(useDmgColors); }

    /// @brief Determine whether background, window, obp0, and obp1 should use the same palette or individual ones.
    /// @param individualPalettes True if each pixel type should use its own palette.
    void UseIndividualPalettes(bool individualPalettes) { ppu_.UseIndividualPalettes(individualPalettes); }

    /// @brief Specify colors in one of the custom DMG palettes.
    /// @param index Index of palette to update.
    ///                 0 = Universal palette
    ///                 1 = Background
    ///                 2 = Window
    ///                 3 = OBP0
    ///                 4 = OBP1
    /// @param data Pointer to RGB data (12 0-255 values)
    void SetCustomPalette(uint8_t index, uint8_t* data) { ppu_.SetCustomPalette(index, data); }

private:
    /// @brief Execute the specified number of machine cycles.
    /// @param numCycles Number of machine cycles to execute.
    std::pair<int, bool> RunMCycles(int numCycles);

    void ClockVariableSpeedComponents(bool clockCpu);

    /// @brief Clock the timer components.
    void ClockTimer();

    /// @brief Run one cycle of an OAM DMA transfer.
    void ClockOamDma();

    void ClockVramDma();

    /// @brief Run one cycle of a serial transfer.
    void ClockSerialTransfer();

    /// @brief Update JOYP with most recently pressed buttons when written.
    /// @param[in] data Data being written to JOYP.
    void UpdateJOYP(uint8_t data);

    /// @brief Check for any pending interrupts (IF & IE).
    /// @return If an interrupt is pending, return the address to jump to and the total number of pending interrupts.
    std::optional<std::pair<uint16_t, uint8_t>> CheckPendingInterrupts();

    /// @brief If a an interrupt was pending during the last call to `CheckPendingInterrupts`, clear that interrupt from the IF
    ///        register.
    void AcknowledgeInterrupt();

    void CheckVBlankInterrupt();

    void CheckStatInterrupt();

    /// @brief Check the current speed mode.
    /// @return True if running in double speed, false if normal speed.
    bool DoubleSpeedMode() const { return cgbMode_ && (ioReg_[IO::KEY1] & 0x80); }

    bool PrepareSpeedSwitch() const { return ioReg_[IO::KEY1] & 0x01; }

    void SwitchSpeedMode() { ioReg_[IO::KEY1] ^= 0x80; }

    /// @brief Determine what happens when CPU executes a STOP instruction.
    /// @param IME Whether interrupts are currently enabled in the CPU.
    /// @return First bool is true if STOP is a 2-byte opcode, false if it's a 1-byte opcode.
    ///         Second bool is true if HALT mode is entered, false otherwise.
    std::pair<bool, bool> Stop(bool IME);

    /// @brief Decode the provided address and read from the relevant location in memory.
    /// @param src Which component initiated this read.
    /// @param addr Address to read from.
    /// @return Byte located at the provided address.
    uint8_t Read(uint16_t addr);

    /// @brief Decode the provided address and write to the relevant location in memory.
    /// @param src Which component initiated this write.
    /// @param addr Address to write to.
    /// @param data Byte to write to provided address.
    void Write(uint16_t addr, uint8_t data);

    /// @brief Read a memory mapped I/O register.
    /// @param addr Address of register.
    /// @return Byte based on the register's read behavior.
    uint8_t ReadIoReg(uint16_t addr);

    /// @brief Write to a memory mapped I/O register.
    /// @param addr Address of register.
    /// @param data Data to write to register. Some registers may have unique write behaviors beyond writing data to memory.
    void WriteIoReg(uint16_t addr, uint8_t data);

    void IoWriteSC(uint8_t data);
    void IoWriteTAC(uint8_t data);
    void IoWriteDMA(uint8_t data);
    void IoWriteVramDMA(uint8_t data);

    void SetHDMARegisters();
    void SetVramDmaAddresses();

    void SetDefaultCgbIoValues();

    // Memory
    std::array<std::array<uint8_t, 0x1000>, 8> WRAM_;  // $C000 ... $DFFF
    std::array<uint8_t, 0x7F> HRAM_;  // $FF80 ... $FFFE
    std::array<uint8_t, 0x900> BOOT_ROM;  // Boot ROM
    uint8_t* frameBuffer_;

    // Joypad
    struct
    {
        bool down, up, left, right;  // Directions
        bool start, select, b, a; // Actions
    } buttons_;

    // I/O Registers
    std::array<uint8_t, 0x78> ioReg_;
    uint8_t IE_;

    // Mode
    bool cgbMode_;
    bool cgbCartridge_;
    bool runningBootRom_;
    bool stopped_;

    // Speed switch
    uint16_t speedSwitchCountdown_;

    // Serial transfer
    uint8_t serialOutData_;
    uint8_t serialBitsSent_;
    uint8_t serialTransferCounter_;
    uint8_t serialClockDivider_;
    bool serialTransferInProgress_;

    // Timer
    uint16_t timerCounter_;
    uint16_t timerControl_;
    bool timerEnabled_;
    bool timerReload_;

    // OAM DMA
    enum class OamDmaSrc
    {
        CART_ROM,
        VRAM,
        CART_RAM,
        WRAM
    };

    OamDmaSrc oamDmaSrc_;
    bool oamDmaInProgress_;
    uint8_t oamDmaCyclesRemaining_;
    uint16_t oamDmaSrcAddr_;
    uint16_t oamDmaDestAddr_;

    // VRAM DMA
    uint8_t vramDmaBlocksRemaining_;
    uint16_t vramDmaBytesRemaining_;
    uint16_t vramDmaSrc_;
    uint16_t vramDmaDest_;
    bool wasMode0_;
    bool gdmaInProgress_;
    bool hdmaInProgress_;
    bool transferActive_;

    // Interrupts
    uint8_t lastPendingInterrupt_;
    bool prevStatState_;

    // Components
    APU apu_;
    CPU cpu_;
    PPU ppu_;
    std::unique_ptr<Cartridge> cartridge_;
};
