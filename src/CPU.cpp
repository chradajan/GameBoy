#include <CPU.hpp>
#include <Paths.hpp>
#include <cstdint>
#include <fstream>
#include <functional>
#include <optional>
#include <sstream>
#include <utility>

CPU::CPU(std::function<uint8_t(uint16_t)> readFunction, std::function<void(uint16_t, uint8_t)> writeFunction) :
    Read(readFunction), Write(writeFunction)
{
    #ifdef LOG
        InitializeLogging();
    #endif
}

bool CPU::Clock(std::optional<std::pair<uint16_t, uint8_t>> interruptInfo)
{
    ++mCyclesTotal_;

    uint16_t interruptAddr = 0x0000;
    bool interruptAcknowledged = false;
    numPendingInterrupts_ = 0;

    if (interruptInfo)
    {
        interruptAddr = interruptInfo.value().first;
        numPendingInterrupts_ = interruptInfo.value().second;
    }

    if (mCycle_ == 0)
    {
        #ifdef LOG
            AddToLog();
            cyclesToLog_ = mCyclesTotal_ - 1;
        #endif

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
                interruptAcknowledged = true;
                --numPendingInterrupts_;
                instruction_ = std::bind(&CPU::InterruptHandler, this, interruptAddr);
                interruptsEnabled_ = false;
                interruptBeingProcessed_ = true;
            }

            halted_ = false;
            haltLogged_ = false;
        }
    }

    if (halted_)
    {
        return interruptAcknowledged;
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

    return interruptAcknowledged;
}

void CPU::Reset(bool const bootRom)
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

    if (bootRom)
    {
        reg_.PC = 0x0000;
    }
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

void CPU::InitializeLogging()
{
    log_.open(LOG_PATH.string() + "log.log");
    opCodeStream_.clear();
    regStream_.clear();
    mCyclesTotal_ = 0;
    haltLogged_ = false;
}

void CPU::AddToLog()
{
    if (mCyclesTotal_ == 1)
    {
        return;
    }

    if (halted_)
    {
        if (haltLogged_)
        {
            return;
        }

        haltLogged_ = true;
    }

    int spacesToAdd = 19 - opCodeStream_.str().length();
    opCodeStream_ << std::setw(spacesToAdd) << std::setfill(' ') << "";
    spacesToAdd = 22 - mnemonic_.str().length();

    log_ << opCodeStream_.str() << mnemonic_.str() << std::setw(spacesToAdd) << std::setfill(' ') << "" << regStream_.str();
    log_ << "  Cycles: " << std::dec << (unsigned int)cyclesToLog_ << "\n";
    mnemonic_.str(std::string());
    mnemonic_.clear();
    opCodeStream_.str(std::string());
    opCodeStream_.clear();
    regStream_.str(std::string());
    regStream_.clear();
}

