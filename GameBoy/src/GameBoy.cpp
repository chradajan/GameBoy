#include <GameBoy.hpp>
#include <Cartridge/Cartridge.hpp>
#include <Cartridge/MBC0.hpp>
#include <Cartridge/MBC1.hpp>
#include <Cartridge/MBC3.hpp>
#include <Cartridge/MBC5.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <utility>

GameBoy::GameBoy() :
    cgbMode_(false),
    cpu_(std::bind(&GameBoy::Read, this, std::placeholders::_1),
         std::bind(&GameBoy::Write, this, std::placeholders::_1, std::placeholders::_2),
         std::bind(&GameBoy::AcknowledgeInterrupt, this),
         std::bind(&GameBoy::Stop, this, std::placeholders::_1)),
    ppu_(cgbMode_),
    cartridge_(nullptr)
{
}

void GameBoy::Initialize(uint8_t* frameBuffer,
                         std::filesystem::path const savePath,
                         std::filesystem::path const bootRomPath)
{
    frameBuffer_ = frameBuffer;
    saveDirectory_ = savePath;
    bootRomPath_ = bootRomPath;
    ppu_.SetFrameBuffer(frameBuffer);
}

void GameBoy::InsertCartridge(std::filesystem::path const romPath)
{
    if (cartridge_)
    {
        cartridge_.reset();
    }

    std::array<uint8_t, 0x4000> bank0;
    std::ifstream rom(romPath, std::ios::binary);
    rom.read(reinterpret_cast<char*>(bank0.data()), sizeof(bank0[0]) * bank0.size());
    uint8_t cartridgeType = bank0[0x0147];
    cgbCartridge_ = (bank0[0x143] & 0x80) == 0x80;

    uint8_t titleLength = cgbCartridge_ ? 15 : 16;
    std::stringstream title;

    for (uint_fast8_t i = 0; i < titleLength; ++i)
    {
        if (bank0[0x0134 + i] == 0x00)
        {
            continue;
        }

        title << static_cast<char>(bank0[0x0134 + i]);
    }

    std::filesystem::path savePath;

    if (!saveDirectory_.empty())
    {
        savePath = saveDirectory_ / (title.str() + ".sav");
    }

    uint16_t romBanks = 2 << bank0[0x0148];
    uint8_t ramBanks;

    switch (bank0[0x0149])
    {
        case 0x00:
        case 0x01:
            ramBanks = 0;
            break;
        case 0x02:
            ramBanks = 1;
            break;
        case 0x03:
            ramBanks = 4;
            break;
        case 0x04:
            ramBanks = 16;
            break;
        case 0x05:
            ramBanks = 8;
            break;
        default:
            ramBanks = 0;
            break;
    }

    switch (cartridgeType)
    {
        case 0x00:
        case 0x08:
        case 0x09:
            cartridge_ = std::make_unique<MBC0>(bank0, rom, savePath, cartridgeType, ramBanks);
            break;
        case 0x01 ... 0x03:
            cartridge_ = std::make_unique<MBC1>(bank0, rom, savePath, cartridgeType, romBanks, ramBanks);
            break;
        case 0x0F ... 0x13:
            cartridge_ = std::make_unique<MBC3>(bank0, rom, savePath, cartridgeType, romBanks, ramBanks);
            break;
        case 0x19 ... 0x1E:
            cartridge_ = std::make_unique<MBC5>(bank0, rom, savePath, cartridgeType, romBanks, ramBanks);
            break;
        default:
            cartridge_ = nullptr;
            break;
    }
}

