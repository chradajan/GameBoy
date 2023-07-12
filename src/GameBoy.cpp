#include <GameBoy.hpp>
#include <Cartridge/Cartridge.hpp>
#include <Cartridge/MBC0.hpp>
#include <Cartridge/MBC1.hpp>
#include <Cartridge/MBC5.hpp>
#include <Controller.hpp>
#include <Paths.hpp>
#include <algorithm>
#include <array>
#include <functional>
#include <iostream>

GameBoy::GameBoy(std::array<uint8_t, FRAME_BUFFER_SIZE>& frameBuffer) :
    frameBuffer_(frameBuffer),
    cpu_(std::bind(&GameBoy::Read, this, std::placeholders::_1),
         std::bind(&GameBoy::Write, this, std::placeholders::_1, std::placeholders::_2)),
    ppu_({ioReg_[IO::LCDC], ioReg_[IO::STAT], ioReg_[IO::SCY], ioReg_[IO::SCX], ioReg_[IO::LY], ioReg_[IO::LYC], ioReg_[IO::WY], ioReg_[IO::WX]},
         {ioReg_[IO::BGP], ioReg_[IO::OBP0], ioReg_[IO::OBP1], BG_CRAM_, OBJ_CRAM_},
         {ioReg_[IO::OPRI], OAM_},
         VRAM_,
         cgbMode_,
         frameBuffer.data()),
    cartridge_(nullptr)
{
}

void GameBoy::Run()
{
    if (!cartridge_ && !runningBootRom_)
    {
        return;
    }

    while (!ppu_.FrameReady())
    {
        RunMCycle();
    }

    if (!ppu_.LCDEnabled())
    {
        frameBuffer_.fill(0xFF);
    }
}

void GameBoy::Reset()
{
    if (!runningBootRom_)
    {
        for (auto& bank : VRAM_)
        {
            bank.fill(0x00);
        }

        for (auto& bank : WRAM_)
        {
            bank.fill(0x00);
        }

        OAM_.fill(0x00);
        HRAM_.fill(0x00);
        BG_CRAM_.fill(0x00);
        OBJ_CRAM_.fill(0x00);

        ioReg_.fill(0x00);
        ioReg_[IO::JOYP] = 0xCF;
        ioReg_[IO::SC] = 0x7F;
        ioReg_[IO::TAC] = 0xF8;
        ioReg_[IO::IF] = 0xE1;
        ioReg_[IO::NR10] = 0x80;
        ioReg_[IO::NR11] = 0xBF;
        ioReg_[IO::NR12] = 0xF3;
        ioReg_[IO::NR13] = 0xFF;
        ioReg_[IO::NR14] = 0xBF;
        ioReg_[IO::NR21] = 0x3F;
        ioReg_[IO::NR23] = 0xFF;
        ioReg_[IO::NR24] = 0xBF;
        ioReg_[IO::NR30] = 0x7F;
        ioReg_[IO::NR31] = 0xFF;
        ioReg_[IO::NR32] = 0x9F;
        ioReg_[IO::NR33] = 0xFF;
        ioReg_[IO::NR34] = 0xBF;
        ioReg_[IO::NR41] = 0xFF;
        ioReg_[IO::NR44] = 0xBF;
        ioReg_[IO::NR50] = 0x77;
        ioReg_[IO::NR51] = 0xF3;
        ioReg_[IO::NR52] = 0xF1;
        ioReg_[IO::LCDC] = 0x91;
        ioReg_[IO::BGP] = 0xFC;
        ioReg_[IO::KEY1] = 0xFF;
        ioReg_[IO::VBK] = 0xFF;
        ioReg_[IO::HDMA1] = 0xFF;
        ioReg_[IO::HDMA2] = 0xFF;
        ioReg_[IO::HDMA3] = 0xFF;
        ioReg_[IO::HDMA4] = 0xFF;
        ioReg_[IO::HDMA5] = 0xFF;
        ioReg_[IO::SVBK] = 0xFF;
    }
    else
    {
        ioReg_.fill(0x00);
    }

    serialOutData_ = 0x00;
    serialBitsSent_ = 0;
    serialTransferInProgress_ = false;

    divCounter_ = 0;
    timerCounter_ = 0;
    timerControl_ = 0;
    timerEnabled_ = false;
    timerReload_ = false;

    oamDmaInProgress_ = false;
    oamDmaCyclesRemaining_ = 0;
    oamDmaSrcAddr_ = 0x0000;
    oamIndexDest_ = 0;

    vramDmaSrcAddr_ = 0x0000;
    vramDmaDestAddr_ = 0x0000;
    gdmaInProgress_ = false;
    hdmaInProgress_ = false;
    gdmaBytesRemaining_ = 0;
    hdmaBytesRemaining_ = 0;
    hdmaBlocksRemaining_ = 0;
    hdmaLy_ = 0xFF;

    lastPendingInterrupt_ = 0x00;
    prevStatState_ = false;

    cpu_.Reset(runningBootRom_);
    ppu_.Reset();
}

