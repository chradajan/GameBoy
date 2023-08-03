#include <CPU.hpp>
#include <cstdint>

void CPU::InterruptHandler(uint16_t addr)
{
    switch (mCycle_)
    {
        case 1:
        case 2:
            break;
        case 3:
            Push(reg_.PC >> 8);
            break;
        case 4:
            Push(reg_.PC & 0x00FF);
            break;
        case 5:
            reg_.PC = addr;
            interruptBeingProcessed_ = false;
            mCycle_ = 0;
            break;
    }
}

void CPU::LoadImmediateToReg(uint8_t* destReg)
{
    *destReg = ReadPC();
    mCycle_ = 0;
}

void CPU::LoadMemToReg(uint8_t* destReg, uint16_t srcAddr)
{
    *destReg = Read(srcAddr);
    mCycle_ = 0;
}

void CPU::LoadRegToMem(uint16_t destAddr, uint8_t srcReg)
{
    Write(destAddr, srcReg);
    mCycle_ = 0;
}

void CPU::LoadImmediateToMem(uint16_t destAddr)
{
    switch (mCycle_)
    {
        case 2:
            cmdData8_ = ReadPC();
            break;
        case 3:
            Write(destAddr, cmdData8_);
            mCycle_ = 0;
            break;
    }
}

void CPU::LoadAbsoluteMemToReg(uint8_t* destReg)
{
    switch (mCycle_)
    {
        case 2:
            cmdData16_ = ReadPC();
            break;
        case 3:
            cmdData16_ = (ReadPC() << 8) | cmdData16_;
            break;
        case 4:
            *destReg = Read(cmdData16_);
            mCycle_ = 0;
            break;
    }
}

void CPU::LoadRegToAbsoluteMem(uint8_t srcReg)
{
    switch (mCycle_)
    {
        case 2:
            cmdData16_ = ReadPC();
            break;
        case 3:
            cmdData16_ = (ReadPC() << 8) | cmdData16_;
            break;
        case 4:
            Write(cmdData16_, srcReg);
            mCycle_ = 0;
            break;
    }
}

void CPU::LoadMemToRegPostfix(bool increment)
{
    reg_.A = Read(reg_.HL);
    reg_.HL += increment ? 1 : -1;
    mCycle_ = 0;
}

void CPU::LoadRegToMemPostfix(bool increment)
{
    Write(reg_.HL, reg_.A);
    reg_.HL += increment ? 1 : -1;
    mCycle_ = 0;
}

void CPU::LoadLastPageToReg()
{
    switch (mCycle_)
    {
        case 2:
            cmdData8_ = ReadPC();
            break;
        case 3:
            reg_.A = Read(0xFF00 + cmdData8_);
            mCycle_ = 0;
            break;
    }
}

void CPU::LoadRegToLastPage()
{
    switch (mCycle_)
    {
        case 2:
            cmdData8_ = ReadPC();
            break;
        case 3:
            Write(0xFF00 + cmdData8_, reg_.A);
            mCycle_ = 0;
            break;
    }
}

void CPU::LoadImmediate16ToReg(uint16_t* destReg)
{
    switch (mCycle_)
    {
        case 2:
            cmdData16_ = ReadPC();
            break;
        case 3:
            cmdData16_ = (ReadPC() << 8) | cmdData16_;
            *destReg = cmdData16_;
            mCycle_ = 0;
            break;
    }
}

void CPU::LoadHLToSP()
{
    reg_.SP = reg_.HL;
    mCycle_ = 0;
}

void CPU::LoadSPnToHL()
{
    switch (mCycle_)
    {
        case 2:
            cmdData8_ = ReadPC();
            break;
        case 3:
            int8_t signedImmediate = cmdData8_;
            uint32_t result = reg_.SP + signedImmediate;

            reg_.SetZeroFlag(false);
            reg_.SetSubtractionFlag(false);
            reg_.SetHalfCarryFlag(((result ^ reg_.SP ^ signedImmediate) & 0x10) == 0x10);
            reg_.SetCarryFlag(((result ^ reg_.SP ^ signedImmediate) & 0x100) == 0x100);
            reg_.HL = result & 0x0000FFFF;
            mCycle_ = 0;
            break;
    }
}

void CPU::LoadSPToAbsoluteMem()
{
    switch (mCycle_)
    {
        case 2:
            cmdData16_ = ReadPC();
            break;
        case 3:
            cmdData16_ = (ReadPC() << 8) | cmdData16_;
            break;
        case 4:
            Write(cmdData16_, reg_.SP & 0xFF);
            break;
        case 5:
            Write(cmdData16_ + 1, reg_.SP >> 8);
            mCycle_ = 0;
            break;
    }
}

