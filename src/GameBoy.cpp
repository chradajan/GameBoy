#include <GameBoy.hpp>
#include <Paths.hpp>
#include <array>
#include <functional>
#include <iostream>

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
    BANK = 0x50, // Boot ROM disable

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

/// @brief Enum of interrupt source masks.
namespace INT_SRC
{
enum : uint8_t
{
    VBLANK = 0x01,
    LCD_STAT = 0x02,
    TIMER = 0x04,
    SERIAL = 0x08,
    JOYPAD = 0x10,
};
}  // namespace INT_SRC

GameBoy::GameBoy(uint8_t* const frameBuffer) :
    cpu_(std::bind(&GameBoy::Read, this, std::placeholders::_1),
         std::bind(&GameBoy::Write, this, std::placeholders::_1, std::placeholders::_2)),
    ppu_({ioReg_[IO::LCDC], ioReg_[IO::STAT], ioReg_[IO::SCY], ioReg_[IO::SCX], ioReg_[IO::LY], ioReg_[IO::LYC], ioReg_[IO::WY], ioReg_[IO::WX]},
         {ioReg_[IO::BGP], ioReg_[IO::OBP0], ioReg_[IO::OBP1], BG_CRAM_, OBJ_CRAM_},
         {ioReg_[IO::OPRI], OAM_},
         VRAM_,
         cgbMode_,
         frameBuffer),
    cartridge_(nullptr)
{
}

void GameBoy::Run()
{
    if (!cartridge_)
    {
        return;
    }

    while (!ppu_.FrameReady())
    {
        if (cpu_.Clock(CheckPendingInterrupts()))
        {
            AcknowledgeInterrupt();
        }

        for (size_t i = 0; i < 4; ++i)
        {
            ppu_.Clock(oamDmaInProgress_);
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
}

void GameBoy::Reset()
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
    ioReg_[IO::VBK] = 0x00;
    ioReg_[IO::HDMA1] = 0xFF;
    ioReg_[IO::HDMA2] = 0xFF;
    ioReg_[IO::HDMA3] = 0xFF;
    ioReg_[IO::HDMA4] = 0xFF;
    ioReg_[IO::HDMA5] = 0xFF;
    ioReg_[IO::SVBK] = 0xFF;

    cgbMode_ = false;

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

    lastPendingInterrupt_ = 0x00;
    prevStatState_ = false;

    cpu_.Reset();
    cartridge_->Reset();
}

void GameBoy::InsertCartridge(fs::path romPath)
{
    if (cartridge_)
    {
        cartridge_.reset();
    }

    std::array<uint8_t, 0x4000> bank0;
    std::ifstream rom(romPath, std::ios::binary);
    rom.read((char*)bank0.data(), sizeof(bank0[0]) * bank0.size());
    uint8_t cartridgeType = bank0[0x0147];
    cgbMode_ = (bank0[0x143] & 0x80) == 0x80;

    uint8_t titleLength = cgbMode_ ? 15 : 16;
    std::string title((char*)&bank0[0x0134], titleLength);
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
            cartridge_ = std::make_unique<MBC0>(bank0, rom, savePath, cartridgeType);
            break;
        case 0x01:
        case 0x02:
        case 0x03:
            cartridge_ = std::make_unique<MBC1>(bank0, rom, savePath, cartridgeType, romBanks, ramBanks);
            break;
        default:
            cartridge_ = nullptr;
            break;
    }

    Reset();
    cpu_.Reset();
}

bool GameBoy::FrameReady()
{
    return ppu_.FrameReady();
}

