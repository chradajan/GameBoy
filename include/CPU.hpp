#pragma once

#include <CPU_Registers.hpp>
#include <cstdint>
#include <fstream>
#include <functional>
#include <optional>
#include <sstream>
#include <utility>

class CPU
{
public:
    // CPU requires a Read/Write function in order to operate, so delete default constructors.
    CPU() = delete;
    CPU(CPU const&) = delete;
    CPU& operator=(CPU const&) = delete;
    CPU(CPU&&) = delete;
    CPU& operator=(CPU&&) = delete;

    /// @brief Create an instance of a Sharp SM83 CPU.
    /// @param readFunction Function wrapper to handle reads.
    /// @param writeFunction Function wrapper to handle writes.
    CPU(std::function<uint8_t(uint16_t)> readFunction,
        std::function<void(uint16_t, uint8_t)> writeFunction,
        std::function<void()> acknowledgeInterruptFunction,
        std::function<std::pair<bool, bool>(bool)> stopFunction);

    /// @brief Default CPU destructor.
    ~CPU() = default;

    /// @brief Run the CPU for 1 M-cycle.
    /// @param interruptInfo If an interrupt is pending, provide the interrupt address to jump to and how many interrupts in total
    ///                      are pending.
    void Clock(std::optional<std::pair<uint16_t, uint8_t>> interruptInfo);

    /// @brief Use to force exit halt mode.
    void ExitHalt() { halted_ = false; }

    /// @brief Reset the state of the CPU to as if it just started.
    /// @param bootRom Used to decided where to set PC.
    void Reset(bool bootRom);

private:
    /// @brief Wrapper for Read function.
    std::function<uint8_t(uint16_t)> Read;

    /// @brief Wrapper for Write function.
    std::function<void(uint16_t, uint8_t)> Write;

    /// @brief Function to call when servicing an interrupt.
    std::function<void()> AcknowledgeInterrupt;

    /// @brief Wrapper for function to call when executing Stop command.
    std::function<std::pair<bool, bool>(bool)> ReportStop;

    /// @brief Read the PC, then increment it.
    /// @return The value pointed to by the current PC address.
    uint8_t ReadPC();

    /// @brief Pop the top byte off the stack.
    /// @return The byte at the top of the stack.
    uint8_t Pop();

    /// @brief Push a byte onto the stack.
    /// @param data Byte to push onto stack.
    void Push(uint8_t data);

    /// @brief Determine which function to execute based on current OpCode.
    void DecodeOpCode();

    /// @brief Save the current PC and then set it to the correct interrupt address.
    /// @param addr New address to set PC to.
    void InterruptHandler(uint16_t addr);

    /// @brief Open a log file stream and prepare to start logging to it.
    void InitializeLogging();

    /// @brief Write current logging data to log file.
    void AddToLog();

    // 8-bit loads
    void LoadImmediateToReg(uint8_t* destReg);
    void LoadMemToReg(uint8_t* destReg, uint16_t srcAddr);
    void LoadAbsoluteMemToReg(uint8_t* destReg);

    void LoadImmediateToMem(uint16_t destAddr);
    void LoadRegToMem(uint16_t destAddr, uint8_t srcReg);
    void LoadRegToAbsoluteMem(uint8_t srcReg);

    void LoadMemToRegPostfix(bool increment);
    void LoadRegToMemPostfix(bool increment);

    void LoadLastPageToReg();
    void LoadRegToLastPage();

    // 16-bit loads
    void LoadImmediate16ToReg(uint16_t* destReg);
    void LoadHLToSP();
    void LoadSPnToHL();
    void LoadSPToAbsoluteMem();
    void PushReg16(uint16_t srcReg);
    void PopReg16(uint16_t* destReg, bool afPop);

    // 8-bit arithmetic
    void AddToReg(uint8_t* destReg, uint8_t operand, bool adc, bool inc);
    void AddMemToA(bool immediate, bool adc);

    void SubFromReg(uint8_t* destReg, uint8_t operand, bool sbc, bool cp, bool dec);
    void SubMemFromA(bool immediate, bool sbc, bool cp);

    void AndWithA(uint8_t operand);
    void AndMemWithA(bool immediate);

    void OrWithA(uint8_t operand);
    void OrMemWithA(bool immediate);

    void XorWithA(uint8_t operand);
    void XorMemWithA(bool immediate);

    void IncHL();
    void DecHL();

    // 16-bit arithmetic
    void AddRegToHL(uint16_t operand);
    void AddImmediateToSP();
    void IncDec16(uint16_t* destReg, int8_t operand);

    // Miscellaneous commands
    void SwapRegNibbles(uint8_t* destReg);
    void SwapMemNibbles();
    void DAA();
    void Halt();
    void Stop();
    void DI();
    void EI();

    // Rotates/shifts
    void RLC(uint8_t* reg, bool prefix);
    void RL(uint8_t* reg, bool prefix);
    void RRC(uint8_t* reg, bool prefix);
    void RR(uint8_t* reg, bool prefix);

    void RLCMem();
    void RLMem();
    void RRCMem();
    void RRMem();

    void SLA(uint8_t* reg);
    void SRA(uint8_t* reg);
    void SRL(uint8_t* reg);

    void SLAMem();
    void SRAMem();
    void SRLMem();

    // Bit operations
    void Bit(uint8_t reg, uint8_t bit);
    void BitMem(uint8_t bit);

    void Set(uint8_t* destReg, uint8_t bit);
    void SetMem(uint8_t bit);

    void Res(uint8_t* destReg, uint8_t bit);
    void ResMem(uint8_t bit);

    // Jumps/calls
    void JumpToAbsolute(bool condition);
    void JumpToRelative(bool condition);
    void Call(bool condition);
    void Restart(uint8_t addr);
    void Return(bool enableInterrupts);
    void ReturnConditional(bool condition);

    // State variables
    CPU_Registers reg_;
    uint8_t opCode_;
    uint8_t mCycle_;
    bool prefixedOpCode_;
    std::function<void()> instruction_;
    uint8_t cmdData8_;
    uint16_t cmdData16_;

    // Interrupt variables
    bool interruptsEnabled_;
    bool setInterruptsEnabled_;
    bool setInterruptsDisabled_;
    bool interruptBeingProcessed_;
    uint8_t interruptCountdown_;

    // Halt state variables
    bool halted_;
    bool haltBug_;
    uint8_t numPendingInterrupts_;

    // Logging variables
    std::stringstream opCodeStream_;
    std::stringstream regStream_;
    std::stringstream mnemonic_;
    std::ofstream log_;
    uint64_t mCyclesTotal_;
    uint64_t cyclesToLog_;
    bool haltLogged_;
};