void CPU::PushReg16(uint16_t srcReg)
{
    switch (mCycle_)
    {
        case 2:
            break;
        case 3:
            Push(srcReg >> 8);
            break;
        case 4:
            Push(srcReg & 0xFF);
            mCycle_ = 0;
            break;
    }
}

void CPU::PopReg16(uint16_t* destReg, bool afPop)
{
    switch (mCycle_)
    {
        case 2:
            cmdData16_ = Pop();
            break;
        case 3:
            cmdData16_ = (Pop() << 8) | cmdData16_;
            *destReg = cmdData16_;

            if (afPop)
            {
                reg_.F &= 0xF0;
            }

            mCycle_ = 0;
            break;
    }
}

void CPU::AddToReg(uint8_t* destReg, uint8_t operand, bool adc, bool inc)
{
    uint16_t result = *destReg + operand;

    if (adc && reg_.IsCarryFlagSet())
    {
        ++result;
    }

    reg_.SetZeroFlag((result & 0x00FF) == 0x0000);
    reg_.SetSubtractionFlag(false);
    reg_.SetHalfCarryFlag(((result ^ *destReg ^ operand) & 0x0010) == 0x0010);

    if (!inc)
    {
        reg_.SetCarryFlag((result & 0x0100) == 0x0100);
    }

    *destReg = result & 0x00FF;
    mCycle_ = 0;
}

void CPU::AddMemToA(bool immediate, bool adc)
{
    uint8_t operand = immediate ? ReadPC() : Read(reg_.HL);
    AddToReg(&reg_.A, operand, adc, false);
}

void CPU::SubFromReg(uint8_t* destReg, uint8_t operand, bool sbc, bool cp, bool dec)
{
    uint16_t result = *destReg - operand;
    uint8_t halfResult = (*destReg & 0x0F) - (operand & 0x0F);

    if (sbc && reg_.IsCarryFlagSet())
    {
        --result;
        --halfResult;
    }

    reg_.SetZeroFlag((result & 0x00FF) == 0x0000);
    reg_.SetSubtractionFlag(true);
    reg_.SetHalfCarryFlag((halfResult & 0xF0) != 0x00);

    if (!dec)
    {
        reg_.SetCarryFlag((result & 0xFF00) != 0x0000);
    }

    if (!cp)
    {
        *destReg = result & 0x00FF;
    }

    mCycle_ = 0;
}

void CPU::SubMemFromA(bool immediate, bool sbc, bool cp)
{
    uint8_t operand = immediate ? ReadPC() : Read(reg_.HL);
    SubFromReg(&reg_.A, operand, sbc, cp, false);
}

void CPU::AndWithA(uint8_t operand)
{
    reg_.A &= operand;
    reg_.SetZeroFlag(reg_.A == 0x00);
    reg_.SetSubtractionFlag(false);
    reg_.SetHalfCarryFlag(true);
    reg_.SetCarryFlag(false);
    mCycle_ = 0;
}

void CPU::AndMemWithA(bool immediate)
{
    uint8_t operand = immediate ? ReadPC() : Read(reg_.HL);
    AndWithA(operand);
}

void CPU::OrWithA(uint8_t operand)
{
    reg_.A |= operand;
    reg_.SetZeroFlag(reg_.A == 0x00);
    reg_.SetSubtractionFlag(false);
    reg_.SetHalfCarryFlag(false);
    reg_.SetCarryFlag(false);
    mCycle_ = 0;
}

void CPU::OrMemWithA(bool immediate)
{
    uint8_t operand = immediate ? ReadPC() : Read(reg_.HL);
    OrWithA(operand);
}

void CPU::XorWithA(uint8_t operand)
{
    reg_.A ^= operand;
    reg_.SetZeroFlag(reg_.A == 0x00);
    reg_.SetSubtractionFlag(false);
    reg_.SetHalfCarryFlag(false);
    reg_.SetCarryFlag(false);
    mCycle_ = 0;
}

void CPU::XorMemWithA(bool immediate)
{
    uint8_t operand = immediate ? ReadPC() : Read(reg_.HL);
    XorWithA(operand);
}

void CPU::IncHL()
{
    switch (mCycle_)
    {
        case 2:
            cmdData8_ = Read(reg_.HL);
            break;
        case 3:
            AddToReg(&cmdData8_, 1, false, true);
            Write(reg_.HL, cmdData8_);
    }
}

void CPU::DecHL()
{
    switch (mCycle_)
    {
        case 2:
            cmdData8_ = Read(reg_.HL);
            break;
        case 3:
            SubFromReg(&cmdData8_, 1, false, false, true);
            Write(reg_.HL, cmdData8_);
    }
}

