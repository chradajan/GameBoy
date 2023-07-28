#include <PPU.hpp>
#include <array>
#include <cstdint>

std::array<std::array<uint8_t, 3>, 4> DMG_PALETTE = {{{175, 203, 70},
                                                      {121, 170, 109},
                                                      {34, 111, 95},
                                                      {8, 41, 85}}};

PPU::PPU(bool const& cgbMode) :
    cgbMode_(cgbMode),
    pixelFifoPtr_(std::make_unique<PixelFIFO>(this))
{
}

void PPU::PowerOn(bool const skipBootRom)
{
    BG_CRAM_.fill(0x00);
    OBJ_CRAM_.fill(0x00);
    OAM_.fill(0x00);
    VRAM_[0].fill(0x00);
    VRAM_[1].fill(0x00);

    framePointer_ = 0;
    dot_ = 0;
    LX_ = 0;
    windowY_ = 0;
    frameReady_ = false;
    vBlank_ = false;
    wyCondition_ = false;
    useDmgColors_ = false;
    oamDmaInProgress_ = false;

    disabledY_ = 0;
    firstEnabledFrame_ = false;

    if (skipBootRom)
    {
        LCDC_ = 0x91;
        STAT_ = 0x00;
        SCY_ = 0x00;
        SCX_ = 0x00;
        LY_ = 0x00;
        LYC_ = 0x00;
        BGP_ = 0xFC;
        OBP0_ = 0x00;
        OBP1_ = 0x00;
        WY_ = 0x00;
        WX_ = 0x00;
        VBK_ = 0xFF;
        BCPS_ = 0x00;
        OCPS_ = 0x00;
        OPRI_ = 0x00;
    }
    else
    {
        LCDC_ = 0x00;
        STAT_ = 0x00;
        SCY_ = 0x00;
        SCX_ = 0x00;
        LY_ = 0x00;
        LYC_ = 0x00;
        BGP_ = 0x00;
        OBP0_ = 0x00;
        OBP1_ = 0x00;
        WY_ = 0x00;
        WX_ = 0x00;
        VBK_ = 0x00;
        BCPS_ = 0x00;
        OCPS_ = 0x00;
        OPRI_ = 0x00;
    }
}

void PPU::Clock()
{
    if (!LCDEnabled())
    {
        DisabledClock();
        return;
    }

    ++dot_;

    if (dot_ == 457)
    {
        dot_ = 0;
        ++LY_;

        if (pixelFifoPtr_->WindowVisible())
        {
            ++windowY_;
        }

        if (LY_ == 154)
        {
            LY_ = 0;
            windowY_ = 0;
            vBlank_ = false;
        }

        if (LY_ < 144)
        {
            SetMode(2);
        }
        else if (LY_ == 144)
        {
            SetMode(1);
            frameReady_ = true;
            framePointer_ = 0;
            vBlank_ = true;
            wyCondition_ = false;
            firstEnabledFrame_ = false;
        }
    }
    else if (LY_ < 144)
    {
        if (LY_ == WY_)
        {
            wyCondition_ = true;
        }

        if (dot_ == 80)
        {
            OamScan();
        }
        else if (dot_ == 81)
        {
            SetMode(3);
        }
        else if (LX_ == 160)
        {
            LX_ = 0;
            SetMode(0);
        }
    }

    SetLYC();

    if ((GetMode() == 3) && (dot_ > 84))
    {
        auto pixel = pixelFifoPtr_->Clock();

        if (pixel)
        {
            RenderPixel(pixel.value());
            ++LX_;
        }
    }
}

void PPU::DisabledClock()
{
    ++dot_;

    if (dot_ == 457)
    {
        dot_ = 0;
        ++disabledY_;

        if (disabledY_ == 144)
        {
            frameReady_ = true;
            framePointer_ = 0;
        }
        else if (disabledY_ == 154)
        {
            disabledY_ = 0;
        }
    }
    else if ((disabledY_ < 144) && (dot_ < 161))
    {
        frameBuffer_[framePointer_++] = 0xFF;
        frameBuffer_[framePointer_++] = 0xFF;
        frameBuffer_[framePointer_++] = 0xFF;
    }
}