void CPU::DecodeOpCode()
{
    #ifdef LOG
        if (!prefixedOpCode_)
        {
            opCodeStream_ << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << (unsigned int)reg_.PC << "  ";
            regStream_ << std::hex << std::uppercase;
            regStream_ << "A: "  << std::setw(2) << std::setfill('0') << (unsigned int)reg_.A  << " ";
            regStream_ << "F: "  << std::setw(2) << std::setfill('0') << (unsigned int)reg_.F  << " ";
            regStream_ << "B: "  << std::setw(2) << std::setfill('0') << (unsigned int)reg_.B  << " ";
            regStream_ << "C: "  << std::setw(2) << std::setfill('0') << (unsigned int)reg_.C  << " ";
            regStream_ << "D: "  << std::setw(2) << std::setfill('0') << (unsigned int)reg_.D  << " ";
            regStream_ << "E: "  << std::setw(2) << std::setfill('0') << (unsigned int)reg_.E  << " ";
            regStream_ << "H: "  << std::setw(2) << std::setfill('0') << (unsigned int)reg_.H  << " ";
            regStream_ << "L: "  << std::setw(2) << std::setfill('0') << (unsigned int)reg_.L  << " ";
            regStream_ << "SP: " << std::setw(4) << std::setfill('0') << (unsigned int)reg_.SP << "  ";
            regStream_ << "Flags: ";
            regStream_ << (reg_.IsZeroFlagSet() ? "Z" : " ");
            regStream_ << (reg_.IsSubtractionFlagSet() ? "N" : " ");
            regStream_ << (reg_.IsHalfCarryFlagSet() ? "H" : " ");
            regStream_ << (reg_.IsCarryFlagSet() ? "C" : " ");
        }
    #endif

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
        opCodeStream_ << "CB ";
        prefixedOpCode_ = true;
        return;
    }

    #ifdef LOG
        opCodeStream_ << std::setw(2) << std::setfill('0') << (unsigned int)opCode_ << " ";
    #endif

    if (prefixedOpCode_)
    {
        switch (opCode_)
        {
            // SWAP n
            case 0x37:
                mnemonic_ << "SWAP A";
                SwapRegNibbles(&reg_.A);
                break;
            case 0x30:
                mnemonic_ << "SWAP B";
                SwapRegNibbles(&reg_.B);
                break;
            case 0x31:
                mnemonic_ << "SWAP C";
                SwapRegNibbles(&reg_.C);
                break;
            case 0x32:
                mnemonic_ << "SWAP D";
                SwapRegNibbles(&reg_.D);
                break;
            case 0x33:
                mnemonic_ << "SWAP E";
                SwapRegNibbles(&reg_.E);
                break;
            case 0x34:
                mnemonic_ << "SWAP H";
                SwapRegNibbles(&reg_.H);
                break;
            case 0x35:
                mnemonic_ << "SWAP L";
                SwapRegNibbles(&reg_.L);
                break;
            case 0x36:
                mnemonic_ << "SWAP (HL) = ";
                instruction_ = std::bind(&CPU::SwapMemNibbles, this);
                break;

            // RLC n
            case 0x07:
                mnemonic_ << "RLC A";
                RLC(&reg_.A, true);
                break;
            case 0x00:
                mnemonic_ << "RLC B";
                RLC(&reg_.B, true);
                break;
            case 0x01:
                mnemonic_ << "RLC C";
                RLC(&reg_.C, true);
                break;
            case 0x02:
                mnemonic_ << "RLC D";
                RLC(&reg_.D, true);
                break;
            case 0x03:
                mnemonic_ << "RLC E";
                RLC(&reg_.E, true);
                break;
            case 0x04:
                mnemonic_ << "RLC H";
                RLC(&reg_.H, true);
                break;
            case 0x05:
                mnemonic_ << "RLC L";
                RLC(&reg_.L, true);
                break;
            case 0x06:
                mnemonic_ << "RLC (HL) = ";
                instruction_ = std::bind(&CPU::RLCMem, this);
                break;

            // RL n
            case 0x17:
                mnemonic_ << "RL A";
                RL(&reg_.A, true);
                break;
            case 0x10:
                mnemonic_ << "RL B";
                RL(&reg_.B, true);
                break;
            case 0x11:
                mnemonic_ << "RL C";
                RL(&reg_.C, true);
                break;
            case 0x12:
                mnemonic_ << "RL D";
                RL(&reg_.D, true);
                break;
            case 0x13:
                mnemonic_ << "RL E";
                RL(&reg_.E, true);
                break;
            case 0x14:
                mnemonic_ << "RL H";
                RL(&reg_.H, true);
                break;
            case 0x15:
                mnemonic_ << "RL L";
                RL(&reg_.L, true);
                break;
            case 0x16:
                mnemonic_ << "RL (HL) = ";
                instruction_ = std::bind(&CPU::RLMem, this);
                break;

            // RRC n
            case 0x0F:
                mnemonic_ << "RRC A";
                RRC(&reg_.A, true);
                break;
            case 0x08:
                mnemonic_ << "RRC B";
                RRC(&reg_.B, true);
                break;
            case 0x09:
                mnemonic_ << "RRC C";
                RRC(&reg_.C, true);
                break;
            case 0x0A:
                mnemonic_ << "RRC D";
                RRC(&reg_.D, true);
                break;
            case 0x0B:
                mnemonic_ << "RRC E";
                RRC(&reg_.E, true);
                break;
            case 0x0C:
                mnemonic_ << "RRC H";
                RRC(&reg_.H, true);
                break;
            case 0x0D:
                mnemonic_ << "RRC L";
                RRC(&reg_.L, true);
                break;
            case 0x0E:
                mnemonic_ << "RRC (HL) = ";
                instruction_ = std::bind(&CPU::RRCMem, this);
                break;

            // RR n
            case 0x1F:
                mnemonic_ << "RR A";
                RR(&reg_.A, true);
                break;
            case 0x18:
                mnemonic_ << "RR B";
                RR(&reg_.B, true);
                break;
            case 0x19:
                mnemonic_ << "RR C";
                RR(&reg_.C, true);
                break;
            case 0x1A:
                mnemonic_ << "RR D";
                RR(&reg_.D, true);
                break;
            case 0x1B:
                mnemonic_ << "RR E";
                RR(&reg_.E, true);
                break;
            case 0x1C:
                mnemonic_ << "RR H";
                RR(&reg_.H, true);
                break;
            case 0x1D:
                mnemonic_ << "RR L";
                RR(&reg_.L, true);
                break;
            case 0x1E:
                mnemonic_ << "RR (HL) = ";
                instruction_ = std::bind(&CPU::RRMem, this);
                break;

            // SLA n
            case 0x27:
                mnemonic_ << "SLA A";
                SLA(&reg_.A);
                break;
            case 0x20:
                mnemonic_ << "SLA B";
                SLA(&reg_.B);
                break;
            case 0x21:
                mnemonic_ << "SLA C";
                SLA(&reg_.C);
                break;
            case 0x22:
                mnemonic_ << "SLA D";
                SLA(&reg_.D);
                break;
            case 0x23:
                mnemonic_ << "SLA E";
                SLA(&reg_.E);
                break;
            case 0x24:
                mnemonic_ << "SLA H";
                SLA(&reg_.H);
                break;
            case 0x25:
                mnemonic_ << "SLA L";
                SLA(&reg_.L);
                break;
            case 0x26:
                mnemonic_ << "SLA (HL) = ";
                instruction_ = std::bind(&CPU::SLAMem, this);
                break;

            // SRA n
            case 0x2F:
                mnemonic_ << "SRA A";
                SRA(&reg_.A);
                break;
            case 0x28:
                mnemonic_ << "SRA B";
                SRA(&reg_.B);
                break;
            case 0x29:
                mnemonic_ << "SRA C";
                SRA(&reg_.C);
                break;
            case 0x2A:
                mnemonic_ << "SRA D";
                SRA(&reg_.D);
                break;
            case 0x2B:
                mnemonic_ << "SRA E";
                SRA(&reg_.E);
                break;
            case 0x2C:
                mnemonic_ << "SRA H";
                SRA(&reg_.H);
                break;
            case 0x2D:
                mnemonic_ << "SRA L";
                SRA(&reg_.L);
                break;
            case 0x2E:
                mnemonic_ << "SRA (HL) = ";
                instruction_ = std::bind(&CPU::SRAMem, this);
                break;

            // SRL n
            case 0x3F:
                mnemonic_ << "SRL A";
                SRL(&reg_.A);
                break;
            case 0x38:
                mnemonic_ << "SRL B";
                SRL(&reg_.B);
                break;
            case 0x39:
                mnemonic_ << "SRL C";
                SRL(&reg_.C);
                break;
            case 0x3A:
                mnemonic_ << "SRL D";
                SRL(&reg_.D);
                break;
            case 0x3B:
                mnemonic_ << "SRL E";
                SRL(&reg_.E);
                break;
            case 0x3C:
                mnemonic_ << "SRL H";
                SRL(&reg_.H);
                break;
            case 0x3D:
                mnemonic_ << "SRL L";
                SRL(&reg_.L);
                break;
            case 0x3E:
                mnemonic_ << "SRL (HL) = ";
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
                        mnemonic_ << "BIT " << (unsigned int)bit << ",B";
                        Bit(reg_.B, bit);
                        break;
                    case 1:
                        mnemonic_ << "BIT " << (unsigned int)bit << ",C";
                        Bit(reg_.C, bit);
                        break;
                    case 2:
                        mnemonic_ << "BIT " << (unsigned int)bit << ",D";
                        Bit(reg_.D, bit);
                        break;
                    case 3:
                        mnemonic_ << "BIT " << (unsigned int)bit << ",E";
                        Bit(reg_.E, bit);
                        break;
                    case 4:
                        mnemonic_ << "BIT " << (unsigned int)bit << ",H";
                        Bit(reg_.H, bit);
                        break;
                    case 5:
                        mnemonic_ << "BIT " << (unsigned int)bit << ",L";
                        Bit(reg_.L, bit);
                        break;
                    case 6:
                        mnemonic_ << "BIT " << (unsigned int)bit << ",(HL) = ";
                        instruction_ = std::bind(&CPU::BitMem, this, bit);
                        break;
                    case 7:
                        mnemonic_ << "BIT " << (unsigned int)bit << ",A";
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
                        mnemonic_ << "SET " << (unsigned int)bit << ", B";
                        Set(&reg_.B, bit);
                        break;
                    case 1:
                        mnemonic_ << "SET " << (unsigned int)bit << ", C";
                        Set(&reg_.C, bit);
                        break;
                    case 2:
                        mnemonic_ << "SET " << (unsigned int)bit << ", D";
                        Set(&reg_.D, bit);
                        break;
                    case 3:
                        mnemonic_ << "SET " << (unsigned int)bit << ", E";
                        Set(&reg_.E, bit);
                        break;
                    case 4:
                        mnemonic_ << "SET " << (unsigned int)bit << ", H";
                        Set(&reg_.H, bit);
                        break;
                    case 5:
                        mnemonic_ << "SET " << (unsigned int)bit << ", L";
                        Set(&reg_.L, bit);
                        break;
                    case 6:
                        mnemonic_ << "SET " << (unsigned int)bit << ", (HL) = ";
                        instruction_ = std::bind(&CPU::SetMem, this, bit);
                        break;
                    case 7:
                        mnemonic_ << "SET " << (unsigned int)bit << ", A";
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
                        mnemonic_ << "RES " << (unsigned int)bit << ", B";
                        Res(&reg_.B, bit);
                        break;
                    case 1:
                        mnemonic_ << "RES " << (unsigned int)bit << ", C";
                        Res(&reg_.C, bit);
                        break;
                    case 2:
                        mnemonic_ << "RES " << (unsigned int)bit << ", D";
                        Res(&reg_.D, bit);
                        break;
                    case 3:
                        mnemonic_ << "RES " << (unsigned int)bit << ", E";
                        Res(&reg_.E, bit);
                        break;
                    case 4:
                        mnemonic_ << "RES " << (unsigned int)bit << ", H";
                        Res(&reg_.H, bit);
                        break;
                    case 5:
                        mnemonic_ << "RES " << (unsigned int)bit << ", L";
                        Res(&reg_.L, bit);
                        break;
                    case 6:
                        mnemonic_ << "RES " << (unsigned int)bit << ", (HL) = ";
                        instruction_ = std::bind(&CPU::ResMem, this, bit);
                        break;
                    case 7:
                        mnemonic_ << "RES " << (unsigned int)bit << ", A";
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
                mnemonic_ << "LD A, #";
                instruction_ = std::bind(&CPU::LoadImmediateToReg, this, &reg_.A);
                break;
            case 0x06:
                mnemonic_ << "LD B, #";
                instruction_ = std::bind(&CPU::LoadImmediateToReg, this, &reg_.B);
                break;
            case 0x0E:
                mnemonic_ << "LD C, #";
                instruction_ = std::bind(&CPU::LoadImmediateToReg, this, &reg_.C);
                break;
            case 0x16:
                mnemonic_ << "LD D, #";
                instruction_ = std::bind(&CPU::LoadImmediateToReg, this, &reg_.D);
                break;
            case 0x1E:
                mnemonic_ << "LD E, #";
                instruction_ = std::bind(&CPU::LoadImmediateToReg, this, &reg_.E);
                break;
            case 0x26:
                mnemonic_ << "LD H, #";
                instruction_ = std::bind(&CPU::LoadImmediateToReg, this, &reg_.H);
                break;
            case 0x2E:
                mnemonic_ << "LD L, #";
                instruction_ = std::bind(&CPU::LoadImmediateToReg, this, &reg_.L);
                break;

            // LD r, r
            case 0x7F:
                mnemonic_ << "LD A, A";
                mCycle_ = 0;
                break;
            case 0x78:
                mnemonic_ << "LD A, B";
                reg_.A = reg_.B;
                mCycle_ = 0;
                break;
            case 0x79:
                mnemonic_ << "LD A, C";
                reg_.A = reg_.C;
                mCycle_ = 0;
                break;
            case 0x7A:
                mnemonic_ << "LD A, D";
                reg_.A = reg_.D;
                mCycle_ = 0;
                break;
            case 0x7B:
                mnemonic_ << "LD A, E";
                reg_.A = reg_.E;
                mCycle_ = 0;
                break;
            case 0x7C:
                mnemonic_ << "LD A, H";
                reg_.A = reg_.H;
                mCycle_ = 0;
                break;
            case 0x7D:
                mnemonic_ << "LD A, L";
                reg_.A = reg_.L;
                mCycle_ = 0;
                break;
            case 0x47:
                mnemonic_ << "LD B, A";
                reg_.B = reg_.A;
                mCycle_ = 0;
                break;
            case 0x40:
                mnemonic_ << "LD B, B";
                mCycle_ = 0;
                break;
            case 0x41:
                mnemonic_ << "LD B, C";
                reg_.B = reg_.C;
                mCycle_ = 0;
                break;
            case 0x42:
                mnemonic_ << "LD B, D";
                reg_.B = reg_.D;
                mCycle_ = 0;
                break;
            case 0x43:
                mnemonic_ << "LD B, E";
                reg_.B = reg_.E;
                mCycle_ = 0;
                break;
            case 0x44:
                mnemonic_ << "LD B, H";
                reg_.B = reg_.H;
                mCycle_ = 0;
                break;
            case 0x45:
                mnemonic_ << "LD B, L";
                reg_.B = reg_.L;
                mCycle_ = 0;
                break;
            case 0x4F:
                mnemonic_ << "LD C, A";
                reg_.C = reg_.A;
                mCycle_ = 0;
                break;
            case 0x48:
                mnemonic_ << "LD C, B";
                reg_.C = reg_.B;
                mCycle_ = 0;
                break;
            case 0x49:
                mnemonic_ << "LD C, C";
                mCycle_ = 0;
                break;
            case 0x4A:
                mnemonic_ << "LD C, D";
                reg_.C = reg_.D;
                mCycle_ = 0;
                break;
            case 0x4B:
                mnemonic_ << "LD C, E";
                reg_.C = reg_.E;
                mCycle_ = 0;
                break;
            case 0x4C:
                mnemonic_ << "LD C, H";
                reg_.C = reg_.H;
                mCycle_ = 0;
                break;
            case 0x4D:
                mnemonic_ << "LD C, L";
                reg_.C = reg_.L;
                mCycle_ = 0;
                break;
            case 0x57:
                mnemonic_ << "LD D, A";
                reg_.D = reg_.A;
                mCycle_ = 0;
                break;
            case 0x50:
                mnemonic_ << "LD D, B";
                reg_.D = reg_.B;
                mCycle_ = 0;
                break;
            case 0x51:
                mnemonic_ << "LD D, C";
                reg_.D = reg_.C;
                mCycle_ = 0;
                break;
            case 0x52:
                mnemonic_ << "LD D, D";
                mCycle_ = 0;
                break;
            case 0x53:
                mnemonic_ << "LD D, E";
                reg_.D = reg_.E;
                mCycle_ = 0;
                break;
            case 0x54:
                mnemonic_ << "LD D, H";
                reg_.D = reg_.H;
                mCycle_ = 0;
                break;
            case 0x55:
                mnemonic_ << "LD D, L";
                reg_.D = reg_.L;
                mCycle_ = 0;
                break;
            case 0x5F:
                mnemonic_ << "LD E, A";
                reg_.E = reg_.A;
                mCycle_ = 0;
                break;
            case 0x58:
                mnemonic_ << "LD E, B";
                reg_.E = reg_.B;
                mCycle_ = 0;
                break;
            case 0x59:
                mnemonic_ << "LD E, C";
                reg_.E = reg_.C;
                mCycle_ = 0;
                break;
            case 0x5A:
                mnemonic_ << "LD E, D";
                reg_.E = reg_.D;
                mCycle_ = 0;
                break;
            case 0x5B:
                mnemonic_ << "LD E, E";
                mCycle_ = 0;
                break;
            case 0x5C:
                mnemonic_ << "LD E, H";
                reg_.E = reg_.H;
                mCycle_ = 0;
                break;
            case 0x5D:
                mnemonic_ << "LD E, L";
                reg_.E = reg_.L;
                mCycle_ = 0;
                break;
            case 0x67:
                mnemonic_ << "LD H, A";
                reg_.H = reg_.A;
                mCycle_ = 0;
                break;
            case 0x60:
                mnemonic_ << "LD H, B";
                reg_.H = reg_.B;
                mCycle_ = 0;
                break;
            case 0x61:
                mnemonic_ << "LD H, C";
                reg_.H = reg_.C;
                mCycle_ = 0;
                break;
            case 0x62:
                mnemonic_ << "LD H, D";
                reg_.H = reg_.D;
                mCycle_ = 0;
                break;
            case 0x63:
                mnemonic_ << "LD H, E";
                reg_.H = reg_.E;
                mCycle_ = 0;
                break;
            case 0x64:
                mnemonic_ << "LD H, H";
                mCycle_ = 0;
                break;
            case 0x65:
                mnemonic_ << "LD H, L";
                reg_.H = reg_.L;
                mCycle_ = 0;
                break;
            case 0x6F:
                mnemonic_ << "LD L, A";
                reg_.L = reg_.A;
                mCycle_ = 0;
                break;
            case 0x68:
                mnemonic_ << "LD L, B";
                reg_.L = reg_.B;
                mCycle_ = 0;
                break;
            case 0x69:
                mnemonic_ << "LD L, C";
                reg_.L = reg_.C;
                mCycle_ = 0;
                break;
            case 0x6A:
                mnemonic_ << "LD L, D";
                reg_.L = reg_.D;
                mCycle_ = 0;
                break;
            case 0x6B:
                mnemonic_ << "LD L, E";
                reg_.L = reg_.E;
                mCycle_ = 0;
                break;
            case 0x6C:
                mnemonic_ << "LD L, H";
                reg_.L = reg_.H;
                mCycle_ = 0;
                break;
            case 0x6D:
                mnemonic_ << "LD L, L";
                mCycle_ = 0;
                break;

            // LD (nn), r
            case 0x02:
                mnemonic_ << "LD (BC), A";
                instruction_ = std::bind(&CPU::LoadRegToMem, this, reg_.BC, reg_.A);
                break;
            case 0x12:
                mnemonic_ << "LD (DE), A";
                instruction_ = std::bind(&CPU::LoadRegToMem, this, reg_.DE, reg_.A);
                break;
            case 0x77:
                mnemonic_ << "LD (HL), A";
                instruction_ = std::bind(&CPU::LoadRegToMem, this, reg_.HL, reg_.A);
                break;
            case 0xEA:
                mnemonic_ << "LD (nn), A";
                instruction_ = std::bind(&CPU::LoadRegToAbsoluteMem, this, reg_.A);
                break;
            case 0x70:
                mnemonic_ << "LD (HL), B";
                instruction_ = std::bind(&CPU::LoadRegToMem, this, reg_.HL, reg_.B);
                break;
            case 0x71:
                mnemonic_ << "LD (HL), C";
                instruction_ = std::bind(&CPU::LoadRegToMem, this, reg_.HL, reg_.C);
                break;
            case 0x72:
                mnemonic_ << "LD (HL), D";
                instruction_ = std::bind(&CPU::LoadRegToMem, this, reg_.HL, reg_.D);
                break;
            case 0x73:
                mnemonic_ << "LD (HL), E";
                instruction_ = std::bind(&CPU::LoadRegToMem, this, reg_.HL, reg_.E);
                break;
            case 0x74:
                mnemonic_ << "LD (HL), H";
                instruction_ = std::bind(&CPU::LoadRegToMem, this, reg_.HL, reg_.H);
                break;
            case 0x75:
                mnemonic_ << "LD (HL), L";
                instruction_ = std::bind(&CPU::LoadRegToMem, this, reg_.HL, reg_.L);
                break;
            case 0x36:
                mnemonic_ << "LD (HL), #";
                instruction_ = std::bind(&CPU::LoadImmediateToMem, this, reg_.HL);
                break;

            // LD r, (nn)
            case 0x7E:
                mnemonic_ << "LD A, (HL)";
                instruction_ = std::bind(&CPU::LoadMemToReg, this, &reg_.A, reg_.HL);
                break;
            case 0x0A:
                mnemonic_ << "LD A, (BC)";
                instruction_ = std::bind(&CPU::LoadMemToReg, this, &reg_.A, reg_.BC);
                break;
            case 0x1A:
                mnemonic_ << "LD A, (DE)";
                instruction_ = std::bind(&CPU::LoadMemToReg, this, &reg_.A, reg_.DE);
                break;
            case 0xFA:
                mnemonic_ << "LD A, $";
                instruction_ = std::bind(&CPU::LoadAbsoluteMemToReg, this, &reg_.A);
                break;
            case 0x46:
                mnemonic_ << "LD B, (HL)";
                instruction_ = std::bind(&CPU::LoadMemToReg, this, &reg_.B, reg_.HL);
                break;
            case 0x4E:
                mnemonic_ << "LD C, (HL)";
                instruction_ = std::bind(&CPU::LoadMemToReg, this, &reg_.C, reg_.HL);
                break;
            case 0x56:
                mnemonic_ << "LD D, (HL)";
                instruction_ = std::bind(&CPU::LoadMemToReg, this, &reg_.D, reg_.HL);
                break;
            case 0x5E:
                mnemonic_ << "LD E, (HL)";
                instruction_ = std::bind(&CPU::LoadMemToReg, this, &reg_.E, reg_.HL);
                break;
            case 0x66:
                mnemonic_ << "LD H, (HL)";
                instruction_ = std::bind(&CPU::LoadMemToReg, this, &reg_.H, reg_.HL);
                break;
            case 0x6E:
                mnemonic_ << "LD L, (HL)";
                instruction_ = std::bind(&CPU::LoadMemToReg, this, &reg_.L, reg_.HL);
                break;

            // LD A, ($FF00+C)
            case 0xF2:
                mnemonic_ << "LD A, ($FF00+C)";
                instruction_ = std::bind(&CPU::LoadMemToReg, this, &reg_.A, 0xFF00 + reg_.C);
                break;

            // LD ($FF00+C), A
            case 0xE2:
                mnemonic_ << "LD ($FF00+C), A";
                instruction_ = std::bind(&CPU::LoadRegToMem, this, 0xFF00 + reg_.C, reg_.A);
                break;

            // LD A, ($FF00+n)
            case 0xF0:
                mnemonic_ << "LD A, ($FF00+";
                instruction_ = std::bind(&CPU::LoadLastPageToReg, this);
                break;

            // LD ($FF00+n), A
            case 0xE0:
                mnemonic_ << "LD ($FF00+";
                instruction_ = std::bind(&CPU::LoadRegToLastPage, this);
                break;

            // LDD A, (HL)
            case 0x3A:
                mnemonic_ << "LDD A, (HL)";
                instruction_ = std::bind(&CPU::LoadMemToRegPostfix, this, false);
                break;

            // LDD (HL), A
            case 0x32:
                mnemonic_ << "LDD (HL), A";
                instruction_ = std::bind(&CPU::LoadRegToMemPostfix, this, false);
                break;

            // LDI A, (HL)
            case 0x2A:
                mnemonic_ << "LDI A, (HL)";
                instruction_ = std::bind(&CPU::LoadMemToRegPostfix, this, true);
                break;

            // LDI (HL), A
            case 0x22:
                mnemonic_ << "LDI (HL), A";
                instruction_ = std::bind(&CPU::LoadRegToMemPostfix, this, true);
                break;

            // LD rr, nn
            case 0x01:
                mnemonic_ << "LD BC, #";
                instruction_ = std::bind(&CPU::LoadImmediate16ToReg, this, &reg_.BC);
                break;
            case 0x11:
                mnemonic_ << "LD DE, #";
                instruction_ = std::bind(&CPU::LoadImmediate16ToReg, this, &reg_.DE);
                break;
            case 0x21:
                mnemonic_ << "LD HL, #";
                instruction_ = std::bind(&CPU::LoadImmediate16ToReg, this, &reg_.HL);
                break;
            case 0x31:
                mnemonic_ << "LD SP, #";
                instruction_ = std::bind(&CPU::LoadImmediate16ToReg, this, &reg_.SP);
                break;

            // LD SP, HL
            case 0xF9:
                mnemonic_ << "LD SP, HL";
                instruction_ = std::bind(&CPU::LoadHLToSP, this);
                break;

            // LD HL, SP+n
            case 0xF8:
                mnemonic_ << "LD HL, SP+";
                instruction_ = std::bind(&CPU::LoadSPnToHL, this);
                break;

            // LD (nn), SP
            case 0x08:
                mnemonic_ << "LD $";
                instruction_ = std::bind(&CPU::LoadSPToAbsoluteMem, this);
                break;

            // PUSH rr
            case 0xF5:
                mnemonic_ << "PUSH AF";
                instruction_ = std::bind(&CPU::PushReg16, this, reg_.AF);
                break;
            case 0xC5:
                mnemonic_ << "PUSH BC";
                instruction_ = std::bind(&CPU::PushReg16, this, reg_.BC);
                break;
            case 0xD5:
                mnemonic_ << "PUSH DE";
                instruction_ = std::bind(&CPU::PushReg16, this, reg_.DE);
                break;
            case 0xE5:
                mnemonic_ << "PUSH HL";
                instruction_ = std::bind(&CPU::PushReg16, this, reg_.HL);
                break;

            // POP rr
            case 0xF1:
                mnemonic_ << "POP AF";
                instruction_ = std::bind(&CPU::PopReg16, this, &reg_.AF, true);
                break;
            case 0xC1:
                mnemonic_ << "POP BC";
                instruction_ = std::bind(&CPU::PopReg16, this, &reg_.BC, false);
                break;
            case 0xD1:
                mnemonic_ << "POP DE";
                instruction_ = std::bind(&CPU::PopReg16, this, &reg_.DE, false);
                break;
            case 0xE1:
                mnemonic_ << "POP HL";
                instruction_ = std::bind(&CPU::PopReg16, this, &reg_.HL, false);
                break;

            // ADD A, n
            case 0x87:
                mnemonic_ << "ADD A, A";
                AddToReg(&reg_.A, reg_.A, false, false);
                break;
            case 0x80:
                mnemonic_ << "ADD A, B";
                AddToReg(&reg_.A, reg_.B, false, false);
                break;
            case 0x81:
                mnemonic_ << "ADD A, C";
                AddToReg(&reg_.A, reg_.C, false, false);
                break;
            case 0x82:
                mnemonic_ << "ADD A, D";
                AddToReg(&reg_.A, reg_.D, false, false);
                break;
            case 0x83:
                mnemonic_ << "ADD A, E";
                AddToReg(&reg_.A, reg_.E, false, false);
                break;
            case 0x84:
                mnemonic_ << "ADD A, H";
                AddToReg(&reg_.A, reg_.H, false, false);
                break;
            case 0x85:
                mnemonic_ << "ADD A, L";
                AddToReg(&reg_.A, reg_.L, false, false);
                break;
            case 0x86:
                mnemonic_ << "ADD A, (HL) = ";
                instruction_ = std::bind(&CPU::AddMemToA, this, false, false);
                break;
            case 0xC6:
                mnemonic_ << "ADD A, #";
                instruction_ = std::bind(&CPU::AddMemToA, this, true, false);
                break;

            // ADC A, n
            case 0x8F:
                mnemonic_ << "ADC A, A";
                AddToReg(&reg_.A, reg_.A, true, false);
                break;
            case 0x88:
                mnemonic_ << "ADC A, B";
                AddToReg(&reg_.A, reg_.B, true, false);
                break;
            case 0x89:
                mnemonic_ << "ADC A, C";
                AddToReg(&reg_.A, reg_.C, true, false);
                break;
            case 0x8A:
                mnemonic_ << "ADC A, D";
                AddToReg(&reg_.A, reg_.D, true, false);
                break;
            case 0x8B:
                mnemonic_ << "ADC A, E";
                AddToReg(&reg_.A, reg_.E, true, false);
                break;
            case 0x8C:
                mnemonic_ << "ADC A, H";
                AddToReg(&reg_.A, reg_.H, true, false);
                break;
            case 0x8D:
                mnemonic_ << "ADC A, L";
                AddToReg(&reg_.A, reg_.L, true, false);
                break;
            case 0x8E:
                mnemonic_ << "ADC A, (HL) = ";
                instruction_ = std::bind(&CPU::AddMemToA, this, false, true);
                break;
            case 0xCE:
                mnemonic_ << "ADC A, #";
                instruction_ = std::bind(&CPU::AddMemToA, this, true, true);
                break;

            // SUB A, n
            case 0x97:
                mnemonic_ << "SUB A, A";
                SubFromReg(&reg_.A, reg_.A, false, false, false);
                break;
            case 0x90:
                mnemonic_ << "SUB A, B";
                SubFromReg(&reg_.A, reg_.B, false, false, false);
                break;
            case 0x91:
                mnemonic_ << "SUB A, C";
                SubFromReg(&reg_.A, reg_.C, false, false, false);
                break;
            case 0x92:
                mnemonic_ << "SUB A, D";
                SubFromReg(&reg_.A, reg_.D, false, false, false);
                break;
            case 0x93:
                mnemonic_ << "SUB A, E";
                SubFromReg(&reg_.A, reg_.E, false, false, false);
                break;
            case 0x94:
                mnemonic_ << "SUB A, H";
                SubFromReg(&reg_.A, reg_.H, false, false, false);
                break;
            case 0x95:
                mnemonic_ << "SUB A, L";
                SubFromReg(&reg_.A, reg_.L, false, false, false);
                break;
            case 0x96:
                mnemonic_ << "SUB A, (HL) = ";
                instruction_ = std::bind(&CPU::SubMemFromA, this, false, false, false);
                break;
            case 0xD6:
                mnemonic_ << "SUB A, #";
                instruction_ = std::bind(&CPU::SubMemFromA, this, true, false, false);
                break;

            // SBC A, n
            case 0x9F:
                mnemonic_ << "SBC A, A";
                SubFromReg(&reg_.A, reg_.A, true, false, false);
                break;
            case 0x98:
                mnemonic_ << "SBC A, B";
                SubFromReg(&reg_.A, reg_.B, true, false, false);
                break;
            case 0x99:
                mnemonic_ << "SBC A, C";
                SubFromReg(&reg_.A, reg_.C, true, false, false);
                break;
            case 0x9A:
                mnemonic_ << "SBC A, D";
                SubFromReg(&reg_.A, reg_.D, true, false, false);
                break;
            case 0x9B:
                mnemonic_ << "SBC A, E";
                SubFromReg(&reg_.A, reg_.E, true, false, false);
                break;
            case 0x9C:
                mnemonic_ << "SBC A, H";
                SubFromReg(&reg_.A, reg_.H, true, false, false);
                break;
            case 0x9D:
                mnemonic_ << "SBC A, L";
                SubFromReg(&reg_.A, reg_.L, true, false, false);
                break;
            case 0x9E:
                mnemonic_ << "SBC A, (HL) = ";
                instruction_ = std::bind(&CPU::SubMemFromA, this, false, true, false);
                break;
            case 0xDE:
                mnemonic_ << "SBC A, #";
                instruction_ = std::bind(&CPU::SubMemFromA, this, true, true, false);
                break;

            // AND A, n
            case 0xA7:
                mnemonic_ << "AND A, A";
                AndWithA(reg_.A);
                break;
            case 0xA0:
                mnemonic_ << "AND A, B";
                AndWithA(reg_.B);
                break;
            case 0xA1:
                mnemonic_ << "AND A, C";
                AndWithA(reg_.C);
                break;
            case 0xA2:
                mnemonic_ << "AND A, D";
                AndWithA(reg_.D);
                break;
            case 0xA3:
                mnemonic_ << "AND A, E";
                AndWithA(reg_.E);
                break;
            case 0xA4:
                mnemonic_ << "AND A, H";
                AndWithA(reg_.H);
                break;
            case 0xA5:
                mnemonic_ << "AND A, L";
                AndWithA(reg_.L);
                break;
            case 0xA6:
                mnemonic_ << "AND A, (HL) = ";
                instruction_ = std::bind(&CPU::AndMemWithA, this, false);
                break;
            case 0xE6:
                mnemonic_ << "AND A, #";
                instruction_ = std::bind(&CPU::AndMemWithA, this, true);
                break;

            // OR A, n
            case 0xB7:
                mnemonic_ << "OR A, A";
                OrWithA(reg_.A);
                break;
            case 0xB0:
                mnemonic_ << "OR A, B";
                OrWithA(reg_.B);
                break;
            case 0xB1:
                mnemonic_ << "OR A, C";
                OrWithA(reg_.C);
                break;
            case 0xB2:
                mnemonic_ << "OR A, D";
                OrWithA(reg_.D);
                break;
            case 0xB3:
                mnemonic_ << "OR A, E";
                OrWithA(reg_.E);
                break;
            case 0xB4:
                mnemonic_ << "OR A, H";
                OrWithA(reg_.H);
                break;
            case 0xB5:
                mnemonic_ << "OR A, L";
                OrWithA(reg_.L);
                break;
            case 0xB6:
                mnemonic_ << "OR A, (HL) = ";
                instruction_ = std::bind(&CPU::OrMemWithA, this, false);
                break;
            case 0xF6:
                mnemonic_ << "OR A, #";
                instruction_ = std::bind(&CPU::OrMemWithA, this, true);
                break;

            // XOR A, n
            case 0xAF:
                mnemonic_ << "XOR A, A";
                XorWithA(reg_.A);
                break;
            case 0xA8:
                mnemonic_ << "XOR A, B";
                XorWithA(reg_.B);
                break;
            case 0xA9:
                mnemonic_ << "XOR A, C";
                XorWithA(reg_.C);
                break;
            case 0xAA:
                mnemonic_ << "XOR A, D";
                XorWithA(reg_.D);
                break;
            case 0xAB:
                mnemonic_ << "XOR A, E";
                XorWithA(reg_.E);
                break;
            case 0xAC:
                mnemonic_ << "XOR A, H";
                XorWithA(reg_.H);
                break;
            case 0xAD:
                mnemonic_ << "XOR A, L";
                XorWithA(reg_.L);
                break;
            case 0xAE:
                mnemonic_ << "XOR A, (HL) = ";
                instruction_ = std::bind(&CPU::XorMemWithA, this, false);
                break;
            case 0xEE:
                mnemonic_ << "XOR A, #";
                instruction_ = std::bind(&CPU::XorMemWithA, this, true);
                break;

            // CP n
            case 0xBF:
                mnemonic_ << "CP A";
                SubFromReg(&reg_.A, reg_.A, false, true, false);
                break;
            case 0xB8:
                mnemonic_ << "CP B";
                SubFromReg(&reg_.A, reg_.B, false, true, false);
                break;
            case 0xB9:
                mnemonic_ << "CP C";
                SubFromReg(&reg_.A, reg_.C, false, true, false);
                break;
            case 0xBA:
                mnemonic_ << "CP D";
                SubFromReg(&reg_.A, reg_.D, false, true, false);
                break;
            case 0xBB:
                mnemonic_ << "CP E";
                SubFromReg(&reg_.A, reg_.E, false, true, false);
                break;
            case 0xBC:
                mnemonic_ << "CP H";
                SubFromReg(&reg_.A, reg_.H, false, true, false);
                break;
            case 0xBD:
                mnemonic_ << "CP L";
                SubFromReg(&reg_.A, reg_.L, false, true, false);
                break;
            case 0xBE:
                mnemonic_ << "CP (HL) = ";
                instruction_ = std::bind(&CPU::SubMemFromA, this, false, false, true);
                break;
            case 0xFE:
                mnemonic_ << "CP #";
                instruction_ = std::bind(&CPU::SubMemFromA, this, true, false, true);
                break;

            // INC n
            case 0x3C:
                mnemonic_ << "INC A";
                AddToReg(&reg_.A, 1, false, true);
                break;
            case 0x04:
                mnemonic_ << "INC B";
                AddToReg(&reg_.B, 1, false, true);
                break;
            case 0x0C:
                mnemonic_ << "INC C";
                AddToReg(&reg_.C, 1, false, true);
                break;
            case 0x14:
                mnemonic_ << "INC D";
                AddToReg(&reg_.D, 1, false, true);
                break;
            case 0x1C:
                mnemonic_ << "INC E";
                AddToReg(&reg_.E, 1, false, true);
                break;
            case 0x24:
                mnemonic_ << "INC H";
                AddToReg(&reg_.H, 1, false, true);
                break;
            case 0x2C:
                mnemonic_ << "INC L";
                AddToReg(&reg_.L, 1, false, true);
                break;
            case 0x34:
                mnemonic_ << "INC (HL) = ";
                instruction_ = std::bind(&CPU::IncHL, this);
                break;

            // DEC n
            case 0x3D:
                mnemonic_ << "DEC A";
                SubFromReg(&reg_.A, 1, false, false, true);
                break;
            case 0x05:
                mnemonic_ << "DEC B";
                SubFromReg(&reg_.B, 1, false, false, true);
                break;
            case 0x0D:
                mnemonic_ << "DEC C";
                SubFromReg(&reg_.C, 1, false, false, true);
                break;
            case 0x15:
                mnemonic_ << "DEC D";
                SubFromReg(&reg_.D, 1, false, false, true);
                break;
            case 0x1D:
                mnemonic_ << "DEC E";
                SubFromReg(&reg_.E, 1, false, false, true);
                break;
            case 0x25:
                mnemonic_ << "DEC H";
                SubFromReg(&reg_.H, 1, false, false, true);
                break;
            case 0x2D:
                mnemonic_ << "DEC L";
                SubFromReg(&reg_.L, 1, false, false, true);
                break;
            case 0x35:
                mnemonic_ << "DEC (HL) = ";
                instruction_ = std::bind(&CPU::DecHL, this);
                break;

            // ADD HL, n
            case 0x09:
                mnemonic_ << "ADD HL, BC";
                instruction_ = std::bind(&CPU::AddRegToHL, this, reg_.BC);
                break;
            case 0x19:
                mnemonic_ << "ADD HL, DE";
                instruction_ = std::bind(&CPU::AddRegToHL, this, reg_.DE);
                break;
            case 0x29:
                mnemonic_ << "ADD HL, HL";
                instruction_ = std::bind(&CPU::AddRegToHL, this, reg_.HL);
                break;
            case 0x39:
                mnemonic_ << "ADD HL, SP";
                instruction_ = std::bind(&CPU::AddRegToHL, this, reg_.SP);
                break;

            // ADD SP, n
            case 0xE8:
                mnemonic_ << "ADD SP, #";
                instruction_ = std::bind(&CPU::AddImmediateToSP, this);
                break;

            // INC nn
            case 0x03:
                mnemonic_ << "INC BC";
                instruction_ = std::bind(&CPU::IncDec16, this, &reg_.BC, 1);
                break;
            case 0x13:
                mnemonic_ << "INC DE";
                instruction_ = std::bind(&CPU::IncDec16, this, &reg_.DE, 1);
                break;
            case 0x23:
                mnemonic_ << "INC HL";
                instruction_ = std::bind(&CPU::IncDec16, this, &reg_.HL, 1);
                break;
            case 0x33:
                mnemonic_ << "INC SP";
                instruction_ = std::bind(&CPU::IncDec16, this, &reg_.SP, 1);
                break;

            // DEC nn
            case 0x0B:
                mnemonic_ << "DEC BC";
                instruction_ = std::bind(&CPU::IncDec16, this, &reg_.BC, -1);
                break;
            case 0x1B:
                mnemonic_ << "DEC DE";
                instruction_ = std::bind(&CPU::IncDec16, this, &reg_.DE, -1);
                break;
            case 0x2B:
                mnemonic_ << "DEC HL";
                instruction_ = std::bind(&CPU::IncDec16, this, &reg_.HL, -1);
                break;
            case 0x3B:
                mnemonic_ << "DEC SP";
                instruction_ = std::bind(&CPU::IncDec16, this, &reg_.SP, -1);
                break;

            // DAA
            case 0x27:
                mnemonic_ << "DAA";
                DAA();
                break;

            // CPL
            case 0x2F:
                mnemonic_ << "CPL";
                reg_.A = ~reg_.A;
                reg_.SetSubtractionFlag(true);
                reg_.SetHalfCarryFlag(true);
                mCycle_ = 0;
                break;

            // CCF
            case 0x3F:
                mnemonic_ << "CCF";
                reg_.SetCarryFlag(!reg_.IsCarryFlagSet());
                reg_.SetSubtractionFlag(false);
                reg_.SetHalfCarryFlag(false);
                mCycle_ = 0;
                break;

            // SCF
            case 0x37:
                mnemonic_ << "SCF";
                reg_.SetSubtractionFlag(false);
                reg_.SetHalfCarryFlag(false);
                reg_.SetCarryFlag(true);
                mCycle_ = 0;
                break;

            // NOP
            case 0x00:
                mnemonic_ << "NOP";
                mCycle_ = 0;
                break;

            // HALT
            case 0x76:
                mnemonic_ << "HALT";
                Halt();
                break;

            // STOP
            case 0x10:
                mnemonic_ << "STOP";
                Stop();
                break;

            // DI
            case 0xF3:
                mnemonic_ << "DI";
                DI();
                break;

            // EI
            case 0xFB:
                mnemonic_ << "EI";
                EI();
                break;

            // RLCA
            case 0x07:
                mnemonic_ << "RLCA";
                RLC(&reg_.A, false);
                break;

            // RLA
            case 0x17:
                mnemonic_ << "RLA";
                RL(&reg_.A, false);
                break;

            // RRCA
            case 0x0F:
                mnemonic_ << "RRCA";
                RRC(&reg_.A, false);
                break;

            // RRA
            case 0x1F:
                mnemonic_ << "RRA";
                RR(&reg_.A, false);
                break;

            // JP nn
            case 0xC3:
                mnemonic_ << "JP $";
                instruction_ = std::bind(&CPU::JumpToAbsolute, this, true);
                break;

            // JP cc, nn
            case 0xC2:
                mnemonic_ << "JP NZ, $";
                instruction_ = std::bind(&CPU::JumpToAbsolute, this, !reg_.IsZeroFlagSet());
                break;
            case 0xCA:
                mnemonic_ << "JP Z, $";
                instruction_ = std::bind(&CPU::JumpToAbsolute, this, reg_.IsZeroFlagSet());
                break;
            case 0xD2:
                mnemonic_ << "JP NC, $";
                instruction_ = std::bind(&CPU::JumpToAbsolute, this, !reg_.IsCarryFlagSet());
                break;
            case 0xDA:
                mnemonic_ << "JP C, $";
                instruction_ = std::bind(&CPU::JumpToAbsolute, this, reg_.IsCarryFlagSet());
                break;

            // JP HL
            case 0xE9:
                mnemonic_ << "JP HL";
                reg_.PC = reg_.HL;
                mCycle_ = 0;
                break;

            // JR n
            case 0x18:
                mnemonic_ << "JR $";
                instruction_ = std::bind(&CPU::JumpToRelative, this, true);
                break;

            // JR cc,n
            case 0x20:
                mnemonic_ << "JR NZ, $";
                instruction_ = std::bind(&CPU::JumpToRelative, this, !reg_.IsZeroFlagSet());
                break;
            case 0x28:
                mnemonic_ << "JR Z, $";
                instruction_ = std::bind(&CPU::JumpToRelative, this, reg_.IsZeroFlagSet());
                break;
            case 0x30:
                mnemonic_ << "JR NC, $";
                instruction_ = std::bind(&CPU::JumpToRelative, this, !reg_.IsCarryFlagSet());
                break;
            case 0x38:
                mnemonic_ << "JR C, $";
                instruction_ = std::bind(&CPU::JumpToRelative, this, reg_.IsCarryFlagSet());
                break;

            // CALL nn
            case 0xCD:
                mnemonic_ << "CALL $";
                instruction_ = std::bind(&CPU::Call, this, true);
                break;

            // CALL cc,nn
            case 0xC4:
                mnemonic_ << "CALL NZ, $";
                instruction_ = std::bind(&CPU::Call, this, !reg_.IsZeroFlagSet());
                break;
            case 0xCC:
                mnemonic_ << "CALL Z, $";
                instruction_ = std::bind(&CPU::Call, this, reg_.IsZeroFlagSet());
                break;
            case 0xD4:
                mnemonic_ << "CALL NC, $";
                instruction_ = std::bind(&CPU::Call, this, !reg_.IsCarryFlagSet());
                break;
            case 0xDC:
                mnemonic_ << "CALL C, $";
                instruction_ = std::bind(&CPU::Call, this, reg_.IsCarryFlagSet());
                break;

            // RST n
            case 0xC7:
                mnemonic_ << "RST $0000";
                instruction_ = std::bind(&CPU::Restart, this, 0x00);
                break;
            case 0xCF:
                mnemonic_ << "RST $0008";
                instruction_ = std::bind(&CPU::Restart, this, 0x08);
                break;
            case 0xD7:
                mnemonic_ << "RST $0010";
                instruction_ = std::bind(&CPU::Restart, this, 0x10);
                break;
            case 0xDF:
                mnemonic_ << "RST $0018";
                instruction_ = std::bind(&CPU::Restart, this, 0x18);
                break;
            case 0xE7:
                mnemonic_ << "RST $0020";
                instruction_ = std::bind(&CPU::Restart, this, 0x20);
                break;
            case 0xEF:
                mnemonic_ << "RST $0028";
                instruction_ = std::bind(&CPU::Restart, this, 0x28);
                break;
            case 0xF7:
                mnemonic_ << "RST $0030";
                instruction_ = std::bind(&CPU::Restart, this, 0x30);
                break;
            case 0xFF:
                mnemonic_ << "RST $0038";
                instruction_ = std::bind(&CPU::Restart, this, 0x38);
                break;

            // RET
            case 0xC9:
                mnemonic_ << "RET";
                instruction_ = std::bind(&CPU::Return, this, false);
                break;

            // RET cc
            case 0xC0:
                mnemonic_ << "RET NZ";
                instruction_ = std::bind(&CPU::ReturnConditional, this, !reg_.IsZeroFlagSet());
                break;
            case 0xC8:
                mnemonic_ << "RET Z";
                instruction_ = std::bind(&CPU::ReturnConditional, this, reg_.IsZeroFlagSet());
                break;
            case 0xD0:
                mnemonic_ << "RET NC";
                instruction_ = std::bind(&CPU::ReturnConditional, this, !reg_.IsCarryFlagSet());
                break;
            case 0xD8:
                mnemonic_ << "RET C";
                instruction_ = std::bind(&CPU::ReturnConditional, this, reg_.IsCarryFlagSet());
                break;

            // RETI
            case 0xD9:
                mnemonic_ << "RETI";
                instruction_ = std::bind(&CPU::Return, this, true);
                break;
        }
    }

    prefixedOpCode_ = false;
}
