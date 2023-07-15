#pragma once

#include <Cartridge/Cartridge.hpp>
#include <Controller.hpp>
#include <CPU.hpp>
#include <PPU.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <utility>

constexpr size_t FRAME_BUFFER_SIZE = 160 * 144 * 3;

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

    // LCD
    LCDC = 0x40, // LCD control
    STAT = 0x41, // LCD status
    SCY = 0x42,  // Viewport Y position
    SCX = 0x43,  // Viewport X position
    LY = 0x44,   // LCD Y coordinate
    LYC = 0x45,  // LY compare

    // OAM
    DMA = 0x46, // OAM DMA source address

    // LCD
    BGP = 0x47,  // BG palette data (Non-CGB mode only)
    OBP0 = 0x48, // OBJ palette 0 data (Non-CGB mode only)
    OBP1 = 0x49, // OBJ palette 1 data (Non-CGB mode only)
    WY = 0x4A,   // Window y position
    WX = 0x4B,   // Window x position

    // Speed switch
    KEY1 = 0x4D, // Prepare speed switch (CGB mode only)

    // VRAM
    VBK = 0x4F, // VRAM bank (CGB mode only)

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

    // LCD
    BCPS = 0x68, // Background color palette specification (CGB mode only)
    BCPD = 0x69, // Background color palette data (CGB mode only)

    OCPS = 0x6A, // OBJ color palette specification (CGB mode only)
    OCPD = 0x6B, // OBJ color palette data (CGB mode only)
    OPRI = 0x6C, // OBJ priority mode (CGB mode only)

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
    /// @param frameBuffer Pointer to buffer to store pixel data.
    GameBoy(std::array<uint8_t, FRAME_BUFFER_SIZE>& frameBuffer);

    /// @brief Infinitely run the GameBoy. Immediately returns if no cartridge is loaded.
    void Run();

    /// @brief Reset the GameBoy to its power-on state.
    void Reset();

    /// @brief Load cartridge data from gb ROM.
    /// @param romPath Path to gb ROM file.
    void InsertCartridge(fs::path romPath);

    void PowerOn();

    bool FrameReady();

    /// @brief Update JOYP based on which buttons are pressed and which buttons are selected to read. If stop mode is active and a
    ///        button is pressed, exit stop mode.
    /// @param buttons Buttons currently being pressed.
    void SetInputs(Buttons const& buttons);

private:
    void RunMCycle();

    void ClockVariableSpeedComponents(bool clockCpu);

    /// @brief Clock the timer components.
    void ClockTimer();

    /// @brief Run one cycle of an OAM DMA transfer.
    void ClockOamDma();

    void ClockVramGDMA();

    void ClockVramHDMA();

    /// @brief Run one cycle of a serial transfer.
    void ClockSerialTransfer();

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
    bool DoubleSpeedMode() const { return ioReg_[IO::KEY1] & 0x80; }

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
    void IoWriteBCPD(uint8_t data);
    void IoWriteOCPD(uint8_t data);

    void DefaultCgbIoValues();

    // Memory
    std::array<std::array<uint8_t, 0x2000>, 2> VRAM_;  // $8000 ... $9FFF
    std::array<std::array<uint8_t, 0x1000>, 8> WRAM_;  // $C000 ... $DFFF
    std::array<uint8_t, 0xA0> OAM_;  // $FE00 ... $FE9F
    std::array<uint8_t, 0x7F> HRAM_;  // $FF80 ... $FFFE
    std::array<uint8_t, 0x40> BG_CRAM_;  // Background palette data accessed through BCPS/BCPD
    std::array<uint8_t, 0x40> OBJ_CRAM_;  // OBJ palette data accessed through OCPS/OCPD
    std::array<uint8_t, 0x900> BOOT_ROM;  // Boot ROM
    std::array<uint8_t, FRAME_BUFFER_SIZE>& frameBuffer_;

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
    uint8_t divCounter_;
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
    uint8_t oamIndexDest_;

    // VRAM DMA
    uint16_t vramDmaSrcAddr_;
    uint16_t vramDmaDestAddr_;
    bool gdmaInProgress_;
    bool hdmaInProgress_;
    uint16_t gdmaBytesRemaining_;
    uint8_t hdmaBytesRemaining_;
    uint8_t hdmaBlocksRemaining_;
    uint8_t hdmaLy_;

    // Interrupts
    uint8_t lastPendingInterrupt_;
    bool prevStatState_;

    // Components
    CPU cpu_;
    PPU ppu_;
    std::unique_ptr<Cartridge> cartridge_;

    // Controller
    std::function<Buttons()> GetKeys;
};