uint8_t PPU::Read(uint16_t addr) const
{
    if ((addr >= 0x8000) && (addr < 0xA000))  // VRAM
    {
        if (GetMode() == 3)
        {
            return 0xFF;
        }

        uint_fast8_t vramBank = cgbMode_ ? (VBK_ & 0x01) : 0;
        return VRAM_[vramBank][addr - 0x8000];
    }
    else if ((addr >= 0xFE00) && (addr < 0xFEA0))  // OAM
    {
        if ((GetMode() == 2) || (GetMode() == 3))
        {
            return 0xFF;
        }

        return OAM_[addr - 0xFE00];
    }
    else if ((addr >= 0xFF00) && (addr < 0xFF80))  // IO Registers
    {
        return ReadIoReg(addr & 0x00FF);
    }

    return 0xFF;
}

void PPU::Write(uint16_t addr, uint8_t data, bool const oamDmaWrite)
{
    if ((addr >= 0x8000) && (addr < 0xA000))  // VRAM
    {
        if (GetMode() == 3)
        {
            return;
        }

        uint_fast8_t vramBank = cgbMode_ ? (VBK_ & 0x01) : 0;
        VRAM_[vramBank][addr - 0x8000] = data;
    }
    else if ((addr >= 0xFE00) && (addr < 0xFEA0))  // OAM
    {
        if (!oamDmaWrite && ((GetMode() == 2) || (GetMode() == 3)))
        {
            return;
        }
        else if (oamDmaWrite)
        {
            oamDmaInProgress_ = addr != 0xFE9F;
        }

        OAM_[addr - 0xFE00] = data;
    }
    else if ((addr >= 0xFF00) && (addr < 0xFF80))  // IO Registers
    {
        WriteIoReg(addr & 0x00FF, data);
    }
}

bool PPU::FrameReady()
{
    if (frameReady_)
    {
        frameReady_ = false;
        return true;
    }

    return false;
}

bool PPU::VBlank()
{
    if (vBlank_)
    {
        vBlank_ = false;
        return true;
    }

    return false;
}

bool PPU::IsSerializable() const
{
    return (!frameReady_ && ((LCDEnabled() && (LY_ > 143)) || (!LCDEnabled() && (disabledY_ > 143))));
}

void PPU::Serialize(std::ofstream& out)
{
    out.write(reinterpret_cast<char*>(&LCDC_), sizeof(LCDC_));
    out.write(reinterpret_cast<char*>(&STAT_), sizeof(STAT_));
    out.write(reinterpret_cast<char*>(&SCY_), sizeof(SCY_));
    out.write(reinterpret_cast<char*>(&SCX_), sizeof(SCX_));
    out.write(reinterpret_cast<char*>(&LY_), sizeof(LY_));
    out.write(reinterpret_cast<char*>(&LYC_), sizeof(LYC_));
    out.write(reinterpret_cast<char*>(&WY_), sizeof(WY_));
    out.write(reinterpret_cast<char*>(&WX_), sizeof(WX_));

    out.write(reinterpret_cast<char*>(&BGP_), sizeof(BGP_));
    out.write(reinterpret_cast<char*>(&OBP0_), sizeof(OBP0_));
    out.write(reinterpret_cast<char*>(&OBP1_), sizeof(OBP1_));
    out.write(reinterpret_cast<char*>(&BCPS_), sizeof(BCPS_));
    out.write(reinterpret_cast<char*>(&OCPS_), sizeof(OCPS_));

    out.write(reinterpret_cast<char*>(BG_CRAM_.data()), BG_CRAM_.size());
    out.write(reinterpret_cast<char*>(OBJ_CRAM_.data()), OBJ_CRAM_.size());

    out.write(reinterpret_cast<char*>(&OPRI_), sizeof(OPRI_));
    out.write(reinterpret_cast<char*>(OAM_.data()), OAM_.size());

    out.write(reinterpret_cast<char*>(&VBK_), sizeof(VBK_));
    out.write(reinterpret_cast<char*>(VRAM_[0].data()), VRAM_[0].size());
    out.write(reinterpret_cast<char*>(VRAM_[1].data()), VRAM_[1].size());

    out.write(reinterpret_cast<char*>(&dot_), sizeof(dot_));
    out.write(reinterpret_cast<char*>(&vBlank_), sizeof(vBlank_));

    out.write(reinterpret_cast<char*>(&disabledY_), sizeof(disabledY_));
    out.write(reinterpret_cast<char*>(&firstEnabledFrame_), sizeof(firstEnabledFrame_));
}

