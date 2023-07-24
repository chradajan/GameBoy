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
                if (cpu_.InBetweenInstructions() && (gdmaInProgress_ || (hdmaInProgress_ && vramDmaBytesRemaining_)))
                {
                    transferActive_ = true;
                    ClockVramDma();
                }

                ClockVariableSpeedComponents(!transferActive_);

                apu_.Clock();
                ppu_.Clock();
                break;
            }
            case 1:
                ppu_.Clock();
                break;
            case 2:
                if (DoubleSpeedMode())
                {
                    ClockVariableSpeedComponents(!transferActive_);
                }

                ppu_.Clock();
                break;
            case 3:
            {
                ppu_.Clock();
                break;
            }
        }
    }

    bool isMode0 = (ppu_.GetMode() == 0);

    if (hdmaInProgress_ && (!wasMode0_ && isMode0))
    {
        vramDmaBytesRemaining_ = 0x10;
    }

    wasMode0_ = isMode0;

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
    ppu_.Write(oamDmaDestAddr_++, Read(oamDmaSrcAddr_++), true);
    --oamDmaCyclesRemaining_;

    if (oamDmaCyclesRemaining_ == 0)
    {
        oamDmaInProgress_ = false;
    }
}

void GameBoy::ClockVramDma()
{
    Write(vramDmaDest_++, Read(vramDmaSrc_++));
    Write(vramDmaDest_++, Read(vramDmaSrc_++));
    vramDmaBytesRemaining_ -= 2;

    if (vramDmaBytesRemaining_ == 0)
    {
        if (gdmaInProgress_)
        {
            gdmaInProgress_ = false;
            ioReg_[IO::HDMA5] = 0xFF;
            transferActive_ = false;
        }
        else
        {
            --vramDmaBlocksRemaining_;
            transferActive_ = false;

            if (vramDmaBlocksRemaining_ == 0)
            {
                hdmaInProgress_ = false;
                ioReg_[IO::HDMA5] = 0xFF;
            }
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