void GameBoy::PowerOn()
{
    if (cartridge_)
    {
        cartridge_->SaveRAM();
    }

    cgbMode_ = true;
    std::ifstream bootROM(bootRomPath_, std::ios::binary);

    if (bootROM.fail())
    {
        runningBootRom_ = false;
        cgbMode_ = cgbCartridge_;
        ppu_.SetDmgColorMode(true);
    }
    else
    {
        bootROM.read(reinterpret_cast<char*>(BOOT_ROM.data()), BOOT_ROM.size());
        runningBootRom_ = true;
        ppu_.SetDmgColorMode(false);
    }

    for (auto& bank : WRAM_)
    {
        bank.fill(0x00);
    }

    HRAM_.fill(0x00);

    if (runningBootRom_)
    {
        ioReg_.fill(0x00);
    }
    else
    {
        SetDefaultCgbIoValues();
    }

    stopped_ = false;

    speedSwitchCountdown_ = 0;

    serialOutData_ = 0x00;
    serialBitsSent_ = 0;
    serialTransferCounter_ = 0;
    serialClockDivider_ = 128;
    serialTransferInProgress_ = false;

    timerCounter_ = 0;
    timerControl_ = 0;
    timerEnabled_ = false;
    timerReload_ = false;

    oamDmaInProgress_ = false;
    oamDmaCyclesRemaining_ = 0;
    oamDmaSrcAddr_ = 0x0000;
    oamDmaDestAddr_ = 0x0000;

    wasMode0_ = false;
    gdmaInProgress_ = false;
    hdmaInProgress_ = false;
    transferActive_ = false;

    lastPendingInterrupt_ = 0x00;
    prevStatState_ = false;

    apu_.PowerOn(runningBootRom_);
    cpu_.Reset(runningBootRom_);
    ppu_.PowerOn(!runningBootRom_);
}

bool GameBoy::IsSerializable() const
{
    return (!runningBootRom_ &&
            !serialTransferInProgress_ &&
            !oamDmaInProgress_ &&
            !gdmaInProgress_ &&
            !hdmaInProgress_ &&
            (speedSwitchCountdown_ == 0) &&
            cpu_.IsSerializable() &&
            ppu_.IsSerializable());
}

void GameBoy::Serialize(std::ofstream& out)
{
    out.write(reinterpret_cast<char*>(&buttons_), sizeof(buttons_));

    for (auto& bank : WRAM_)
    {
        out.write(reinterpret_cast<char*>(bank.data()), bank.size());
    }

    out.write(reinterpret_cast<char*>(HRAM_.data()), HRAM_.size());
    out.write(reinterpret_cast<char*>(ioReg_.data()), ioReg_.size());

    out.write(reinterpret_cast<char*>(&IE_), sizeof(IE_));

    out.write(reinterpret_cast<char*>(&timerCounter_), sizeof(timerCounter_));
    out.write(reinterpret_cast<char*>(&timerControl_), sizeof(timerControl_));
    out.write(reinterpret_cast<char*>(&timerEnabled_), sizeof(timerEnabled_));
    out.write(reinterpret_cast<char*>(&timerReload_), sizeof(timerReload_));

    out.write(reinterpret_cast<char*>(&wasMode0_), sizeof(wasMode0_));

    out.write(reinterpret_cast<char*>(&lastPendingInterrupt_), sizeof(lastPendingInterrupt_));
    out.write(reinterpret_cast<char*>(&prevStatState_), sizeof(prevStatState_));

    cartridge_->Serialize(out);
    apu_.Serialize(out);
    cpu_.Serialize(out);
    ppu_.Serialize(out);
}