void CPU::AddRegToHL(uint16_t operand)
{
    uint32_t result = reg_.HL + operand;
    reg_.SetSubtractionFlag(false);
    reg_.SetHalfCarryFlag(((result ^ reg_.HL ^ operand) & 0x1000) == 0x1000);
    reg_.SetCarryFlag((result & 0x00010000) == 0x00010000);
    reg_.HL = result & 0x0000FFFF;
    mCycle_ = 0;
}

void CPU::AddImmediateToSP()
{
    switch (mCycle_)
    {
        case 2:
            cmdData8_ = ReadPC();
            break;
        case 3:
            break;
        case 4:
            int8_t signedImmediate = cmdData8_;
            uint16_t result = reg_.SP + signedImmediate;

            reg_.SetZeroFlag(false);
            reg_.SetSubtractionFlag(false);
            reg_.SetHalfCarryFlag(((result ^ reg_.SP ^ signedImmediate) & 0x10) == 0x10);
            reg_.SetCarryFlag(((result ^ reg_.SP ^ signedImmediate) & 0x100) == 0x100);
            reg_.SP = result;
            mCycle_ = 0;
            break;
    }
}

void CPU::IncDec16(uint16_t* destReg, int8_t operand)
{
    *destReg += operand;
    mCycle_ = 0;
}

void CPU::SwapRegNibbles(uint8_t* destReg)
{
    *destReg = ((*destReg & 0x0F) << 4) | ((*destReg & 0xF0) >> 4);
    reg_.SetZeroFlag(*destReg == 0x00);
    reg_.SetSubtractionFlag(false);
    reg_.SetHalfCarryFlag(false);
    reg_.SetCarryFlag(false);
    mCycle_ = 0;
}

void CPU::SwapMemNibbles()
{
    switch (mCycle_)
    {
        case 3:
            cmdData8_ = Read(reg_.HL);
            break;
        case 4:
            SwapRegNibbles(&cmdData8_);
            Write(reg_.HL, cmdData8_);
            break;
    }
}

void CPU::DAA()
{
    if (!reg_.IsSubtractionFlagSet())
    {
        if (reg_.IsCarryFlagSet() || (reg_.A > 0x99))
        {
            reg_.A += 0x60;
            reg_.SetCarryFlag(true);
        }

        if (reg_.IsHalfCarryFlagSet() || ((reg_.A & 0x0F) > 0x09))
        {
            reg_.A += 0x06;
        }
    }
    else
    {
        if (reg_.IsCarryFlagSet())
        {
            reg_.A -= 0x60;
            reg_.SetCarryFlag(true);
        }

        if (reg_.IsHalfCarryFlagSet())
        {
            reg_.A -= 0x06;
        }
    }

    reg_.SetZeroFlag(reg_.A == 0x00);
    reg_.SetHalfCarryFlag(false);
    mCycle_ = 0;
}

void CPU::Halt()
{
    if (!interruptsEnabled_ && (numPendingInterrupts_ > 0))
    {
        if (setInterruptsEnabled_)
        {
            setInterruptsEnabled_ = false;
            interruptCountdown_ = 0;
            interruptsEnabled_ = true;
        }
        else
        {
            haltBug_ = true;
        }
    }
    else
    {
        halted_ = true;
    }

    mCycle_ = 0;
}

void CPU::Stop()
{
    auto [readNextByte, halted] = ReportStop(interruptsEnabled_);

    if (readNextByte)
    {
        cmdData8_ = ReadPC();
    }

    halted_ = halted;
    mCycle_ = 0;
}

void CPU::DI()
{
    if (setInterruptsEnabled_)
    {
        setInterruptsEnabled_ = false;
        interruptsEnabled_ = true;
    }
    else if (setInterruptsDisabled_)
    {
        interruptsEnabled_ = false;
    }

    setInterruptsDisabled_ = true;
    interruptCountdown_ = 2;
    mCycle_ = 0;
}

void CPU::EI()
{
    if (setInterruptsDisabled_)
    {
        setInterruptsDisabled_ = false;
        interruptsEnabled_ = false;
    }
    else if (setInterruptsEnabled_)
    {
        interruptsEnabled_ = true;
    }

    setInterruptsEnabled_ = true;
    interruptCountdown_ = 2;
    mCycle_ = 0;
}