void PPU::Deserialize(std::ifstream& in)
{
    in.read(reinterpret_cast<char*>(&LCDC_), sizeof(LCDC_));
    in.read(reinterpret_cast<char*>(&STAT_), sizeof(STAT_));
    in.read(reinterpret_cast<char*>(&SCY_), sizeof(SCY_));
    in.read(reinterpret_cast<char*>(&SCX_), sizeof(SCX_));
    in.read(reinterpret_cast<char*>(&LY_), sizeof(LY_));
    in.read(reinterpret_cast<char*>(&LYC_), sizeof(LYC_));
    in.read(reinterpret_cast<char*>(&WY_), sizeof(WY_));
    in.read(reinterpret_cast<char*>(&WX_), sizeof(WX_));

    in.read(reinterpret_cast<char*>(&BGP_), sizeof(BGP_));
    in.read(reinterpret_cast<char*>(&OBP0_), sizeof(OBP0_));
    in.read(reinterpret_cast<char*>(&OBP1_), sizeof(OBP1_));
    in.read(reinterpret_cast<char*>(&BCPS_), sizeof(BCPS_));
    in.read(reinterpret_cast<char*>(&OCPS_), sizeof(OCPS_));

    in.read(reinterpret_cast<char*>(BG_CRAM_.data()), BG_CRAM_.size());
    in.read(reinterpret_cast<char*>(OBJ_CRAM_.data()), OBJ_CRAM_.size());

    in.read(reinterpret_cast<char*>(&OPRI_), sizeof(OPRI_));
    in.read(reinterpret_cast<char*>(OAM_.data()), OAM_.size());

    in.read(reinterpret_cast<char*>(&VBK_), sizeof(VBK_));
    in.read(reinterpret_cast<char*>(VRAM_[0].data()), VRAM_[0].size());
    in.read(reinterpret_cast<char*>(VRAM_[1].data()), VRAM_[1].size());

    in.read(reinterpret_cast<char*>(&dot_), sizeof(dot_));
    in.read(reinterpret_cast<char*>(&vBlank_), sizeof(vBlank_));

    in.read(reinterpret_cast<char*>(&disabledY_), sizeof(disabledY_));
    in.read(reinterpret_cast<char*>(&firstEnabledFrame_), sizeof(firstEnabledFrame_));
}

void PPU::OamScan()
{
    std::vector<OamEntry> currentSprites;

    if (oamDmaInProgress_)
    {
        pixelFifoPtr_->LoadSprites(currentSprites);
        return;
    }

    auto oamPtr = reinterpret_cast<OamEntry const*>(OAM_.data());
    size_t numSprites = 0;

    for (uint_fast8_t oamIndex = 0; oamIndex < 40; ++oamIndex)
    {
        if (numSprites == 10)
        {
            break;
        }

        uint_fast16_t adjustedLY = LY_ + 16;
        uint_fast16_t spriteTopLine = oamPtr[oamIndex].yPos;
        uint_fast16_t spriteBotLine = oamPtr[oamIndex].yPos + (TallSpriteMode() ? 15 : 7);

        if ((spriteTopLine <= adjustedLY) && (spriteBotLine >= adjustedLY))
        {
            currentSprites.push_back(oamPtr[oamIndex]);
            ++numSprites;
        }
    }

    pixelFifoPtr_->LoadSprites(currentSprites);
}

