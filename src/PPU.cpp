#include <PPU.hpp>
#include <array>
#include <cstdint>
#include <cstring>

std::array<std::array<uint8_t, 3>, 4> DMG_PALETTE = {{{175, 203, 70},
                                                      {121, 170, 109},
                                                      {34, 111, 95},
                                                      {8, 41, 85}}};

PPU::PPU(PPU_REG reg,
        PALETTE_DATA palette,
        OAM_DATA oam,
        std::array<std::array<uint8_t, 0x2000>, 2> const& vram,
        bool const& cgbMode,
        uint8_t* const frameBuffer) :
    reg_(reg),
    palette_(palette),
    oam_(oam),
    VRAM_(vram),
    cgbMode_(cgbMode),
    frameBuffer_(frameBuffer),
    pixelFifoPtr_(std::make_unique<PixelFIFO>(this))
{
    Reset();
}

void PPU::Clock(bool const oamDmaInProgress)
{
    if (!LCDEnabled())
    {
        if (!frameReady_)
        {
            ++disabledDots_;

            if (disabledDots_ == 65664)
            {
                frameReady_ = true;
                disabledDots_ = 0;
            }
        }
    }

    ++dot_;

    if (dot_ == 457)
    {
        dot_ = 1;
        ++reg_.LY;

        if (pixelFifoPtr_->WindowVisible())
        {
            ++windowY_;
        }

        if (reg_.LY == 154)
        {
            reg_.LY = 0;
            windowY_ = 0;
            vBlank_ = false;
        }

        if (reg_.LY < 144)
        {
            SetMode(2);
        }
        else if (reg_.LY == 144)
        {
            SetMode(1);
            frameReady_ = true;
            framePointer_ = 0;
            vBlank_ = true;
            wyCondition_ = false;
        }
    }
    else if (reg_.LY < 144)
    {
        if (reg_.LY == reg_.WY)
        {
            wyCondition_ = true;
        }

        if (dot_ == 80)
        {
            OamScan(oamDmaInProgress);
            LX_ = 0;
        }
        else if (dot_ == 81)
        {
            SetMode(3);
        }
        else if (LX_ == 160)
        {
            SetMode(0);
        }
    }

    SetLYC();

    if (GetMode() == 3)
    {
        auto pixel = pixelFifoPtr_->Clock();

        if (pixel)
        {
            RenderPixel(pixel.value());
            ++LX_;
        }
    }
}

void PPU::Reset()
{
    reg_.LY = 0;
    framePointer_ = 0;
    dot_ = 0;
    LX_ = 0;
    windowY_ = 0;
    frameReady_ = false;
    vBlank_ = false;
    wyCondition_ = false;
    disabledDots_ = 0;
    currentSprites_.clear();
    SetMode(2);
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

void PPU::OamScan(bool const oamDmaInProgress)
{
    currentSprites_.clear();

    if (oamDmaInProgress)
    {
        pixelFifoPtr_->LoadSprites(currentSprites_);
        return;
    }

    auto oamPtr = reinterpret_cast<OamEntry const*>(oam_.OAM.data());
    size_t numSprites = 0;

    for (uint_fast8_t oamIndex = 0; oamIndex < 40; ++oamIndex)
    {
        if (numSprites == 10)
        {
            break;
        }

        uint_fast16_t adjustedLY = reg_.LY + 16;
        uint_fast16_t spriteTopLine = oamPtr[oamIndex].yPos;
        uint_fast16_t spriteBotLine = oamPtr[oamIndex].yPos + (TallSpriteMode() ? 15 : 7);

        if ((spriteTopLine <= adjustedLY) && (spriteBotLine >= adjustedLY))
        {
            currentSprites_.push_back(oamPtr[oamIndex]);
            ++numSprites;
        }
    }

    pixelFifoPtr_->LoadSprites(currentSprites_);
}

void PPU::RenderPixel(Pixel pixel)
{
    if (cgbMode_)
    {
        if (pixel.src == PixelSource::BLANK)
        {
            frameBuffer_[framePointer_++] = 255;
            frameBuffer_[framePointer_++] = 255;
            frameBuffer_[framePointer_++] = 255;
        }
        else
        {
            uint_fast8_t colorIndex = (pixel.palette * 8) + (pixel.color * 2);
            uint_fast8_t lsb = 0x00;
            uint_fast8_t msb = 0x00;

            if (pixel.src == PixelSource::BACKGROUND)
            {
                lsb = palette_.BG_CRAM[colorIndex];
                msb = palette_.BG_CRAM[colorIndex + 1];
            }
            else
            {
                lsb = palette_.OBJ_CRAM[colorIndex];
                msb = palette_.OBJ_CRAM[colorIndex + 1];
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
            uint_fast8_t colorIndex = ((palette_.BGP >> (pixel.color * 2)) & 0x03);
            frameBuffer_[framePointer_++] = DMG_PALETTE[colorIndex][0];
            frameBuffer_[framePointer_++] = DMG_PALETTE[colorIndex][1];
            frameBuffer_[framePointer_++] = DMG_PALETTE[colorIndex][2];
        }
        else
        {
            uint_fast8_t palette = pixel.palette ? palette_.OBP1 : palette_.OBP0;
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
            frameBuffer_[framePointer_++] = 255;
            frameBuffer_[framePointer_++] = 255;
            frameBuffer_[framePointer_++] = 255;
        }
        else
        {
            uint_fast8_t lsb = 0x00;
            uint_fast8_t msb = 0x00;

            if (pixel.src == PixelSource::BACKGROUND)
            {
                uint_fast8_t colorIndex = ((palette_.BGP >> (pixel.color * 2)) & 0x03) * 2;
                lsb = palette_.BG_CRAM[colorIndex];
                msb = palette_.BG_CRAM[colorIndex + 1];
            }
            else
            {
                uint_fast8_t palette = pixel.palette ? palette_.OBP1 : palette_.OBP0;
                uint_fast8_t color = ((palette >> (pixel.color * 2)) & 0x03) * 2;
                uint_fast8_t colorIndex = color + (pixel.palette ? 8 : 0);
                lsb = palette_.OBJ_CRAM[colorIndex];
                msb = palette_.OBJ_CRAM[colorIndex + 1];
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