void GameBoy::Deserialize(std::ifstream& in)
{
    in.read(reinterpret_cast<char*>(&buttons_), sizeof(buttons_));

    for (auto& bank : WRAM_)
    {
        in.read(reinterpret_cast<char*>(bank.data()), bank.size());
    }

    in.read(reinterpret_cast<char*>(HRAM_.data()), HRAM_.size());
    in.read(reinterpret_cast<char*>(ioReg_.data()), ioReg_.size());

    in.read(reinterpret_cast<char*>(&IE_), sizeof(IE_));

    in.read(reinterpret_cast<char*>(&timerCounter_), sizeof(timerCounter_));
    in.read(reinterpret_cast<char*>(&timerControl_), sizeof(timerControl_));
    in.read(reinterpret_cast<char*>(&timerEnabled_), sizeof(timerEnabled_));
    in.read(reinterpret_cast<char*>(&timerReload_), sizeof(timerReload_));

    in.read(reinterpret_cast<char*>(&wasMode0_), sizeof(wasMode0_));

    in.read(reinterpret_cast<char*>(&lastPendingInterrupt_), sizeof(lastPendingInterrupt_));
    in.read(reinterpret_cast<char*>(&prevStatState_), sizeof(prevStatState_));

    cartridge_->Deserialize(in);
    apu_.Deserialize(in);
    cpu_.Deserialize(in);
    ppu_.Deserialize(in);
}

void GameBoy::Clock(int const numCycles)
{
    if (!cartridge_ && !runningBootRom_)
    {
        return;
    }

    RunMCycles(numCycles);
}

void GameBoy::UpdateJOYP(uint8_t data)
{
    uint_fast8_t const prevState = ioReg_[IO::JOYP] & 0x0F;
    ioReg_[IO::JOYP] = data | 0xCF;
    bool const actionSelect = (ioReg_[IO::JOYP] & 0x20) == 0;
    bool const directionSelect = (ioReg_[IO::JOYP] & 0x10) == 0;

    if (actionSelect)
    {
        if (buttons_.start)
        {
            ioReg_[IO::IF] |= (prevState & 0x08) ? INT_SRC::JOYPAD : 0x00;
            ioReg_[IO::JOYP] &= 0x07;
        }

        if (buttons_.select)
        {
            ioReg_[IO::IF] |= (prevState & 0x04) ? INT_SRC::JOYPAD : 0x00;
            ioReg_[IO::JOYP] &= 0x0B;
        }

        if (buttons_.b)
        {
            ioReg_[IO::IF] |= (prevState & 0x02) ? INT_SRC::JOYPAD : 0x00;
            ioReg_[IO::JOYP] &= 0x0D;
        }

        if (buttons_.a)
        {
            ioReg_[IO::IF] |= (prevState & 0x01) ? INT_SRC::JOYPAD : 0x00;
            ioReg_[IO::JOYP] &= 0x0E;
        }
    }

    if (directionSelect)
    {
        if (buttons_.down)
        {
            ioReg_[IO::IF] |= (prevState & 0x08) ? INT_SRC::JOYPAD : 0x00;
            ioReg_[IO::JOYP] &= 0x07;
        }

        if (buttons_.up)
        {
            ioReg_[IO::IF] |= (prevState & 0x04) ? INT_SRC::JOYPAD : 0x00;
            ioReg_[IO::JOYP] &= 0x0B;
        }

        if (buttons_.left)
        {
            ioReg_[IO::IF] |= (prevState & 0x02) ? INT_SRC::JOYPAD : 0x00;
            ioReg_[IO::JOYP] &= 0x0D;
        }

        if (buttons_.right)
        {
            ioReg_[IO::IF] |= (prevState & 0x01) ? INT_SRC::JOYPAD : 0x00;
            ioReg_[IO::JOYP] &= 0x0E;
        }
    }

    if (stopped_ && ((ioReg_[IO::JOYP] & 0x0F) != 0x0F))
    {
        stopped_ = false;
    }
}