void GameBoy::ClockTimer()
{
    ++divCounter_;

    if (divCounter_ == 64)
    {
        ++ioReg_[IO::DIV];
        divCounter_ = 0;
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
            timerCounter_ = 0x0000;
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

void GameBoy::ClockSerialTransfer()
{
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

std::optional<std::pair<uint16_t, uint8_t>> GameBoy::CheckPendingInterrupts()
{
    CheckVBlankInterrupt();
    CheckStatInterrupt();
    uint8_t pendingInterrupts = ioReg_[IO::IF] & IE_;

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
    bool currStatState_ = false;

    // Check for LYC=LY interrupt
    if ((ioReg_[IO::STAT] & 0x44) == 0x44)
    {
        currStatState_ = true;
    }
    else
    {
        // Check for Mode interrupt
        uint_fast8_t ppuMode = (ioReg_[IO::STAT] & 0x03);

        switch (ppuMode)
        {
            case 0:
                currStatState_ = (ioReg_[IO::STAT] & 0x08);
                break;
            case 1:
                currStatState_ = (ioReg_[IO::STAT] & 0x10);
                break;
            case 2:
                currStatState_ = (ioReg_[IO::STAT] & 0x20);
                break;
            default:
                break;
        }
    }

    if (!prevStatState_ && currStatState_)
    {
        ioReg_[IO::IF] |= INT_SRC::LCD_STAT;
    }

    prevStatState_ = currStatState_;
}

uint8_t GameBoy::Read(uint16_t addr)
{
    if (addr < 0x8000)
    {
        // Cartridge ROM
        return cartridge_->ReadROM(addr);
    }
    else if (addr < 0xA000)
    {
        // VRAM
        if (ppu_.LCDEnabled() && (ppu_.GetMode() == 3))
        {
            return 0xFF;
        }

        return VRAM_[ioReg_[IO::VBK] & 0x01][addr - 0x8000];
    }
    else if (addr < 0xC000)
    {
        // Cartridge RAM
        return cartridge_->ReadRAM(addr);
    }
    else if (addr < 0xD000)
    {
        // WRAM Bank 0
        return WRAM_[0][addr - 0xC000];
    }
    else if (addr < 0xE000)
    {
        // WRAM Banks 1-7
        uint8_t ramBank = (ioReg_[IO::SVBK] == 0x00) ? 0x01 : (ioReg_[IO::SVBK] & 0x07);
        return WRAM_[ramBank][addr - 0xD000];
    }
    else if (addr < 0xFE00)
    {
        // ECHO RAM, prohibited, TODO
        return 0x00;
    }
    else if (addr < 0xFEA0)
    {
        if (ppu_.LCDEnabled() && ((ppu_.GetMode() == 2) || (ppu_.GetMode() == 3)))
        {
            return 0xFF;
        }

        return OAM_[addr - 0xFE00];
    }
    else if (addr < 0xFF00)
    {
        // Unusable, prohibited, TODO
        return 0x00;
    }
    else if (addr < 0xFF80)
    {
        // I/O Registers
        return ReadIoReg(addr);
    }
    else if (addr < 0xFFFF)
    {
        // HRAM
        return HRAM_[addr - 0xFF80];
    }
    else if (addr == 0xFFFF)
    {
        // Interrupt Enable Register
        return IE_;
    }

    return 0x00;
}

void GameBoy::Write(uint16_t addr, uint8_t data)
{
    if (addr < 0x8000)
    {
        // Cartridge ROM
        cartridge_->WriteROM(addr, data);
    }
    else if (addr < 0xA000)
    {
        // VRAM
        if (ppu_.LCDEnabled() && (ppu_.GetMode() == 3))
        {
            return;
        }

        VRAM_[ioReg_[IO::VBK] & 0x01][addr - 0x8000] = data;
    }
    else if (addr < 0xC000)
    {
        // Cartridge RAM
        cartridge_->WriteRAM(addr, data);
    }
    else if (addr < 0xD000)
    {
        // WRAM Bank 0
        WRAM_[0][addr - 0xC000] = data;
    }
    else if (addr < 0xE000)
    {
        // WRAM Banks 1-7
        uint8_t ramBank = (ioReg_[IO::SVBK] == 0x00) ? 0x01 : (ioReg_[IO::SVBK] & 0x07);
        WRAM_[ramBank][addr - 0xD000] = data;
    }
    else if (addr < 0xFE00)
    {
        // ECHO RAM, prohibited, TODO
    }
    else if (addr < 0xFEA0)
    {
        // OAM

        if (ppu_.LCDEnabled() && ((ppu_.GetMode() == 2) || (ppu_.GetMode() == 3)))
        {
            return;
        }

        OAM_[addr - 0xFE00] = data;
    }
    else if (addr < 0xFF00)
    {
        // Unusable, prohibited, TODO
    }
    else if (addr < 0xFF80)
    {
        // I/O Registers
        WriteIoReg(addr, data);
    }
    else if (addr < 0xFFFF)
    {
        // HRAM
        HRAM_[addr - 0xFF80] = data;
    }
    else if (addr == 0xFFFF)
    {
        // Interrupt Enable Register
        IE_ = data;
    }
}

uint8_t GameBoy::ReadIoReg(uint16_t addr)
{
    switch (addr & 0xFF)
    {
        case IO::BCPD:  // Background color palette data
            if (ppu_.LCDEnabled() && (ppu_.GetMode() == 3))
            {
                return 0xFF;
            }

            return BG_CRAM_[ioReg_[IO::BCPS] & 0x3F];
        case IO::OCPD:  // OBJ color palette data
            if (ppu_.LCDEnabled() && (ppu_.GetMode() == 3))
            {
                return 0xFF;
            }

            return OBJ_CRAM_[ioReg_[IO::OCPS] & 0x3F];
        case IO::KEY1:
            return 0xFF;
        default:
            break;
    }

    return ioReg_[addr & 0x00FF];
}

void GameBoy::WriteIoReg(uint16_t addr, uint8_t data)
{
    switch (addr & 0xFF)
    {
        case IO::SC:  // Serial transfer control
            ioReg_[IO::SC] = data;
            serialTransferInProgress_ = (data & 0x80) == 0x80;
            serialBitsSent_ = 0x00;
            break;
        case IO::DIV:  // Divider register
            ioReg_[IO::DIV] = 0x00;
            timerCounter_ = 0x0000;
            divCounter_ = 0x00;
            break;
        case IO::TIMA:  // Timer counter
            timerReload_ = false;
            ioReg_[IO::TIMA] = data;
            break;
        case IO::TAC:  // Timer control
        {
            IoWriteTAC(data);
            break;
        }
        case IO::STAT:  // STAT register
        {
            data &= 0x78;
            ioReg_[IO::STAT] &= 0x07;
            ioReg_[IO::STAT] |= data;
            break;
        }
        case IO::DMA:  // OAM DMA source address
            IoWriteDMA(data);
            break;
        case IO::BCPD:  // Background color palette data
            IoWriteBCPD(data);
            break;
        case IO::OCPD:  // OBJ color palette data
            IoWriteOCPD(data);
            break;
        default:
            ioReg_[addr & 0xFF] = data;
            break;
    }
}

void GameBoy::IoWriteTAC(uint8_t data)
{
    ioReg_[IO::TAC] = data;
    timerCounter_ = 0x0000;
    timerEnabled_ = (data & 0x04) == 0x04;
    uint8_t clockSelect = data & 0x03;

    switch (clockSelect)
    {
        case 0x00:
            timerControl_ = 256;
            break;
        case 0x01:
            timerControl_ = 4;
            break;
        case 0x02:
            timerControl_ = 16;
            break;
        case 0x03:
            timerControl_ = 64;
            break;
    }
}

void GameBoy::IoWriteDMA(uint8_t data)
{
    data %= 0xE0;

    if (data < 0x80)
    {
        oamDmaSrc_ = OamDmaSrc::CART_ROM;
    }
    else if (data < 0xA0)
    {
        oamDmaSrc_ = OamDmaSrc::VRAM;
    }
    else if (data < 0xC0)
    {
        oamDmaSrc_ = OamDmaSrc::CART_RAM;
    }
    else
    {
        oamDmaSrc_ = OamDmaSrc::WRAM;
    }

    ioReg_[IO::DMA] = data;
    oamDmaInProgress_ = true;
    oamDmaCyclesRemaining_ = 160;
    oamDmaSrcAddr_ = data << 8;
    oamIndexDest_ = 0;
}

void GameBoy::IoWriteBCPD(uint8_t data)
{
    if (ppu_.LCDEnabled() && (ppu_.GetMode() == 3))
    {
        return;
    }

    BG_CRAM_[ioReg_[IO::BCPS] & 0x3F] = data;

    if (ioReg_[IO::BCPS] & 0x80)
    {
        ioReg_[IO::BCPS] = (ioReg_[IO::BCPS] & 0xC0) | ((ioReg_[IO::BCPS] + 1) & 0x3F);
    }
}

void GameBoy::IoWriteOCPD(uint8_t data)
{
    if (ppu_.LCDEnabled() && (ppu_.GetMode() == 3))
    {
        return;
    }

    OBJ_CRAM_[ioReg_[IO::OCPS] & 0x3F] = data;

    if (ioReg_[IO::OCPS] & 0x80)
    {
        ioReg_[IO::OCPS] = (ioReg_[IO::OCPS] & 0xC0) | ((ioReg_[IO::OCPS] + 1) & 0x3F);
    }
}
