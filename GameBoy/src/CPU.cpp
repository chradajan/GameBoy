#include <CPU.hpp>
#include <cstdint>
#include <fstream>
#include <functional>
#include <optional>
#include <string>
#include <utility>

CPU::CPU(std::function<uint8_t(uint16_t)> readFunction,
         std::function<void(uint16_t, uint8_t)> writeFunction,
         std::function<void()> acknowledgeInterruptFunction,
         std::function<std::pair<bool, bool>(bool)> stopFunction) :
    Read(readFunction), Write(writeFunction), AcknowledgeInterrupt(acknowledgeInterruptFunction), ReportStop(stopFunction)
{
}

void CPU::PowerOn(bool const skipBootRom)
{
    reg_.Reset();
    opCode_ = 0x00;
    mCycle_ = 0x00;
    prefixedOpCode_ = false;
    instruction_ = [](){};
    cmdData8_ = 0x00;
    cmdData16_ = 0x00;

    interruptsEnabled_ = false;
    setInterruptsEnabled_ = false;
    setInterruptsDisabled_ = false;
    interruptBeingProcessed_ = false;
    interruptCountdown_ = 0x00;

    halted_ = false;
    haltBug_ = false;
    numPendingInterrupts_ = 0x00;

    if (!skipBootRom)
    {
        reg_.PC = 0x0000;
    }
}

void CPU::Clock(std::optional<std::pair<uint16_t, uint8_t>> interruptInfo)
{
    uint16_t interruptAddr = 0x0000;
    numPendingInterrupts_ = 0;

    if (interruptInfo)
    {
        interruptAddr = interruptInfo.value().first;
        numPendingInterrupts_ = interruptInfo.value().second;
    }

    if (mCycle_ == 0)
    {
        if (setInterruptsEnabled_ || setInterruptsDisabled_)
        {
            --interruptCountdown_;

            if (interruptCountdown_ == 0)
            {
                if (setInterruptsEnabled_)
                {
                    setInterruptsEnabled_ = false;
                    interruptsEnabled_ = true;
                }
                else
                {
                    setInterruptsDisabled_ = false;
                    interruptsEnabled_ = false;
                }
            }
        }

        if (interruptInfo)
        {
            if (interruptsEnabled_)
            {
                AcknowledgeInterrupt();
                --numPendingInterrupts_;
                instruction_ = std::bind(&CPU::InterruptHandler, this, interruptAddr);
                interruptsEnabled_ = false;
                interruptBeingProcessed_ = true;
            }

            halted_ = false;
        }
    }

    if (halted_)
    {
        return;
    }

    ++mCycle_;

    if (!interruptBeingProcessed_ && (prefixedOpCode_ || (mCycle_ == 1)))
    {
        DecodeOpCode();
    }
    else
    {
        instruction_();
    }

    return;
}

bool CPU::IsSerializable() const
{
    return mCycle_ == 0;
}

void CPU::Serialize(std::ofstream& out)
{
    out.write(reinterpret_cast<char*>(&interruptsEnabled_), sizeof(interruptsEnabled_));
    out.write(reinterpret_cast<char*>(&setInterruptsEnabled_), sizeof(setInterruptsEnabled_));
    out.write(reinterpret_cast<char*>(&setInterruptsDisabled_), sizeof(setInterruptsDisabled_));
    out.write(reinterpret_cast<char*>(&interruptCountdown_), sizeof(interruptCountdown_));
    out.write(reinterpret_cast<char*>(&halted_), sizeof(halted_));
    out.write(reinterpret_cast<char*>(&haltBug_), sizeof(haltBug_));
    reg_.Serialize(out);
}

void CPU::Deserialize(std::ifstream& in)
{
    in.read(reinterpret_cast<char*>(&interruptsEnabled_), sizeof(interruptsEnabled_));
    in.read(reinterpret_cast<char*>(&setInterruptsEnabled_), sizeof(setInterruptsEnabled_));
    in.read(reinterpret_cast<char*>(&setInterruptsDisabled_), sizeof(setInterruptsDisabled_));
    in.read(reinterpret_cast<char*>(&interruptCountdown_), sizeof(interruptCountdown_));
    in.read(reinterpret_cast<char*>(&halted_), sizeof(halted_));
    in.read(reinterpret_cast<char*>(&haltBug_), sizeof(haltBug_));
    reg_.Deserialize(in);
}

uint8_t CPU::ReadPC()
{
    return Read(reg_.PC++);
}

uint8_t CPU::Pop()
{
    uint8_t poppedByte = Read(reg_.SP);
    ++reg_.SP;
    return poppedByte;
}

void CPU::Push(uint8_t data)
{
    --reg_.SP;
    Write(reg_.SP, data);
}

