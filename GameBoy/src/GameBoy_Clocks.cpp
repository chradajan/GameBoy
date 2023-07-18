#include <GameBoy.hpp>
#include <iostream>

void GameBoy::RunMCycle()
{
    for (uint_fast8_t i = 0; i < 4; ++i)
    {
        switch (i)
        {
            case 0:
            {
                bool clockCpu = true;

                if (gdmaInProgress_)
                {
                    clockCpu = false;
                    ClockVramGDMA();
                }
                else if (hdmaInProgress_ && (hdmaBytesRemaining_ > 0))
                {
                    clockCpu = false;
                    ClockVramHDMA();
                }

                ClockVariableSpeedComponents(clockCpu);

                apu_.Clock();
                ppu_.Clock(oamDmaInProgress_);
                break;
            }
            case 1:
                ppu_.Clock(oamDmaInProgress_);
                break;
            case 2:
                if (DoubleSpeedMode())
                {
                    bool clockCpu = !(gdmaInProgress_ || (hdmaInProgress_ && (hdmaBytesRemaining_ > 0)));
                    ClockVariableSpeedComponents(clockCpu);
                }

                ppu_.Clock(oamDmaInProgress_);
                break;
            case 3:
            {
                ppu_.Clock(oamDmaInProgress_);

                if (hdmaInProgress_ && (ppu_.GetMode() == 0) && (ioReg_[IO::LY] != hdmaLy_) && (hdmaBytesRemaining_ == 0))
                {
                    hdmaBytesRemaining_ = 0x10;
                    hdmaLy_ = ioReg_[IO::LY];
                }
                break;
            }
        }
    }

    if (speedSwitchCountdown_ > 0)
    {
        --speedSwitchCountdown_;

        if (speedSwitchCountdown_ == 0)
        {
            cpu_.ExitHalt();
        }
    }
}

void GameBoy::ClockVariableSpeedComponents(bool const clockCpu)
{
    if (clockCpu)
    {
        cpu_.Clock(CheckPendingInterrupts());
    }

    if (serialTransferInProgress_)
    {
        ClockSerialTransfer();
    }

    if (oamDmaInProgress_)
    {
        ClockOamDma();
    }

    apu_.ClockDIV(DoubleSpeedMode());
    ClockTimer();
}

void GameBoy::ClockTimer()
{
    if (speedSwitchCountdown_ == 0)
    {
        apu_.ClockDIV(DoubleSpeedMode());
    }

    if (timerEnabled_)
    {
        ++timerCounter_;

        if (timerReload_)
        {
            timerReload_ = false;
            ioReg_[IO::TIMA] = ioReg_[IO::TMA];
            ioReg_[IO::IF] |= INT_SRC::TIMER;
        }
        else if (timerCounter_ == timerControl_)
        {
            timerCounter_ = 0;
            ++ioReg_[IO::TIMA];

            if (ioReg_[IO::TIMA] == 0x00)
            {
                timerReload_ = true;
            }
        }
    }
}

void GameBoy::ClockOamDma()
{
    OAM_[oamIndexDest_++] = Read(oamDmaSrcAddr_++);
    --oamDmaCyclesRemaining_;

    if (oamDmaCyclesRemaining_ == 0)
    {
        oamDmaInProgress_ = false;
    }
}

void GameBoy::ClockVramGDMA()
{
    uint_fast8_t xferByte = Read(vramDmaSrcAddr_++);
    Write(vramDmaDestAddr_++, xferByte);

    if (vramDmaDestAddr_ > 0x9FFF)
    {
        gdmaInProgress_ = false;
        ioReg_[IO::HDMA5] = 0xFF;
        return;
    }

    --gdmaBytesRemaining_;
    xferByte = Read(vramDmaSrcAddr_++);
    Write(vramDmaDestAddr_++, xferByte);

    if (vramDmaDestAddr_ > 0x9FFF)
    {
        gdmaInProgress_ = false;
        ioReg_[IO::HDMA5] = 0xFF;
        return;
    }

    --gdmaBytesRemaining_;

    if (gdmaBytesRemaining_ == 0)
    {
        gdmaInProgress_ = false;
        ioReg_[IO::HDMA5] = 0xFF;
    }
}

void GameBoy::ClockVramHDMA()
{
    uint_fast8_t xferByte = Read(vramDmaSrcAddr_++);
    Write(vramDmaDestAddr_++, xferByte);

    if (vramDmaDestAddr_ > 0x9FFF)
    {
        hdmaInProgress_ = false;
        ioReg_[IO::HDMA5] = 0xFF;
        return;
    }

    --hdmaBytesRemaining_;
    xferByte = Read(vramDmaSrcAddr_++);
    Write(vramDmaDestAddr_++, xferByte);

    if (vramDmaDestAddr_ > 0x9FFF)
    {
        hdmaInProgress_ = false;
        ioReg_[IO::HDMA5] = 0xFF;
        return;
    }

    --hdmaBytesRemaining_;

    if (hdmaBytesRemaining_ == 0)
    {
        --hdmaBlocksRemaining_;

        if (hdmaBlocksRemaining_ == 0)
        {
            hdmaInProgress_ = false;
            ioReg_[IO::HDMA5] = 0xFF;
        }
    }
}

void GameBoy::ClockSerialTransfer()
{
    ++serialTransferCounter_;

    if (serialTransferCounter_ == 128)
    {
        serialTransferCounter_ = 0;
        serialOutData_ <<= 1;
        serialOutData_ |= (ioReg_[IO::SB] & 0x80) >> 7;
        ioReg_[IO::SB] <<= 1;
        ioReg_[IO::SB] |= 0x01;
        ++serialBitsSent_;

        if (serialBitsSent_ == 8)
        {
            serialTransferInProgress_ = false;
            ioReg_[IO::SC] &= 0x7F;
            ioReg_[IO::IF] |= INT_SRC::SERIAL;

            #ifdef PRINT_SERIAL
                std::cout << (char)serialOutData_;
            #endif
        }
    }
}