std::optional<std::pair<uint16_t, uint8_t>> GameBoy::CheckPendingInterrupts()
{
    CheckVBlankInterrupt();
    CheckStatInterrupt();
    uint8_t pendingInterrupts = ioReg_[IO::IF] & IE_ & 0x1F;

    if (pendingInterrupts != 0x00)
    {
        uint16_t interruptAddr = 0x0000;
        uint8_t numPendingInterrupts = 0;

        if (pendingInterrupts & INT_SRC::JOYPAD)
        {
            interruptAddr = 0x0060;
            lastPendingInterrupt_ = INT_SRC::JOYPAD;
            ++numPendingInterrupts;
        }
        if (pendingInterrupts & INT_SRC::SERIAL)
        {
            interruptAddr = 0x0058;
            lastPendingInterrupt_ = INT_SRC::SERIAL;
            ++numPendingInterrupts;
        }
        if (pendingInterrupts & INT_SRC::TIMER)
        {
            interruptAddr = 0x0050;
            lastPendingInterrupt_ = INT_SRC::TIMER;
            ++numPendingInterrupts;
        }
        if (pendingInterrupts & INT_SRC::LCD_STAT)
        {
            interruptAddr = 0x0048;
            lastPendingInterrupt_ = INT_SRC::LCD_STAT;
            ++numPendingInterrupts;
        }
        if (pendingInterrupts & INT_SRC::VBLANK)
        {
            interruptAddr = 0x0040;
            lastPendingInterrupt_ = INT_SRC::VBLANK;
            ++numPendingInterrupts;
        }

        return std::make_pair(interruptAddr, numPendingInterrupts);
    }

    return {};
}

void GameBoy::AcknowledgeInterrupt()
{
    ioReg_[IO::IF] &= ~lastPendingInterrupt_;
}

void GameBoy::CheckVBlankInterrupt()
{
    if (ppu_.VBlank())
    {
        ioReg_[IO::IF] |= INT_SRC::VBLANK;
    }
}

void GameBoy::CheckStatInterrupt()
{
    if (!ppu_.LCDEnabled())
    {
        return;
    }

    bool currStatState = false;
    uint_fast8_t STAT = ppu_.STAT();

    // Check for LYC=LY interrupt
    if ((STAT & 0x44) == 0x44)
    {
        currStatState = true;
    }
    else
    {
        // Check for Mode interrupt
        uint_fast8_t ppuMode = (STAT & 0x03);

        switch (ppuMode)
        {
            case 0:
                currStatState = (STAT & 0x08);
                break;
            case 1:
                currStatState = (STAT & 0x10);
                break;
            case 2:
                currStatState = (STAT & 0x20);
                break;
            default:
                break;
        }
    }

    if (!prevStatState_ && currStatState)
    {
        ioReg_[IO::IF] |= INT_SRC::LCD_STAT;
    }

    prevStatState_ = currStatState;
}

std::pair<bool, bool> GameBoy::Stop(bool const IME)
{
    bool const buttonsPressed = (ioReg_[IO::JOYP] & 0x0F) != 0x0F;
    bool const interruptPending = (ioReg_[IO::IF] & IE_ & 0x1F) != 0x00;
    bool twoByteOpcode = false;
    bool enterHaltMode = false;

    if (buttonsPressed)
    {
        if (interruptPending)
        {
            twoByteOpcode = false;
            enterHaltMode = false;
        }
        else
        {
            twoByteOpcode = true;
            enterHaltMode = true;
        }
    }
    else if (PrepareSpeedSwitch())
    {
        if (interruptPending)
        {
            if (IME)
            {
                // CPU glitches non-deterministically. Just treat as a 2-byte opcode and hope for the best.
                twoByteOpcode = true;
                enterHaltMode = false;
            }
            else
            {
                SwitchSpeedMode();
                ioReg_[IO::KEY1] &= 0xFE;
                apu_.ResetDIV(DoubleSpeedMode());
                twoByteOpcode = false;
                enterHaltMode = false;
            }
        }
        else
        {
            // Good path for speed switch.
            apu_.ResetDIV(DoubleSpeedMode());
            SwitchSpeedMode();
            ioReg_[IO::KEY1] &= 0xFE;
            speedSwitchCountdown_ = 2050;
            twoByteOpcode = true;
            enterHaltMode = true;
        }
    }
    else
    {
        apu_.ResetDIV(DoubleSpeedMode());
        twoByteOpcode = !interruptPending;
        enterHaltMode = false;
        stopped_ = true;
    }

    return {twoByteOpcode, enterHaltMode};
}