void CPU::RLC(uint8_t* reg, bool prefix)
{
    bool msbSet = (*reg & 0x80) == 0x80;
    *reg <<= 1;
    *reg |= msbSet ? 0x01 : 0x00;

    if (prefix)
    {
        reg_.SetZeroFlag(*reg == 0x00);
    }
    else
    {
        reg_.SetZeroFlag(false);
    }

    reg_.SetSubtractionFlag(false);
    reg_.SetHalfCarryFlag(false);
    reg_.SetCarryFlag(msbSet);
    mCycle_ = 0;
}

void CPU::RL(uint8_t* reg, bool prefix)
{
    bool msbSet = (*reg & 0x80) == 0x80;
    *reg <<= 1;
    *reg |= reg_.IsCarryFlagSet() ? 0x01 : 0x00;

    if (prefix)
    {
        reg_.SetZeroFlag(*reg == 0x00);
    }
    else
    {
        reg_.SetZeroFlag(false);
    }

    reg_.SetSubtractionFlag(false);
    reg_.SetHalfCarryFlag(false);
    reg_.SetCarryFlag(msbSet);
    mCycle_ = 0;
}

void CPU::RRC(uint8_t* reg, bool prefix)
{
    bool lsbSet = (*reg & 0x01) == 0x01;
    *reg >>= 1;
    *reg |= lsbSet ? 0x80 : 0x00;

    if (prefix)
    {
        reg_.SetZeroFlag(*reg == 0x00);
    }
    else
    {
        reg_.SetZeroFlag(false);
    }

    reg_.SetSubtractionFlag(false);
    reg_.SetHalfCarryFlag(false);
    reg_.SetCarryFlag(lsbSet);
    mCycle_ = 0;
}

void CPU::RR(uint8_t* reg, bool prefix)
{
    bool lsbSet = (*reg & 0x01) == 0x01;
    *reg >>= 1;
    *reg |= reg_.IsCarryFlagSet() ? 0x80 : 0x00;

    if (prefix)
    {
        reg_.SetZeroFlag(*reg == 0x00);
    }
    else
    {
        reg_.SetZeroFlag(false);
    }

    reg_.SetSubtractionFlag(false);
    reg_.SetHalfCarryFlag(false);
    reg_.SetCarryFlag(lsbSet);
    mCycle_ = 0;
}

void CPU::RLCMem()
{
    switch (mCycle_)
    {
        case 3:
            cmdData8_ = Read(reg_.HL);
            break;
        case 4:
            RLC(&cmdData8_, true);
            Write(reg_.HL, cmdData8_);
            break;
    }
}

void CPU::RLMem()
{
    switch (mCycle_)
    {
        case 3:
            cmdData8_ = Read(reg_.HL);
            break;
        case 4:
            RL(&cmdData8_, true);
            Write(reg_.HL, cmdData8_);
            break;
    }
}

void CPU::RRCMem()
{
    switch (mCycle_)
    {
        case 3:
            cmdData8_ = Read(reg_.HL);
            break;
        case 4:
            RRC(&cmdData8_, true);
            Write(reg_.HL, cmdData8_);
            break;
    }
}

void CPU::RRMem()
{
    switch (mCycle_)
    {
        case 3:
            cmdData8_ = Read(reg_.HL);
            break;
        case 4:
            RR(&cmdData8_, true);
            Write(reg_.HL, cmdData8_);
            break;
    }
}

void CPU::SLA(uint8_t* reg)
{
    reg_.SetCarryFlag((*reg & 0x80) == 0x80);
    *reg <<= 1;
    reg_.SetZeroFlag(*reg == 0x00);
    reg_.SetSubtractionFlag(false);
    reg_.SetHalfCarryFlag(false);
    mCycle_ = 0;
}

void CPU::SRA(uint8_t* reg)
{
    reg_.SetCarryFlag((*reg & 0x01) == 0x01);
    bool msbSet = (*reg & 0x80) == 0x80;
    *reg >>= 1;
    *reg |= msbSet ? 0x80 : 0x00;
    reg_.SetZeroFlag(*reg == 0);
    reg_.SetSubtractionFlag(false);
    reg_.SetHalfCarryFlag(false);
    mCycle_ = 0;
}

void CPU::SRL(uint8_t* reg)
{
    reg_.SetCarryFlag((*reg & 0x01) == 0x01);
    *reg >>= 1;
    reg_.SetZeroFlag(*reg == 0);
    reg_.SetSubtractionFlag(false);
    reg_.SetHalfCarryFlag(false);
    mCycle_ = 0;
}

void CPU::SLAMem()
{
    switch (mCycle_)
    {
        case 3:
            cmdData8_ = Read(reg_.HL);
            break;
        case 4:
            SLA(&cmdData8_);
            Write(reg_.HL, cmdData8_);
            break;
    }
}