void GameBoy::InsertCartridge(fs::path romPath)
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
    std::string title(reinterpret_cast<char*>(&bank0[0x0134]), titleLength);
    fs::path savePath = SAVE_PATH.string() + title + ".sav";

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
    cgbMode_ = true;

    std::ifstream bootROM(BOOT_ROM_PATH, std::ios::binary);

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

    Reset();
}

bool GameBoy::FrameReady()
{
    return ppu_.FrameReady();
}

void GameBoy::SetInputs(Buttons const& buttons)
{
    uint_fast8_t prevState = ioReg_[IO::JOYP] & 0x0F;
    bool const actionSelect = ((ioReg_[IO::JOYP] & 0x20) == 0);
    bool const directionSelect = ((ioReg_[IO::JOYP] & 0x10) == 0);
    ioReg_[IO::JOYP] |= 0x0F;

    if (actionSelect)
    {
        if (buttons.start)
        {
            ioReg_[IO::IF] |= (prevState & 0x08) ? INT_SRC::JOYPAD : 0x00;
            ioReg_[IO::JOYP] &= 0x07;
        }

        if (buttons.select)
        {
            ioReg_[IO::IF] |= (prevState & 0x04) ? INT_SRC::JOYPAD : 0x00;
            ioReg_[IO::JOYP] &= 0x0B;
        }

        if (buttons.b)
        {
            ioReg_[IO::IF] |= (prevState & 0x02) ? INT_SRC::JOYPAD : 0x00;
            ioReg_[IO::JOYP] &= 0x0D;
        }

        if (buttons.a)
        {
            ioReg_[IO::IF] |= (prevState & 0x01) ? INT_SRC::JOYPAD : 0x00;
            ioReg_[IO::JOYP] &= 0x0E;
        }
    }

    if (directionSelect)
    {
        if (buttons.down)
        {
            ioReg_[IO::IF] |= (prevState & 0x08) ? INT_SRC::JOYPAD : 0x00;
            ioReg_[IO::JOYP] &= 0x07;
        }

        if (buttons.up)
        {
            ioReg_[IO::IF] |= (prevState & 0x04) ? INT_SRC::JOYPAD : 0x00;
            ioReg_[IO::JOYP] &= 0x0B;
        }

        if (buttons.left)
        {
            ioReg_[IO::IF] |= (prevState & 0x02) ? INT_SRC::JOYPAD : 0x00;
            ioReg_[IO::JOYP] &= 0x0D;
        }

        if (buttons.right)
        {
            ioReg_[IO::IF] |= (prevState & 0x01) ? INT_SRC::JOYPAD : 0x00;
            ioReg_[IO::JOYP] &= 0x0E;
        }
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
    bool currStatState = false;

    // Check for LYC=LY interrupt
    if ((ioReg_[IO::STAT] & 0x44) == 0x44)
    {
        currStatState = true;
    }
    else
    {
        // Check for Mode interrupt
        uint_fast8_t ppuMode = (ioReg_[IO::STAT] & 0x03);

        switch (ppuMode)
        {
            case 0:
                currStatState = (ioReg_[IO::STAT] & 0x08);
                break;
            case 1:
                currStatState = (ioReg_[IO::STAT] & 0x10);
                break;
            case 2:
                currStatState = (ioReg_[IO::STAT] & 0x20);
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
