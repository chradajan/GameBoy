#include <GameBoy.hpp>
#include <iostream>

uint8_t GameBoy::Read(uint16_t addr)
{
    if (addr < 0x8000)
    {
        if (runningBootRom_)
        {
            if ((addr < 0x0100) || ((addr >= 0x0200) && (addr < 0x0900)))
            {
                return BOOT_ROM[addr];
            }
            else
            {
                if (cartridge_)
                {
                    return cartridge_->ReadROM(addr);
                }

                return 0xFF;
            }
        }

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

        uint_fast8_t vramBank = cgbMode_ ? (ioReg_[IO::VBK] & 0x01) : 0;
        return VRAM_[vramBank][addr - 0x8000];
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
        uint8_t ramBank = (!cgbMode_ || (ioReg_[IO::SVBK] == 0x00)) ? 0x01 : (ioReg_[IO::SVBK] & 0x07);
        return WRAM_[ramBank][addr - 0xD000];
    }
    else if (addr < 0xFE00)
    {
        // ECHO RAM, prohibited, TODO
        return 0xFF;
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
        return 0xFF;
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

    return 0xFF;
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

        uint_fast8_t vramBank = cgbMode_ ? (ioReg_[IO::VBK] & 0x01) : 0;
        VRAM_[vramBank][addr - 0x8000] = data;
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
        uint8_t ramBank = (!cgbMode_ || (ioReg_[IO::SVBK] == 0x00)) ? 0x01 : (ioReg_[IO::SVBK] & 0x07);
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
    uint_fast8_t ioAddr = addr & 0x00FF;

    switch (ioAddr)
    {
        case IO::JOYP:  // Joypad
            return ioReg_[IO::JOYP];
        case IO::SB:  // Serial transfer data
            return ioReg_[IO::SB];
        case IO::SC:  // Serial transfer control
            return ioReg_[IO::SC];
        case IO::DIV:  // Divider register
            return apu_.GetDIV();
        case IO::TIMA: // Timer counter
            return ioReg_[IO::TIMA];
        case IO::TMA:  // Timer modulo
            return ioReg_[IO::TMA];
        case IO::TAC:  // Timer control
            return ioReg_[IO::TAC];
        case IO::IF:  // Interrupt flag
            return ioReg_[IO::IF];

        case IO::NR10 ... IO::WAVE_RAM_END:
            return apu_.Read(ioAddr);

        case IO::LCDC:  // LCD control
            return ioReg_[IO::LCDC];
        case IO::STAT:  // LCD Status
            return ppu_.LCDEnabled() ? (ioReg_[IO::STAT] | 0x80) : ((ioReg_[IO::STAT] | 0x80) & 0xFC);
        case IO::SCY ... IO::SCX:  // Viewport position
            return ioReg_[ioAddr];
        case IO::LY:  // LCD Y coordinate
            return ppu_.LCDEnabled() ? ioReg_[IO::LY] : 0;
        case IO::LYC:  // LY compare
            return ioReg_[IO::LYC];
        case IO::DMA:  // OAM DMA
            return ioReg_[IO::DMA];
        case IO::BGP ... IO::OBP1:  // DMG palettes
            return ioReg_[ioAddr];
        case IO::WY ... IO::WX:  // Window position
            return ioReg_[ioAddr];
        case IO::KEY1:  // Speed switch
            return ioReg_[IO::KEY1];
        case IO::VBK:  // VRAM bank
            return ioReg_[IO::VBK];
        case IO::HDMA5:
        {
            if (hdmaInProgress_)
            {
                return vramDmaBlocksRemaining_ - 1;
            }

            return ioReg_[IO::HDMA5];
        }
        case IO::RP:  // Infrared communication port
            return 0xFF;
        case IO::BCPS:  // Background color palette specification
            return ioReg_[IO::BCPS];
        case IO::BCPD:  // Background color palette data
        {
            if (ppu_.LCDEnabled() && (ppu_.GetMode() == 3))
            {
                return 0xFF;
            }

            return BG_CRAM_[ioReg_[IO::BCPS] & 0x3F];
        }
        case IO::OCPS:  // OBJ color palette specification
            return ioReg_[IO::OCPS];
        case IO::OCPD:  // OBJ color palette data
        {
            if (ppu_.LCDEnabled() && (ppu_.GetMode() == 3))
            {
                return 0xFF;
            }

            return OBJ_CRAM_[ioReg_[IO::OCPS] & 0x3F];
        }
        case IO::OPRI:  // OBJ priority mode
            return ioReg_[IO::OPRI];
        case IO::SVBK:  // WRAM bank
            return ioReg_[IO::SVBK];
        case IO::ff72 ... IO::ff75:
            return ioReg_[ioAddr];
        default:
            return 0xFF;
    }
}

void GameBoy::WriteIoReg(uint16_t addr, uint8_t data)
{
    uint_fast8_t ioAddr = addr & 0x00FF;

    switch (ioAddr)
    {
        case IO::JOYP:  // Joypad
            ioReg_[IO::JOYP] = (0xC0 | (data & 0x30));
            UpdateInputs();
            break;
        case IO::SB:  // Serial data
            ioReg_[IO::SB] = data;
            break;
        case IO::SC:  // Serial transfer control
            IoWriteSC(data);
            break;
        case IO::DIV:  // Divider register
            apu_.ResetDIV(DoubleSpeedMode());
            timerCounter_ = 0;
            break;
        case IO::TIMA:  // Timer counter
            timerReload_ = false;
            ioReg_[IO::TIMA] = data;
            break;
        case IO::TMA:  // Timer modulo
            ioReg_[IO::TMA] = data;
            break;
        case IO::TAC:  // Timer control
            IoWriteTAC(data);
            break;
        case IO::IF:  // Interrupt flag
            ioReg_[IO::IF] = (data | 0xE0);
            break;

        case IO::NR10 ... IO::WAVE_RAM_END:
            apu_.Write(ioAddr, data);
            break;

        case IO::LCDC:  // LCD control
        {
            bool currentlyEnabled = ppu_.LCDEnabled();
            ioReg_[IO::LCDC] = data;

            if (!ppu_.LCDEnabled())
            {
                ppu_.Reset();
                ioReg_[IO::STAT] &= 0xFC;
            }
            else if (!currentlyEnabled && ppu_.LCDEnabled())
            {
                ppu_.Reset();
            }
            break;
        }
        case IO::STAT:  // LCD status
            data &= 0x78;
            ioReg_[IO::STAT] &= 0x07;
            ioReg_[IO::STAT] |= (data | 0x80);
            break;
        case IO::SCY ... IO::SCX:  // Viewport position
            ioReg_[ioAddr] = data;
            break;
        case IO::LY:  // LCD Y coordinate
            break;
        case IO::LYC:  // LY compare
            ioReg_[IO::LYC] = data;
            break;
        case IO::DMA:  // OAM DMA source address
            IoWriteDMA(data);
            break;
        case IO::BGP ... IO::OBP1:  // DMG palettes
            ioReg_[ioAddr] = data;
            break;
        case IO::WY ... IO::WX:  // Window position
            ioReg_[ioAddr] = data;
            break;
        case IO::KEY1:  // Speed switch
            ioReg_[IO::KEY1] = (ioReg_[IO::KEY1] & 0x80) | (data & 0x01) | 0x7E;
            break;
        case IO::VBK:  // VRAM bank
            ioReg_[IO::VBK] = (data | 0xFE);
            break;
        case IO::UNMAP_BOOT_ROM:  // Boot ROM disable
            if (runningBootRom_)
            {
                runningBootRom_ = false;
                cgbMode_ = cgbCartridge_;
            }
            break;
        case IO::HDMA1 ... IO::HDMA4:  // VRAM  DMA src/dest
            ioReg_[ioAddr] = data;
            break;
        case IO::HDMA5:  // VRAM DMA length/mode/start
            IoWriteVramDMA(data);
            break;
        case IO::RP:  // Infrared communication port
            break;
        case IO::BCPS:  // Background color palette specification
            ioReg_[IO::BCPS] = (data & 0xBF);
            break;
        case IO::BCPD:  // Background color palette data
            IoWriteBCPD(data);
            break;
        case IO::OCPS:  // OBJ color palette specification
            ioReg_[IO::OCPS] = (data & 0xBF);
            break;
        case IO::OCPD:  // OBJ color palette data
            IoWriteOCPD(data);
            break;
        case IO::OPRI:  // OBJ priority mode
            if (runningBootRom_)
            {
                ioReg_[IO::OPRI] = data | 0xFE;
            }
            break;
        case IO::SVBK:  // WRAM bank
            ioReg_[IO::SVBK] = data;
            break;
        case IO::ff72 ... IO::ff74:
            ioReg_[ioAddr] = data;
            break;
        case IO::ff75:
            ioReg_[IO::ff75] = data | 0x8F;
            break;
        default:
            break;
    }
}

void GameBoy::IoWriteSC(uint8_t data)
{
    if (!serialTransferInProgress_)
    {
        ioReg_[IO::SC] = (data | 0x7C);
        serialTransferInProgress_ = (data & 0x81) == 0x81;
        serialBitsSent_ = 0;

        serialTransferCounter_ = 0;
        bool fastClock = data & 0x02;

        if (DoubleSpeedMode())
        {
            serialClockDivider_ = fastClock ? 2 : 4;
        }
        else
        {
            serialClockDivider_ = fastClock ? 64 : 128;
        }
    }
}

void GameBoy::IoWriteTAC(uint8_t data)
{
    ioReg_[IO::TAC] = (data | 0xF8);
    timerCounter_ = 0;
    timerEnabled_ = data & 0x04;
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

void GameBoy::IoWriteVramDMA(uint8_t data)
{
    if (hdmaInProgress_)
    {
        if ((data & 0x80) == 0x00)
        {
            hdmaInProgress_ = false;
            transferActive_ = false;
            SetHDMARegisters();
            ioReg_[IO::HDMA5] = 0x80 | (vramDmaBlocksRemaining_ - 1);
        }
        else
        {
            vramDmaBlocksRemaining_ = (data & 0x7F) + 1;
        }

        return;
    }

    SetVramDmaAddresses();
    vramDmaBlocksRemaining_ = (data & 0x7F) + 1;

    if (data & 0x80)
    {
        hdmaInProgress_ = true;
        vramDmaBytesRemaining_ = 0;
    }
    else
    {
        gdmaInProgress_ = true;
        vramDmaBytesRemaining_ = vramDmaBlocksRemaining_ * 0x10;
    }
}

void GameBoy::IoWriteBCPD(uint8_t data)
{
    if (ppu_.LCDEnabled() && (ppu_.GetMode() == 3))
    {
        if (ioReg_[IO::BCPS] & 0x80)
        {
            ioReg_[IO::BCPS] = (ioReg_[IO::BCPS] & 0xC0) | ((ioReg_[IO::BCPS] + 1) & 0x3F);
        }

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
        if (ioReg_[IO::OCPS] & 0x80)
        {
            ioReg_[IO::OCPS] = (ioReg_[IO::OCPS] & 0xC0) | ((ioReg_[IO::OCPS] + 1) & 0x3F);
        }

        return;
    }

    OBJ_CRAM_[ioReg_[IO::OCPS] & 0x3F] = data;

    if (ioReg_[IO::OCPS] & 0x80)
    {
        ioReg_[IO::OCPS] = (ioReg_[IO::OCPS] & 0xC0) | ((ioReg_[IO::OCPS] + 1) & 0x3F);
    }
}

void GameBoy::SetHDMARegisters()
{
    ioReg_[IO::HDMA1] = (vramDmaSrc_ & 0xFF00) >> 8;
    ioReg_[IO::HDMA2] = vramDmaSrc_ & 0x00F0;
    ioReg_[IO::HDMA3] = (vramDmaDest_ & 0x1F00) >> 8;
    ioReg_[IO::HDMA4] = vramDmaDest_ & 0x00F0;
}

void GameBoy::SetVramDmaAddresses()
{
    vramDmaSrc_ = ((ioReg_[IO::HDMA1] << 8) | ioReg_[IO::HDMA2]) & 0xFFF0;
    vramDmaDest_ = 0x8000 | (((ioReg_[IO::HDMA3] << 8) | ioReg_[IO::HDMA4]) & 0x1FF0);
}

void GameBoy::SetDefaultCgbIoValues()
{
    ioReg_[IO::JOYP] = 0xCF;
    ioReg_[IO::SB] = 0x00;
    ioReg_[IO::SC] = 0x7F;
    // ioReg[IO::DIV] = ?;
    ioReg_[IO::TIMA] = 0x00;
    ioReg_[IO::TMA] = 0x00;
    ioReg_[IO::TAC] = 0xF8;
    ioReg_[IO::IF] = 0xE1;
    // Audio registers initialized in APU
    ioReg_[IO::LCDC] = 0x91;
    // ioReg_[IO::STAT] = ?;
    ioReg_[IO::SCY] = 0x00;
    ioReg_[IO::SCX] = 0x00;
    // ioReg_[IO::LY] = ?;
    ioReg_[IO::LYC] = 0x00;
    ioReg_[IO::DMA] = 0x00;
    ioReg_[IO::BGP] = 0xFC;
    // ioReg_[IO::OBP0] = ?;
    // ioReg_[IO::OBP1] = ?;
    ioReg_[IO::WY] = 0x00;
    ioReg_[IO::WX] = 0x00;
    ioReg_[IO::KEY1] = 0xFF;
    ioReg_[IO::VBK] = 0xFF;
    ioReg_[IO::HDMA1] = 0xFF;
    ioReg_[IO::HDMA2] = 0xFF;
    ioReg_[IO::HDMA3] = 0xFF;
    ioReg_[IO::HDMA4] = 0xFF;
    ioReg_[IO::HDMA5] = 0xFF;
    ioReg_[IO::RP] = 0xFF;
    // ioReg_[IO::BCPS] = ?;
    // ioReg_[IO::BCPD] = ?;
    // ioReg_[IO::OCPS] = ?;
    // ioReg_[IO::OCPD] = ?;
    // ioReg_[IO::OPRI] = ?;
    ioReg_[IO::SVBK] = 0xFF;
    ioReg_[IO::ff72] = 0x00;
    ioReg_[IO::ff73] = 0x00;
    ioReg_[IO::ff74] = 0xFF;
    ioReg_[IO::ff75] = 0x8F;
}