void CPU::SRAMem()
{
    switch (mCycle_)
    {
        case 3:
            cmdData8_ = Read(reg_.HL);
            break;
        case 4:
            SRA(&cmdData8_);
            Write(reg_.HL, cmdData8_);
            break;
    }
}

void CPU::SRLMem()
{
    switch (mCycle_)
    {
        case 3:
            cmdData8_ = Read(reg_.HL);
            break;
        case 4:
            SRL(&cmdData8_);
            Write(reg_.HL, cmdData8_);
            break;
    }
}

void CPU::Bit(uint8_t reg, uint8_t bit)
{
    uint8_t mask = 0x01 << bit;
    reg_.SetZeroFlag((reg & mask) == 0x00);
    reg_.SetSubtractionFlag(false);
    reg_.SetHalfCarryFlag(true);
    mCycle_ = 0;
}

void CPU::BitMem(uint8_t bit)
{
    cmdData8_ = Read(reg_.HL);
    Bit(cmdData8_, bit);
}

void CPU::Set(uint8_t* destReg, uint8_t bit)
{
    uint8_t mask = 0x01 << bit;
    *destReg |= mask;
    mCycle_ = 0;
}

void CPU::SetMem(uint8_t bit)
{
    switch (mCycle_)
    {
        case 3:
            cmdData8_ = Read(reg_.HL);
            break;
        case 4:
            Set(&cmdData8_, bit);
            Write(reg_.HL, cmdData8_);
            break;
    }
}

void CPU::Res(uint8_t* destReg, uint8_t bit)
{
    uint8_t mask = 0x01 << bit;
    *destReg &= ~mask;
    mCycle_ = 0;
}

void CPU::ResMem(uint8_t bit)
{
    switch (mCycle_)
    {
        case 3:
            cmdData8_ = Read(reg_.HL);
            break;
        case 4:
            Res(&cmdData8_, bit);
            Write(reg_.HL, cmdData8_);
            break;
    }
}

void CPU::JumpToAbsolute(bool condition)
{
    switch (mCycle_)
    {
        case 2:
            cmdData16_ = ReadPC();
            break;
        case 3:
            cmdData16_ = (ReadPC() << 8) | cmdData16_;

            if (!condition)
            {
                mCycle_ = 0;
            }

            break;
        case 4:
            reg_.PC = cmdData16_;
            mCycle_ = 0;
            break;
    }
}

void CPU::JumpToRelative(bool condition)
{
    switch (mCycle_)
    {
        case 2:
        {
            cmdData8_ = ReadPC();
            int8_t signedImmediate = cmdData8_;
            cmdData16_ = reg_.PC + signedImmediate;

            if (!condition)
            {
                mCycle_ = 0;
            }

            break;
        }
        case 3:
            reg_.PC = cmdData16_;
            mCycle_ = 0;
            break;
    }
}

void CPU::Call(bool condition)
{
    switch (mCycle_)
    {
        case 2:
            cmdData16_ = ReadPC();
            break;
        case 3:
            cmdData16_ = (ReadPC() << 8) | cmdData16_;

            if (!condition)
            {
                mCycle_ = 0;
            }

            break;
        case 4:
            Push(reg_.PC >> 8);
            break;
        case 5:
            Push(reg_.PC & 0x00FF);
            break;
        case 6:
            reg_.PC = cmdData16_;
            mCycle_ = 0;
            break;
    }
}

void CPU::Restart(uint8_t addr)
{
    switch (mCycle_)
    {
        case 2:
            Push(reg_.PC >> 8);
            break;
        case 3:
            Push(reg_.PC & 0x00FF);
            break;
        case 4:
            reg_.PC = addr;
            mCycle_ = 0;
            break;
    }
}

void CPU::Return(bool enableInterrupts)
{
    switch (mCycle_)
    {
        case 2:
            cmdData16_ = Pop();
            break;
        case 3:
            cmdData16_ = (Pop() << 8) | cmdData16_;
            break;
        case 4:
            if (enableInterrupts)
            {
                interruptsEnabled_ = true;
            }

            reg_.PC = cmdData16_;
            mCycle_ = 0;
            break;
    }
}

void CPU::ReturnConditional(bool condition)
{
    switch (mCycle_)
    {
        case 2:
            if (!condition)
            {
                mCycle_ = 0;
            }

            break;
        case 3:
            cmdData16_ = Pop();
            break;
        case 4:
            cmdData16_ = (Pop() << 8) | cmdData16_;
            break;
        case 5:
            reg_.PC = cmdData16_;
            mCycle_ = 0;
            break;
    }
}
