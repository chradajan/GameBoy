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
        case IO::STAT:  // LCD Status
            return (ioReg_[IO::STAT] | 0x80);
        // case IO::KEY1:
        //     std::cout << "read\n";
        //     throw;
        //     return 0xFF;
        case IO::VBK:  // VRAM bank
            return (ioReg_[IO::VBK] | 0xFE);
        case IO::HDMA1:
        case IO::HDMA2:
        case IO::HDMA3:
        case IO::HDMA4:
            return 0xFF;
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
        case IO::SVBK:  // WRAM bank
            return (ioReg_[IO::SVBK] | 0xF8);
        default:
            break;
    }

    return ioReg_[addr & 0x00FF];
}

void GameBoy::WriteIoReg(uint16_t addr, uint8_t data)
{
    switch (addr & 0xFF)
    {
        case IO::JOYP:
            ioReg_[IO::JOYP] = (0xC0 | (data & 0x30));
            SetInputs(GetButtons());
            break;
        case IO::SC:  // Serial transfer control
            if (!serialTransferInProgress_)
            {
                ioReg_[IO::SC] = data;
                serialTransferInProgress_ = (data & 0x80) == 0x80;
                serialBitsSent_ = 0x00;
            }
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
        case IO::LCDC:  // LCD control
        {
            bool currentlyEnabled = ppu_.LCDEnabled();
            ioReg_[IO::LCDC] = data;

            if (!ppu_.LCDEnabled())
            {
                ppu_.Reset();
                ioReg_[IO::STAT] &= 0x7C;
            }
            else if (!currentlyEnabled && ppu_.LCDEnabled())
            {
                ppu_.Reset();
            }
            break;
        }
        case IO::STAT:  // LCD status
        {
            data &= 0x78;
            ioReg_[IO::STAT] &= 0x07;
            ioReg_[IO::STAT] |= data;
            break;
        }
        case IO::LY:  // LCD Y Coordinate
            break;
        case IO::DMA:  // OAM DMA source address
            IoWriteDMA(data);
            break;
        case IO::UNMAP_BOOT_ROM:  // Boot ROM disable
            runningBootRom_ = false;
            cgbMode_ = cgbCartridge_;
            break;
        case IO::HDMA5:  // VRAM DMA length/mode/start
            IoWriteVramDMA(data);
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

void GameBoy::IoWriteVramDMA(uint8_t data)
{
    if (hdmaInProgress_)
    {
        if ((data & 0x80) == 0x00)
        {
            hdmaInProgress_ = false;
            ioReg_[IO::HDMA5] = (0x80 | (hdmaBlocksRemaining_ - 1));
        }

        return;
    }

    vramDmaSrcAddr_ = ((ioReg_[IO::HDMA1] << 8) | ioReg_[IO::HDMA2]) & 0xFFF0;
    vramDmaDestAddr_ = 0x8000 | (((ioReg_[IO::HDMA3] << 8) | ioReg_[IO::HDMA4]) & 0x1FF0);

    if (data & 0x80)
    {
        hdmaInProgress_ = true;
    }
    else
    {
        gdmaInProgress_ = true;
    }

    hdmaBlocksRemaining_ = (data & 0x7F) + 1;
    gdmaBytesRemaining_ = hdmaBlocksRemaining_ * 0x10;
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