void PPU::RenderPixel(Pixel pixel)
{
    if (firstEnabledFrame_)
    {
        if (useDmgColors_)
        {
            frameBuffer_[framePointer_++] = DMG_PALETTE[0][0];
            frameBuffer_[framePointer_++] = DMG_PALETTE[0][1];
            frameBuffer_[framePointer_++] = DMG_PALETTE[0][2];
        }
        else
        {
            frameBuffer_[framePointer_++] = 0xFF;
            frameBuffer_[framePointer_++] = 0xFF;
            frameBuffer_[framePointer_++] = 0xFF;
        }
    }
    else if (cgbMode_)
    {
        if (pixel.src == PixelSource::BLANK)
        {
            frameBuffer_[framePointer_++] = 0xFF;
            frameBuffer_[framePointer_++] = 0xFF;
            frameBuffer_[framePointer_++] = 0xFF;
        }
        else
        {
            uint_fast8_t colorIndex = (pixel.palette * 8) + (pixel.color * 2);
            uint_fast8_t lsb = 0x00;
            uint_fast8_t msb = 0x00;

            if (pixel.src == PixelSource::BACKGROUND)
            {
                lsb = BG_CRAM_[colorIndex];
                msb = BG_CRAM_[colorIndex + 1];
            }
            else
            {
                lsb = OBJ_CRAM_[colorIndex];
                msb = OBJ_CRAM_[colorIndex + 1];
            }

            uint_fast16_t rgb555 = (msb << 8) | lsb;

            uint_fast8_t r = rgb555 & 0x001F;
            frameBuffer_[framePointer_++] = ((r << 3) | (r >> 2));

            uint_fast8_t g = (rgb555 & 0x03E0) >> 5;
            frameBuffer_[framePointer_++] = ((g << 3) | (g >> 2));

            uint_fast8_t b = (rgb555 & 0x7C00) >> 10;
            frameBuffer_[framePointer_++] = ((b << 3) | (b >> 2));
        }
    }
    else if (useDmgColors_)
    {
        if (pixel.src == PixelSource::BLANK)
        {
            frameBuffer_[framePointer_++] = DMG_PALETTE[0][0];
            frameBuffer_[framePointer_++] = DMG_PALETTE[0][1];
            frameBuffer_[framePointer_++] = DMG_PALETTE[0][2];
        }
        else if (pixel.src == PixelSource::BACKGROUND)
        {
            uint_fast8_t colorIndex = ((BGP_ >> (pixel.color * 2)) & 0x03);
            frameBuffer_[framePointer_++] = DMG_PALETTE[colorIndex][0];
            frameBuffer_[framePointer_++] = DMG_PALETTE[colorIndex][1];
            frameBuffer_[framePointer_++] = DMG_PALETTE[colorIndex][2];
        }
        else
        {
            uint_fast8_t palette = pixel.palette ? OBP1_ : OBP0_;
            uint_fast8_t colorIndex = ((palette >> (pixel.color * 2)) & 0x03);
            frameBuffer_[framePointer_++] = DMG_PALETTE[colorIndex][0];
            frameBuffer_[framePointer_++] = DMG_PALETTE[colorIndex][1];
            frameBuffer_[framePointer_++] = DMG_PALETTE[colorIndex][2];
        }
    }
    else
    {
        if (pixel.src == PixelSource::BLANK)
        {
            frameBuffer_[framePointer_++] = 0xFF;
            frameBuffer_[framePointer_++] = 0xFF;
            frameBuffer_[framePointer_++] = 0xFF;
        }
        else
        {
            uint_fast8_t lsb = 0x00;
            uint_fast8_t msb = 0x00;

            if (pixel.src == PixelSource::BACKGROUND)
            {
                uint_fast8_t colorIndex = ((BGP_ >> (pixel.color * 2)) & 0x03) * 2;
                lsb = BG_CRAM_[colorIndex];
                msb = BG_CRAM_[colorIndex + 1];
            }
            else
            {
                uint_fast8_t palette = pixel.palette ? OBP1_ : OBP0_;
                uint_fast8_t color = ((palette >> (pixel.color * 2)) & 0x03) * 2;
                uint_fast8_t colorIndex = color + (pixel.palette ? 8 : 0);
                lsb = OBJ_CRAM_[colorIndex];
                msb = OBJ_CRAM_[colorIndex + 1];
            }

            uint_fast16_t rgb555 = (msb << 8) | lsb;

            uint_fast8_t r = rgb555 & 0x001F;
            frameBuffer_[framePointer_++] = ((r << 3) | (r >> 2));

            uint_fast8_t g = (rgb555 & 0x03E0) >> 5;
            frameBuffer_[framePointer_++] = ((g << 3) | (g >> 2));

            uint_fast8_t b = (rgb555 & 0x7C00) >> 10;
            frameBuffer_[framePointer_++] = ((b << 3) | (b >> 2));
        }
    }
}