void CPU::DecodeOpCode()
{
    if (haltBug_)
    {
        opCode_ = Read(reg_.PC);
        haltBug_ = false;
    }
    else
    {
        opCode_ = ReadPC();
    }

    if (!prefixedOpCode_ && (opCode_ == 0xCB))
    {
        prefixedOpCode_ = true;
        return;
    }

    if (prefixedOpCode_)
    {
        switch (opCode_)
        {
            // SWAP n
            case 0x37:
                SwapRegNibbles(&reg_.A);
                break;
            case 0x30:
                SwapRegNibbles(&reg_.B);
                break;
            case 0x31:
                SwapRegNibbles(&reg_.C);
                break;
            case 0x32:
                SwapRegNibbles(&reg_.D);
                break;
            case 0x33:
                SwapRegNibbles(&reg_.E);
                break;
            case 0x34:
                SwapRegNibbles(&reg_.H);
                break;
            case 0x35:
                SwapRegNibbles(&reg_.L);
                break;
            case 0x36:
                instruction_ = std::bind(&CPU::SwapMemNibbles, this);
                break;

            // RLC n
            case 0x07:
                RLC(&reg_.A, true);
                break;
            case 0x00:
                RLC(&reg_.B, true);
                break;
            case 0x01:
                RLC(&reg_.C, true);
                break;
            case 0x02:
                RLC(&reg_.D, true);
                break;
            case 0x03:
                RLC(&reg_.E, true);
                break;
            case 0x04:
                RLC(&reg_.H, true);
                break;
            case 0x05:
                RLC(&reg_.L, true);
                break;
            case 0x06:
                instruction_ = std::bind(&CPU::RLCMem, this);
                break;

            // RL n
            case 0x17:
                RL(&reg_.A, true);
                break;
            case 0x10:
                RL(&reg_.B, true);
                break;
            case 0x11:
                RL(&reg_.C, true);
                break;
            case 0x12:
                RL(&reg_.D, true);
                break;
            case 0x13:
                RL(&reg_.E, true);
                break;
            case 0x14:
                RL(&reg_.H, true);
                break;
            case 0x15:
                RL(&reg_.L, true);
                break;
            case 0x16:
                instruction_ = std::bind(&CPU::RLMem, this);
                break;

            // RRC n
            case 0x0F:
                RRC(&reg_.A, true);
                break;
            case 0x08:
                RRC(&reg_.B, true);
                break;
            case 0x09:
                RRC(&reg_.C, true);
                break;
            case 0x0A:
                RRC(&reg_.D, true);
                break;
            case 0x0B:
                RRC(&reg_.E, true);
                break;
            case 0x0C:
                RRC(&reg_.H, true);
                break;
            case 0x0D:
                RRC(&reg_.L, true);
                break;
            case 0x0E:
                instruction_ = std::bind(&CPU::RRCMem, this);
                break;

            // RR n
            case 0x1F:
                RR(&reg_.A, true);
                break;
            case 0x18:
                RR(&reg_.B, true);
                break;
            case 0x19:
                RR(&reg_.C, true);
                break;
            case 0x1A:
                RR(&reg_.D, true);
                break;
            case 0x1B:
                RR(&reg_.E, true);
                break;
            case 0x1C:
                RR(&reg_.H, true);
                break;
            case 0x1D:
                RR(&reg_.L, true);
                break;
            case 0x1E:
                instruction_ = std::bind(&CPU::RRMem, this);
                break;

            // SLA n
            case 0x27:
                SLA(&reg_.A);
                break;
            case 0x20:
                SLA(&reg_.B);
                break;
            case 0x21:
                SLA(&reg_.C);
                break;
            case 0x22:
                SLA(&reg_.D);
                break;
            case 0x23:
                SLA(&reg_.E);
                break;
            case 0x24:
                SLA(&reg_.H);
                break;
            case 0x25:
                SLA(&reg_.L);
                break;
            case 0x26:
                instruction_ = std::bind(&CPU::SLAMem, this);
                break;

            // SRA n
            case 0x2F:
                SRA(&reg_.A);
                break;
            case 0x28:
                SRA(&reg_.B);
                break;
            case 0x29:
                SRA(&reg_.C);
                break;
            case 0x2A:
                SRA(&reg_.D);
                break;
            case 0x2B:
                SRA(&reg_.E);
                break;
            case 0x2C:
                SRA(&reg_.H);
                break;
            case 0x2D:
                SRA(&reg_.L);
                break;
            case 0x2E:
                instruction_ = std::bind(&CPU::SRAMem, this);
                break;

            // SRL n
            case 0x3F:
                SRL(&reg_.A);
                break;
            case 0x38:
                SRL(&reg_.B);
                break;
            case 0x39:
                SRL(&reg_.C);
                break;
            case 0x3A:
                SRL(&reg_.D);
                break;
            case 0x3B:
                SRL(&reg_.E);
                break;
            case 0x3C:
                SRL(&reg_.H);
                break;
            case 0x3D:
                SRL(&reg_.L);
                break;
            case 0x3E:
                instruction_ = std::bind(&CPU::SRLMem, this);
                break;

            // BIT n, r
            case 0x40 ... 0x7F:
            {
                uint8_t regIndex = (opCode_ & 0x07);
                uint8_t bit = (opCode_ & 0x38) >> 3;

                switch (regIndex)
                {
                    case 0:
                        Bit(reg_.B, bit);
                        break;
                    case 1:
                        Bit(reg_.C, bit);
                        break;
                    case 2:
                        Bit(reg_.D, bit);
                        break;
                    case 3:
                        Bit(reg_.E, bit);
                        break;
                    case 4:
                        Bit(reg_.H, bit);
                        break;
                    case 5:
                        Bit(reg_.L, bit);
                        break;
                    case 6:
                        instruction_ = std::bind(&CPU::BitMem, this, bit);
                        break;
                    case 7:
                        Bit(reg_.A, bit);
                        break;
                }
                break;
            }

            // SET b, r
            case 0xC0 ... 0xFF:
            {
                uint8_t regIndex = (opCode_ & 0x07);
                uint8_t bit = (opCode_ & 0x38) >> 3;

                switch (regIndex)
                {
                    case 0:
                        Set(&reg_.B, bit);
                        break;
                    case 1:
                        Set(&reg_.C, bit);
                        break;
                    case 2:
                        Set(&reg_.D, bit);
                        break;
                    case 3:
                        Set(&reg_.E, bit);
                        break;
                    case 4:
                        Set(&reg_.H, bit);
                        break;
                    case 5:
                        Set(&reg_.L, bit);
                        break;
                    case 6:
                        instruction_ = std::bind(&CPU::SetMem, this, bit);
                        break;
                    case 7:
                        Set(&reg_.A, bit);
                        break;
                }
                break;
            }

            // RES b, r
            case 0x80 ... 0xBF:
            {
                uint8_t regIndex = (opCode_ & 0x07);
                uint8_t bit = (opCode_ & 0x38) >> 3;

                switch (regIndex)
                {
                    case 0:
                        Res(&reg_.B, bit);
                        break;
                    case 1:
                        Res(&reg_.C, bit);
                        break;
                    case 2:
                        Res(&reg_.D, bit);
                        break;
                    case 3:
                        Res(&reg_.E, bit);
                        break;
                    case 4:
                        Res(&reg_.H, bit);
                        break;
                    case 5:
                        Res(&reg_.L, bit);
                        break;
                    case 6:
                        instruction_ = std::bind(&CPU::ResMem, this, bit);
                        break;
                    case 7:
                        Res(&reg_.A, bit);
                        break;
                }
                break;
            }
        }
    }
    else
    {
        switch (opCode_)
        {
            // LD r, n
            case 0x3E:
                instruction_ = std::bind(&CPU::LoadImmediateToReg, this, &reg_.A);
                break;
            case 0x06:
                instruction_ = std::bind(&CPU::LoadImmediateToReg, this, &reg_.B);
                break;
            case 0x0E:
                instruction_ = std::bind(&CPU::LoadImmediateToReg, this, &reg_.C);
                break;
            case 0x16:
                instruction_ = std::bind(&CPU::LoadImmediateToReg, this, &reg_.D);
                break;
            case 0x1E:
                instruction_ = std::bind(&CPU::LoadImmediateToReg, this, &reg_.E);
                break;
            case 0x26:
                instruction_ = std::bind(&CPU::LoadImmediateToReg, this, &reg_.H);
                break;
            case 0x2E:
                instruction_ = std::bind(&CPU::LoadImmediateToReg, this, &reg_.L);
                break;

            // LD r, r
            case 0x7F:
                mCycle_ = 0;
                break;
            case 0x78:
                reg_.A = reg_.B;
                mCycle_ = 0;
                break;
            case 0x79:
                reg_.A = reg_.C;
                mCycle_ = 0;
                break;
            case 0x7A:
                reg_.A = reg_.D;
                mCycle_ = 0;
                break;
            case 0x7B:
                reg_.A = reg_.E;
                mCycle_ = 0;
                break;
            case 0x7C:
                reg_.A = reg_.H;
                mCycle_ = 0;
                break;
            case 0x7D:
                reg_.A = reg_.L;
                mCycle_ = 0;
                break;
            case 0x47:
                reg_.B = reg_.A;
                mCycle_ = 0;
                break;
            case 0x40:
                mCycle_ = 0;
                break;
            case 0x41:
                reg_.B = reg_.C;
                mCycle_ = 0;
                break;
            case 0x42:
                reg_.B = reg_.D;
                mCycle_ = 0;
                break;
            case 0x43:
                reg_.B = reg_.E;
                mCycle_ = 0;
                break;
            case 0x44:
                reg_.B = reg_.H;
                mCycle_ = 0;
                break;
            case 0x45:
                reg_.B = reg_.L;
                mCycle_ = 0;
                break;
            case 0x4F:
                reg_.C = reg_.A;
                mCycle_ = 0;
                break;
            case 0x48:
                reg_.C = reg_.B;
                mCycle_ = 0;
                break;
            case 0x49:
                mCycle_ = 0;
                break;
            case 0x4A:
                reg_.C = reg_.D;
                mCycle_ = 0;
                break;
            case 0x4B:
                reg_.C = reg_.E;
                mCycle_ = 0;
                break;
            case 0x4C:
                reg_.C = reg_.H;
                mCycle_ = 0;
                break;
            case 0x4D:
                reg_.C = reg_.L;
                mCycle_ = 0;
                break;
            case 0x57:
                reg_.D = reg_.A;
                mCycle_ = 0;
                break;
            case 0x50:
                reg_.D = reg_.B;
                mCycle_ = 0;
                break;
            case 0x51:
                reg_.D = reg_.C;
                mCycle_ = 0;
                break;
            case 0x52:
                mCycle_ = 0;
                break;
            case 0x53:
                reg_.D = reg_.E;
                mCycle_ = 0;
                break;
            case 0x54:
                reg_.D = reg_.H;
                mCycle_ = 0;
                break;
            case 0x55:
                reg_.D = reg_.L;
                mCycle_ = 0;
                break;
            case 0x5F:
                reg_.E = reg_.A;
                mCycle_ = 0;
                break;
            case 0x58:
                reg_.E = reg_.B;
                mCycle_ = 0;
                break;
            case 0x59:
                reg_.E = reg_.C;
                mCycle_ = 0;
                break;
            case 0x5A:
                reg_.E = reg_.D;
                mCycle_ = 0;
                break;
            case 0x5B:
                mCycle_ = 0;
                break;
            case 0x5C:
                reg_.E = reg_.H;
                mCycle_ = 0;
                break;
            case 0x5D:
                reg_.E = reg_.L;
                mCycle_ = 0;
                break;
            case 0x67:
                reg_.H = reg_.A;
                mCycle_ = 0;
                break;
            case 0x60:
                reg_.H = reg_.B;
                mCycle_ = 0;
                break;
            case 0x61:
                reg_.H = reg_.C;
                mCycle_ = 0;
                break;
            case 0x62:
                reg_.H = reg_.D;
                mCycle_ = 0;
                break;
            case 0x63:
                reg_.H = reg_.E;
                mCycle_ = 0;
                break;
            case 0x64:
                mCycle_ = 0;
                break;
            case 0x65:
                reg_.H = reg_.L;
                mCycle_ = 0;
                break;
            case 0x6F:
                reg_.L = reg_.A;
                mCycle_ = 0;
                break;
            case 0x68:
                reg_.L = reg_.B;
                mCycle_ = 0;
                break;
            case 0x69:
                reg_.L = reg_.C;
                mCycle_ = 0;
                break;
            case 0x6A:
                reg_.L = reg_.D;
                mCycle_ = 0;
                break;
            case 0x6B:
                reg_.L = reg_.E;
                mCycle_ = 0;
                break;
            case 0x6C:
                reg_.L = reg_.H;
                mCycle_ = 0;
                break;
            case 0x6D:
                mCycle_ = 0;
                break;

            // LD (nn), r
            case 0x02:
                instruction_ = std::bind(&CPU::LoadRegToMem, this, reg_.BC, reg_.A);
                break;
            case 0x12:
                instruction_ = std::bind(&CPU::LoadRegToMem, this, reg_.DE, reg_.A);
                break;
            case 0x77:
                instruction_ = std::bind(&CPU::LoadRegToMem, this, reg_.HL, reg_.A);
                break;
            case 0xEA:
                instruction_ = std::bind(&CPU::LoadRegToAbsoluteMem, this, reg_.A);
                break;
            case 0x70:
                instruction_ = std::bind(&CPU::LoadRegToMem, this, reg_.HL, reg_.B);
                break;
            case 0x71:
                instruction_ = std::bind(&CPU::LoadRegToMem, this, reg_.HL, reg_.C);
                break;
            case 0x72:
                instruction_ = std::bind(&CPU::LoadRegToMem, this, reg_.HL, reg_.D);
                break;
            case 0x73:
                instruction_ = std::bind(&CPU::LoadRegToMem, this, reg_.HL, reg_.E);
                break;
            case 0x74:
                instruction_ = std::bind(&CPU::LoadRegToMem, this, reg_.HL, reg_.H);
                break;
            case 0x75:
                instruction_ = std::bind(&CPU::LoadRegToMem, this, reg_.HL, reg_.L);
                break;
            case 0x36:
                instruction_ = std::bind(&CPU::LoadImmediateToMem, this, reg_.HL);
                break;

            // LD r, (nn)
            case 0x7E:
                instruction_ = std::bind(&CPU::LoadMemToReg, this, &reg_.A, reg_.HL);
                break;
            case 0x0A:
                instruction_ = std::bind(&CPU::LoadMemToReg, this, &reg_.A, reg_.BC);
                break;
            case 0x1A:
                instruction_ = std::bind(&CPU::LoadMemToReg, this, &reg_.A, reg_.DE);
                break;
            case 0xFA:
                instruction_ = std::bind(&CPU::LoadAbsoluteMemToReg, this, &reg_.A);
                break;
            case 0x46:
                instruction_ = std::bind(&CPU::LoadMemToReg, this, &reg_.B, reg_.HL);
                break;
            case 0x4E:
                instruction_ = std::bind(&CPU::LoadMemToReg, this, &reg_.C, reg_.HL);
                break;
            case 0x56:
                instruction_ = std::bind(&CPU::LoadMemToReg, this, &reg_.D, reg_.HL);
                break;
            case 0x5E:
                instruction_ = std::bind(&CPU::LoadMemToReg, this, &reg_.E, reg_.HL);
                break;
            case 0x66:
                instruction_ = std::bind(&CPU::LoadMemToReg, this, &reg_.H, reg_.HL);
                break;
            case 0x6E:
                instruction_ = std::bind(&CPU::LoadMemToReg, this, &reg_.L, reg_.HL);
                break;

            // LD A, ($FF00+C)
            case 0xF2:
                instruction_ = std::bind(&CPU::LoadMemToReg, this, &reg_.A, 0xFF00 + reg_.C);
                break;

            // LD ($FF00+C), A
            case 0xE2:
                instruction_ = std::bind(&CPU::LoadRegToMem, this, 0xFF00 + reg_.C, reg_.A);
                break;

            // LD A, ($FF00+n)
            case 0xF0:
                instruction_ = std::bind(&CPU::LoadLastPageToReg, this);
                break;

            // LD ($FF00+n), A
            case 0xE0:
                instruction_ = std::bind(&CPU::LoadRegToLastPage, this);
                break;

            // LDD A, (HL)
            case 0x3A:
                instruction_ = std::bind(&CPU::LoadMemToRegPostfix, this, false);
                break;

            // LDD (HL), A
            case 0x32:
                instruction_ = std::bind(&CPU::LoadRegToMemPostfix, this, false);
                break;

            // LDI A, (HL)
            case 0x2A:
                instruction_ = std::bind(&CPU::LoadMemToRegPostfix, this, true);
                break;

            // LDI (HL), A
            case 0x22:
                instruction_ = std::bind(&CPU::LoadRegToMemPostfix, this, true);
                break;

            // LD rr, nn
            case 0x01:
                instruction_ = std::bind(&CPU::LoadImmediate16ToReg, this, &reg_.BC);
                break;
            case 0x11:
                instruction_ = std::bind(&CPU::LoadImmediate16ToReg, this, &reg_.DE);
                break;
            case 0x21:
                instruction_ = std::bind(&CPU::LoadImmediate16ToReg, this, &reg_.HL);
                break;
            case 0x31:
                instruction_ = std::bind(&CPU::LoadImmediate16ToReg, this, &reg_.SP);
                break;

            // LD SP, HL
            case 0xF9:
                instruction_ = std::bind(&CPU::LoadHLToSP, this);
                break;

            // LD HL, SP+n
            case 0xF8:
                instruction_ = std::bind(&CPU::LoadSPnToHL, this);
                break;

            // LD (nn), SP
            case 0x08:
                instruction_ = std::bind(&CPU::LoadSPToAbsoluteMem, this);
                break;

            // PUSH rr
            case 0xF5:
                instruction_ = std::bind(&CPU::PushReg16, this, reg_.AF);
                break;
            case 0xC5:
                instruction_ = std::bind(&CPU::PushReg16, this, reg_.BC);
                break;
            case 0xD5:
                instruction_ = std::bind(&CPU::PushReg16, this, reg_.DE);
                break;
            case 0xE5:
                instruction_ = std::bind(&CPU::PushReg16, this, reg_.HL);
                break;

            // POP rr
            case 0xF1:
                instruction_ = std::bind(&CPU::PopReg16, this, &reg_.AF, true);
                break;
            case 0xC1:
                instruction_ = std::bind(&CPU::PopReg16, this, &reg_.BC, false);
                break;
            case 0xD1:
                instruction_ = std::bind(&CPU::PopReg16, this, &reg_.DE, false);
                break;
            case 0xE1:
                instruction_ = std::bind(&CPU::PopReg16, this, &reg_.HL, false);
                break;

            // ADD A, n
            case 0x87:
                AddToReg(&reg_.A, reg_.A, false, false);
                break;
            case 0x80:
                AddToReg(&reg_.A, reg_.B, false, false);
                break;
            case 0x81:
                AddToReg(&reg_.A, reg_.C, false, false);
                break;
            case 0x82:
                AddToReg(&reg_.A, reg_.D, false, false);
                break;
            case 0x83:
                AddToReg(&reg_.A, reg_.E, false, false);
                break;
            case 0x84:
                AddToReg(&reg_.A, reg_.H, false, false);
                break;
            case 0x85:
                AddToReg(&reg_.A, reg_.L, false, false);
                break;
            case 0x86:
                instruction_ = std::bind(&CPU::AddMemToA, this, false, false);
                break;
            case 0xC6:
                instruction_ = std::bind(&CPU::AddMemToA, this, true, false);
                break;

            // ADC A, n
            case 0x8F:
                AddToReg(&reg_.A, reg_.A, true, false);
                break;
            case 0x88:
                AddToReg(&reg_.A, reg_.B, true, false);
                break;
            case 0x89:
                AddToReg(&reg_.A, reg_.C, true, false);
                break;
            case 0x8A:
                AddToReg(&reg_.A, reg_.D, true, false);
                break;
            case 0x8B:
                AddToReg(&reg_.A, reg_.E, true, false);
                break;
            case 0x8C:
                AddToReg(&reg_.A, reg_.H, true, false);
                break;
            case 0x8D:
                AddToReg(&reg_.A, reg_.L, true, false);
                break;
            case 0x8E:
                instruction_ = std::bind(&CPU::AddMemToA, this, false, true);
                break;
            case 0xCE:
                instruction_ = std::bind(&CPU::AddMemToA, this, true, true);
                break;

            // SUB A, n
            case 0x97:
                SubFromReg(&reg_.A, reg_.A, false, false, false);
                break;
            case 0x90:
                SubFromReg(&reg_.A, reg_.B, false, false, false);
                break;
            case 0x91:
                SubFromReg(&reg_.A, reg_.C, false, false, false);
                break;
            case 0x92:
                SubFromReg(&reg_.A, reg_.D, false, false, false);
                break;
            case 0x93:
                SubFromReg(&reg_.A, reg_.E, false, false, false);
                break;
            case 0x94:
                SubFromReg(&reg_.A, reg_.H, false, false, false);
                break;
            case 0x95:
                SubFromReg(&reg_.A, reg_.L, false, false, false);
                break;
            case 0x96:
                instruction_ = std::bind(&CPU::SubMemFromA, this, false, false, false);
                break;
            case 0xD6:
                instruction_ = std::bind(&CPU::SubMemFromA, this, true, false, false);
                break;

            // SBC A, n
            case 0x9F:
                SubFromReg(&reg_.A, reg_.A, true, false, false);
                break;
            case 0x98:
                SubFromReg(&reg_.A, reg_.B, true, false, false);
                break;
            case 0x99:
                SubFromReg(&reg_.A, reg_.C, true, false, false);
                break;
            case 0x9A:
                SubFromReg(&reg_.A, reg_.D, true, false, false);
                break;
            case 0x9B:
                SubFromReg(&reg_.A, reg_.E, true, false, false);
                break;
            case 0x9C:
                SubFromReg(&reg_.A, reg_.H, true, false, false);
                break;
            case 0x9D:
                SubFromReg(&reg_.A, reg_.L, true, false, false);
                break;
            case 0x9E:
                instruction_ = std::bind(&CPU::SubMemFromA, this, false, true, false);
                break;
            case 0xDE:
                instruction_ = std::bind(&CPU::SubMemFromA, this, true, true, false);
                break;

            // AND A, n
            case 0xA7:
                AndWithA(reg_.A);
                break;
            case 0xA0:
                AndWithA(reg_.B);
                break;
            case 0xA1:
                AndWithA(reg_.C);
                break;
            case 0xA2:
                AndWithA(reg_.D);
                break;
            case 0xA3:
                AndWithA(reg_.E);
                break;
            case 0xA4:
                AndWithA(reg_.H);
                break;
            case 0xA5:
                AndWithA(reg_.L);
                break;
            case 0xA6:
                instruction_ = std::bind(&CPU::AndMemWithA, this, false);
                break;
            case 0xE6:
                instruction_ = std::bind(&CPU::AndMemWithA, this, true);
                break;

            // OR A, n
            case 0xB7:
                OrWithA(reg_.A);
                break;
            case 0xB0:
                OrWithA(reg_.B);
                break;
            case 0xB1:
                OrWithA(reg_.C);
                break;
            case 0xB2:
                OrWithA(reg_.D);
                break;
            case 0xB3:
                OrWithA(reg_.E);
                break;
            case 0xB4:
                OrWithA(reg_.H);
                break;
            case 0xB5:
                OrWithA(reg_.L);
                break;
            case 0xB6:
                instruction_ = std::bind(&CPU::OrMemWithA, this, false);
                break;
            case 0xF6:
                instruction_ = std::bind(&CPU::OrMemWithA, this, true);
                break;

            // XOR A, n
            case 0xAF:
                XorWithA(reg_.A);
                break;
            case 0xA8:
                XorWithA(reg_.B);
                break;
            case 0xA9:
                XorWithA(reg_.C);
                break;
            case 0xAA:
                XorWithA(reg_.D);
                break;
            case 0xAB:
                XorWithA(reg_.E);
                break;
            case 0xAC:
                XorWithA(reg_.H);
                break;
            case 0xAD:
                XorWithA(reg_.L);
                break;
            case 0xAE:
                instruction_ = std::bind(&CPU::XorMemWithA, this, false);
                break;
            case 0xEE:
                instruction_ = std::bind(&CPU::XorMemWithA, this, true);
                break;

            // CP n
            case 0xBF:
                SubFromReg(&reg_.A, reg_.A, false, true, false);
                break;
            case 0xB8:
                SubFromReg(&reg_.A, reg_.B, false, true, false);
                break;
            case 0xB9:
                SubFromReg(&reg_.A, reg_.C, false, true, false);
                break;
            case 0xBA:
                SubFromReg(&reg_.A, reg_.D, false, true, false);
                break;
            case 0xBB:
                SubFromReg(&reg_.A, reg_.E, false, true, false);
                break;
            case 0xBC:
                SubFromReg(&reg_.A, reg_.H, false, true, false);
                break;
            case 0xBD:
                SubFromReg(&reg_.A, reg_.L, false, true, false);
                break;
            case 0xBE:
                instruction_ = std::bind(&CPU::SubMemFromA, this, false, false, true);
                break;
            case 0xFE:
                instruction_ = std::bind(&CPU::SubMemFromA, this, true, false, true);
                break;

            // INC n
            case 0x3C:
                AddToReg(&reg_.A, 1, false, true);
                break;
            case 0x04:
                AddToReg(&reg_.B, 1, false, true);
                break;
            case 0x0C:
                AddToReg(&reg_.C, 1, false, true);
                break;
            case 0x14:
                AddToReg(&reg_.D, 1, false, true);
                break;
            case 0x1C:
                AddToReg(&reg_.E, 1, false, true);
                break;
            case 0x24:
                AddToReg(&reg_.H, 1, false, true);
                break;
            case 0x2C:
                AddToReg(&reg_.L, 1, false, true);
                break;
            case 0x34:
                instruction_ = std::bind(&CPU::IncHL, this);
                break;

            // DEC n
            case 0x3D:
                SubFromReg(&reg_.A, 1, false, false, true);
                break;
            case 0x05:
                SubFromReg(&reg_.B, 1, false, false, true);
                break;
            case 0x0D:
                SubFromReg(&reg_.C, 1, false, false, true);
                break;
            case 0x15:
                SubFromReg(&reg_.D, 1, false, false, true);
                break;
            case 0x1D:
                SubFromReg(&reg_.E, 1, false, false, true);
                break;
            case 0x25:
                SubFromReg(&reg_.H, 1, false, false, true);
                break;
            case 0x2D:
                SubFromReg(&reg_.L, 1, false, false, true);
                break;
            case 0x35:
                instruction_ = std::bind(&CPU::DecHL, this);
                break;

            // ADD HL, n
            case 0x09:
                instruction_ = std::bind(&CPU::AddRegToHL, this, reg_.BC);
                break;
            case 0x19:
                instruction_ = std::bind(&CPU::AddRegToHL, this, reg_.DE);
                break;
            case 0x29:
                instruction_ = std::bind(&CPU::AddRegToHL, this, reg_.HL);
                break;
            case 0x39:
                instruction_ = std::bind(&CPU::AddRegToHL, this, reg_.SP);
                break;

            // ADD SP, n
            case 0xE8:
                instruction_ = std::bind(&CPU::AddImmediateToSP, this);
                break;

            // INC nn
            case 0x03:
                instruction_ = std::bind(&CPU::IncDec16, this, &reg_.BC, 1);
                break;
            case 0x13:
                instruction_ = std::bind(&CPU::IncDec16, this, &reg_.DE, 1);
                break;
            case 0x23:
                instruction_ = std::bind(&CPU::IncDec16, this, &reg_.HL, 1);
                break;
            case 0x33:
                instruction_ = std::bind(&CPU::IncDec16, this, &reg_.SP, 1);
                break;

            // DEC nn
            case 0x0B:
                instruction_ = std::bind(&CPU::IncDec16, this, &reg_.BC, -1);
                break;
            case 0x1B:
                instruction_ = std::bind(&CPU::IncDec16, this, &reg_.DE, -1);
                break;
            case 0x2B:
                instruction_ = std::bind(&CPU::IncDec16, this, &reg_.HL, -1);
                break;
            case 0x3B:
                instruction_ = std::bind(&CPU::IncDec16, this, &reg_.SP, -1);
                break;

            // DAA
            case 0x27:
                DAA();
                break;

            // CPL
            case 0x2F:
                reg_.A = ~reg_.A;
                reg_.SetSubtractionFlag(true);
                reg_.SetHalfCarryFlag(true);
                mCycle_ = 0;
                break;

            // CCF
            case 0x3F:
                reg_.SetCarryFlag(!reg_.IsCarryFlagSet());
                reg_.SetSubtractionFlag(false);
                reg_.SetHalfCarryFlag(false);
                mCycle_ = 0;
                break;

            // SCF
            case 0x37:
                reg_.SetSubtractionFlag(false);
                reg_.SetHalfCarryFlag(false);
                reg_.SetCarryFlag(true);
                mCycle_ = 0;
                break;

            // NOP
            case 0x00:
                mCycle_ = 0;
                break;

            // HALT
            case 0x76:
                Halt();
                break;

            // STOP
            case 0x10:
                Stop();
                break;

            // DI
            case 0xF3:
                DI();
                break;

            // EI
            case 0xFB:
                EI();
                break;

            // RLCA
            case 0x07:
                RLC(&reg_.A, false);
                break;

            // RLA
            case 0x17:
                RL(&reg_.A, false);
                break;

            // RRCA
            case 0x0F:
                RRC(&reg_.A, false);
                break;

            // RRA
            case 0x1F:
                RR(&reg_.A, false);
                break;

            // JP nn
            case 0xC3:
                instruction_ = std::bind(&CPU::JumpToAbsolute, this, true);
                break;

            // JP cc, nn
            case 0xC2:
                instruction_ = std::bind(&CPU::JumpToAbsolute, this, !reg_.IsZeroFlagSet());
                break;
            case 0xCA:
                instruction_ = std::bind(&CPU::JumpToAbsolute, this, reg_.IsZeroFlagSet());
                break;
            case 0xD2:
                instruction_ = std::bind(&CPU::JumpToAbsolute, this, !reg_.IsCarryFlagSet());
                break;
            case 0xDA:
                instruction_ = std::bind(&CPU::JumpToAbsolute, this, reg_.IsCarryFlagSet());
                break;

            // JP HL
            case 0xE9:
                reg_.PC = reg_.HL;
                mCycle_ = 0;
                break;

            // JR n
            case 0x18:
                instruction_ = std::bind(&CPU::JumpToRelative, this, true);
                break;

            // JR cc,n
            case 0x20:
                instruction_ = std::bind(&CPU::JumpToRelative, this, !reg_.IsZeroFlagSet());
                break;
            case 0x28:
                instruction_ = std::bind(&CPU::JumpToRelative, this, reg_.IsZeroFlagSet());
                break;
            case 0x30:
                instruction_ = std::bind(&CPU::JumpToRelative, this, !reg_.IsCarryFlagSet());
                break;
            case 0x38:
                instruction_ = std::bind(&CPU::JumpToRelative, this, reg_.IsCarryFlagSet());
                break;

            // CALL nn
            case 0xCD:
                instruction_ = std::bind(&CPU::Call, this, true);
                break;

            // CALL cc,nn
            case 0xC4:
                instruction_ = std::bind(&CPU::Call, this, !reg_.IsZeroFlagSet());
                break;
            case 0xCC:
                instruction_ = std::bind(&CPU::Call, this, reg_.IsZeroFlagSet());
                break;
            case 0xD4:
                instruction_ = std::bind(&CPU::Call, this, !reg_.IsCarryFlagSet());
                break;
            case 0xDC:
                instruction_ = std::bind(&CPU::Call, this, reg_.IsCarryFlagSet());
                break;

            // RST n
            case 0xC7:
                instruction_ = std::bind(&CPU::Restart, this, 0x00);
                break;
            case 0xCF:
                instruction_ = std::bind(&CPU::Restart, this, 0x08);
                break;
            case 0xD7:
                instruction_ = std::bind(&CPU::Restart, this, 0x10);
                break;
            case 0xDF:
                instruction_ = std::bind(&CPU::Restart, this, 0x18);
                break;
            case 0xE7:
                instruction_ = std::bind(&CPU::Restart, this, 0x20);
                break;
            case 0xEF:
                instruction_ = std::bind(&CPU::Restart, this, 0x28);
                break;
            case 0xF7:
                instruction_ = std::bind(&CPU::Restart, this, 0x30);
                break;
            case 0xFF:
                instruction_ = std::bind(&CPU::Restart, this, 0x38);
                break;

            // RET
            case 0xC9:
                instruction_ = std::bind(&CPU::Return, this, false);
                break;

            // RET cc
            case 0xC0:
                instruction_ = std::bind(&CPU::ReturnConditional, this, !reg_.IsZeroFlagSet());
                break;
            case 0xC8:
                instruction_ = std::bind(&CPU::ReturnConditional, this, reg_.IsZeroFlagSet());
                break;
            case 0xD0:
                instruction_ = std::bind(&CPU::ReturnConditional, this, !reg_.IsCarryFlagSet());
                break;
            case 0xD8:
                instruction_ = std::bind(&CPU::ReturnConditional, this, reg_.IsCarryFlagSet());
                break;

            // RETI
            case 0xD9:
                instruction_ = std::bind(&CPU::Return, this, true);
                break;
        }
    }

    prefixedOpCode_ = false;
}
