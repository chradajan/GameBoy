#include <GameBoy.hpp>
#include <Paths.hpp>
#include <array>
#include <functional>
#include <iostream>

namespace IO_REG
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

GameBoy::GameBoy() :
    cpu_(std::bind(&GameBoy::Read, this, RW_SRC::CPU, std::placeholders::_1),
         std::bind(&GameBoy::Write, this, RW_SRC::CPU, std::placeholders::_1, std::placeholders::_2)),
    cartridge_(nullptr)
{
}

void GameBoy::Run()
{
    if (!cartridge_)
    {
        return;
    }

    while (true)
    {
        if (cpu_.Clock(CheckPendingInterrupts()))
        {
            AcknowledgeInterrupt();
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
    ioReg_.fill(0x00);
    ioReg_[IO_REG::JOYP] = 0xCF;
    ioReg_[IO_REG::SC] = 0x7F;
    ioReg_[IO_REG::TAC] = 0xF8;
    ioReg_[IO_REG::IF] = 0xE1;
    ioReg_[IO_REG::NR10] = 0x80;
    ioReg_[IO_REG::NR11] = 0xBF;
    ioReg_[IO_REG::NR12] = 0xF3;
    ioReg_[IO_REG::NR13] = 0xFF;
    ioReg_[IO_REG::NR14] = 0xBF;
    ioReg_[IO_REG::NR21] = 0x3F;
    ioReg_[IO_REG::NR23] = 0xFF;
    ioReg_[IO_REG::NR24] = 0xBF;
    ioReg_[IO_REG::NR30] = 0x7F;
    ioReg_[IO_REG::NR31] = 0xFF;
    ioReg_[IO_REG::NR32] = 0x9F;
    ioReg_[IO_REG::NR33] = 0xFF;
    ioReg_[IO_REG::NR34] = 0xBF;
    ioReg_[IO_REG::NR41] = 0xFF;
    ioReg_[IO_REG::NR44] = 0xBF;
    ioReg_[IO_REG::NR50] = 0x77;
    ioReg_[IO_REG::NR51] = 0xF3;
    ioReg_[IO_REG::NR52] = 0xF1;
    ioReg_[IO_REG::LCDC] = 0x91;
    ioReg_[IO_REG::BGP] = 0xFC;
    ioReg_[IO_REG::KEY1] = 0xFF;
    ioReg_[IO_REG::VBK] = 0xFF;
    ioReg_[IO_REG::HDMA1] = 0xFF;
    ioReg_[IO_REG::HDMA2] = 0xFF;
    ioReg_[IO_REG::HDMA3] = 0xFF;
    ioReg_[IO_REG::HDMA4] = 0xFF;
    ioReg_[IO_REG::HDMA5] = 0xFF;
    ioReg_[IO_REG::SVBK] = 0xFF;

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

void GameBoy::ClockTimer()
{
    ++divCounter_;

    if (divCounter_ == 64)
    {
        ++ioReg_[IO_REG::DIV];
        divCounter_ = 0;
    }

    if (timerEnabled_)
    {
        ++timerCounter_;

        if (timerReload_)
        {
            timerReload_ = false;
            ioReg_[IO_REG::TIMA] = ioReg_[IO_REG::TMA];
            ioReg_[IO_REG::IF] |= INT_SRC::TIMER;
        }
        else if (timerCounter_ == timerControl_)
        {
            timerCounter_ = 0x0000;
            ++ioReg_[IO_REG::TIMA];

            if (ioReg_[IO_REG::TIMA] == 0x00)
            {
                timerReload_ = true;
            }
        }
    }
}

void GameBoy::ClockOamDma()
{
    OAM_[oamIndexDest_++] = Read(RW_SRC::BUS, oamDmaSrcAddr_++);
    --oamDmaCyclesRemaining_;

    if (oamDmaCyclesRemaining_ == 0)
    {
        oamDmaInProgress_ = false;
    }
}

void GameBoy::ClockSerialTransfer()
{
    serialOutData_ <<= 1;
    serialOutData_ |= (ioReg_[IO_REG::SB] & 0x80) >> 7;
    ioReg_[IO_REG::SB] <<= 1;
    ioReg_[IO_REG::SB] |= 0x01;
    ++serialBitsSent_;

    if (serialBitsSent_ == 8)
    {
        serialTransferInProgress_ = false;
        ioReg_[IO_REG::SC] &= 0x7F;
        ioReg_[IO_REG::IF] |= INT_SRC::SERIAL;

        #ifdef PRINT_SERIAL
            std::cout << (char)serialOutData_;
        #endif
    }
}

std::optional<std::pair<uint16_t, uint8_t>> GameBoy::CheckPendingInterrupts()
{
    uint8_t pendingInterrupts = ioReg_[IO_REG::IF] & IE_;

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
    ioReg_[IO_REG::IF] &= ~lastPendingInterrupt_;
}

uint8_t GameBoy::Read(RW_SRC src, uint16_t addr)
{
    (void)src;

    if (addr < 0x8000)
    {
        // Cartridge ROM
        return cartridge_->ReadROM(addr);
    }
    else if (addr < 0xA000)
    {
        // VRAM, TODO: prevent reading during specific modes
        return VRAM_[ioReg_[IO_REG::VBK] & 0x01][addr - 0x8000];
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
        uint8_t ramBank = (ioReg_[IO_REG::SVBK] == 0x00) ? 0x01 : (ioReg_[IO_REG::SVBK] & 0x07);
        return WRAM_[ramBank][addr - 0xD000];
    }
    else if (addr < 0xFE00)
    {
        // ECHO RAM, prohibited, TODO
        return 0x00;
    }
    else if (addr < 0xFEA0)
    {
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

void GameBoy::Write(RW_SRC src, uint16_t addr, uint8_t data)
{
    (void)src;

    if (addr < 0x8000)
    {
        // Cartridge ROM
        cartridge_->WriteROM(addr, data);
    }
    else if (addr < 0xA000)
    {
        // VRAM, TODO: prevent writing during specific modes
        VRAM_[ioReg_[IO_REG::VBK] & 0x01][addr - 0x8000] = data;
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
        uint8_t ramBank = (ioReg_[IO_REG::SVBK] == 0x00) ? 0x01 : (ioReg_[IO_REG::SVBK] & 0x07);
        WRAM_[ramBank][addr - 0xD000] = data;
    }
    else if (addr < 0xFE00)
    {
        // ECHO RAM, prohibited, TODO
    }
    else if (addr < 0xFEA0)
    {
        // OAM, TODO
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
    return ioReg_[addr & 0x00FF];
}

void GameBoy::WriteIoReg(uint16_t addr, uint8_t data)
{
    switch (addr & 0xFF)
    {
        case IO_REG::SC: // Serial transfer control
            ioReg_[IO_REG::SC] = data;
            serialTransferInProgress_ = (data & 0x80) == 0x80;
            serialBitsSent_ = 0x00;
            break;
        case IO_REG::DIV: // Divider register
            ioReg_[IO_REG::DIV] = 0x00;
            timerCounter_ = 0x0000;
            divCounter_ = 0x00;
            break;
        case IO_REG::TIMA: // Timer counter
            timerReload_ = false;
            ioReg_[IO_REG::TIMA] = data;
            break;
        case IO_REG::TAC: // Timer control
        {
            ioReg_[IO_REG::TAC] = data;
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

            break;
        }
        case IO_REG::DMA: // OAM DMA source address
            data %= 0xE0;
            ioReg_[IO_REG::DMA] = data;
            oamDmaInProgress_ = true;
            oamDmaCyclesRemaining_ = 160;
            oamDmaSrcAddr_ = data << 8;
            oamIndexDest_ = 0;
            break;
        default:
            ioReg_[addr & 0xFF] = data;
            break;
    }
}