uint8_t PPU::ReadIoReg(uint8_t ioAddr) const
{
    switch (ioAddr)
    {
        case IO::LCDC: // LCD control
            return LCDC_;
        case IO::STAT: // LCD status
            return STAT_ | 0x80;
        case IO::SCY:  // Viewport Y position
            return SCY_;
        case IO::SCX:  // Viewport X position
            return SCX_;
        case IO::LY:   // LCD Y coordinate
        {
            if ((LY_ == 153) && (dot_ > 3))
            {
                return 0;
            }

            return LY_;
        }
        case IO::LYC:  // LY compare
            return LYC_;
        case IO::BGP:  // BG palette data (Non-CGB mode only)
            return BGP_;
        case IO::OBP0: // OBJ palette 0 data (Non-CGB mode only)
            return OBP0_;
        case IO::OBP1: // OBJ palette 1 data (Non-CGB mode only)
            return OBP1_;
        case IO::WY:   // Window y position
            return WY_;
        case IO::WX:   // Window x position
            return WX_;
        case IO::VBK: // VRAM bank (CGB mode only)
            return VBK_ | 0xFE;
        case IO::BCPS: // Background color palette specification (CGB mode only)
            return BCPS_ | 0x40;
        case IO::BCPD: // Background color palette data (CGB mode only)
        {
            if (GetMode() == 3)
            {
                return 0xFF;
            }

            return BG_CRAM_[BCPS_ & 0x3F];
        }
        case IO::OCPS: // OBJ color palette specification (CGB mode only)
            return OCPS_ | 0x40;
        case IO::OCPD: // OBJ color palette data (CGB mode only)
        {
            if (GetMode() == 3)
            {
                return 0xFF;
            }

            return OBJ_CRAM_[OCPS_ & 0x3F];
        }
        case IO::OPRI: // OBJ priority mode (CGB mode only)
            return OPRI_;
        default:
            return 0xFF;
    }
}

void PPU::WriteIoReg(uint8_t ioAddr, uint8_t data)
{
    switch (ioAddr)
    {
        case IO::LCDC: // LCD control
        {
            bool const wasEnabled = LCDEnabled();
            LCDC_ = data;
            bool const isEnabled = LCDEnabled();

            if (wasEnabled && !isEnabled)
            {
                LY_ = 0;
                LX_ = 0;
                windowY_ = 0;
                disabledY_ = 0;
                dot_ = 0;
                framePointer_ = 0;
                frameReady_ = false;
                vBlank_ = false;
                wyCondition_ = false;
                STAT_ &= 0xFC;
            }
            else if (!wasEnabled && isEnabled)
            {
                dot_ = 0;
                framePointer_ = 0;
                frameReady_ = false;
                firstEnabledFrame_ = true;
                SetMode(2);
            }
            break;
        }
        case IO::STAT: // LCD status
        {
            data &= 0x78;
            STAT_ &= 0x07;
            STAT_ |= data;
            break;
        }
        case IO::SCY:  // Viewport Y position
            SCY_ = data;
            break;
        case IO::SCX:  // Viewport X position
            SCX_ = data;
            break;
        case IO::LY:   // LCD Y coordinate
            break;
        case IO::LYC:  // LY compare
            LYC_ = data;
            break;
        case IO::BGP:  // BG palette data (Non-CGB mode only)
            BGP_ = data;
            break;
        case IO::OBP0: // OBJ palette 0 data (Non-CGB mode only)
            OBP0_ = data;
            break;
        case IO::OBP1: // OBJ palette 1 data (Non-CGB mode only)
            OBP1_ = data;
            break;
        case IO::WY:   // Window y position
            WY_ = data;
            break;
        case IO::WX:   // Window x position
            WX_ = data;
            break;
        case IO::VBK: // VRAM bank (CGB mode only)
            VBK_ = data;
            break;
        case IO::BCPS: // Background color palette specification (CGB mode only)
            BCPS_ = data;
            break;
        case IO::BCPD: // Background color palette data (CGB mode only)
        {
            if (GetMode() != 3)
            {
                BG_CRAM_[BCPS_ & 0x3F] = data;
            }

            if (BCPS_ & 0x80)
            {
                BCPS_ =  (BCPS_ & 0x80) | ((BCPS_ + 1) & 0x3F);
            }

            break;
        }
        case IO::OCPS: // OBJ color palette specification (CGB mode only)
            OCPS_ = data;
            break;
        case IO::OCPD: // OBJ color palette data (CGB mode only)
        {
            if (GetMode() != 3)
            {
                OBJ_CRAM_[OCPS_ & 0x3F] = data;
            }

            if (OCPS_ & 0x80)
            {
                OCPS_ =  (OCPS_ & 0x80) | ((OCPS_ + 1) & 0x3F);
            }

            break;
        }
        case IO::OPRI: // OBJ priority mode (CGB mode only)
            OPRI_ = data;
            break;
        default:
            break;
    }
}
