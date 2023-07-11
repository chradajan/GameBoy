#pragma once

#include <Cartridge/Cartridge.hpp>
#include <Cartridge/MBC0.hpp>
#include <Cartridge/MBC1.hpp>
#include <Controller.hpp>
#include <CPU.hpp>
#include <PPU.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <utility>

class GameBoy
{
public:
    /// @brief GameBoy constructor. Handles creation of all components.
    /// @param frameBuffer Pointer to buffer to store pixel data.
    GameBoy(uint8_t* frameBuffer);

    /// @brief Infinitely run the GameBoy. Immediately returns if no cartridge is loaded.
    void Run();

    /// @brief Reset the GameBoy to its power-on state.
    void Reset();

    /// @brief Load cartridge data from gb ROM.
    /// @param romPath Path to gb ROM file.
    void InsertCartridge(fs::path romPath);

    bool FrameReady();

    void SetInputs(Buttons const& buttons);

private:
    /// @brief Clock the timer components.
    void ClockTimer();

    /// @brief Run one cycle of an OAM DMA transfer.
    void ClockOamDma();

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

    void IoWriteTAC(uint8_t data);
    void IoWriteDMA(uint8_t data);
    void IoWriteBCPD(uint8_t data);
    void IoWriteOCPD(uint8_t data);

    // Memory
    std::array<std::array<uint8_t, 0x2000>, 2> VRAM_;  // $8000 ... $9FFF
    std::array<std::array<uint8_t, 0x1000>, 8> WRAM_;  // $C000 ... $DFFF
    std::array<uint8_t, 0xA0> OAM_;  // $FE00 ... $FE9F
    std::array<uint8_t, 0x7F> HRAM_;  // $FF80 ... $FFFE
    std::array<uint8_t, 0x40> BG_CRAM_;  // Background palette data accessed through BCPS/BCPD
    std::array<uint8_t, 0x40> OBJ_CRAM_;  // OBJ palette data accessed through OCPS/OCPD

    // I/O Registers
    std::array<uint8_t, 0x78> ioReg_;
    uint8_t IE_;

    // Mode
    bool cgbMode_;

    // Serial transfer
    uint8_t serialOutData_;
    uint8_t serialBitsSent_;
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
