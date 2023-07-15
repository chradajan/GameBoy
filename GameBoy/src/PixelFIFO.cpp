#include <PixelFIFO.hpp>
#include <PPU.hpp>
#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <utility>
#include <vector>

static_assert(sizeof(OamEntry) == 4, "OamEntry must be 4 bytes");

PixelFIFO::PixelFIFO(PPU* const ppuPtr) :
    ppuPtr_(ppuPtr)
{
}

void PixelFIFO::Reset()
{
    // State
    fifoState_ = FifoState::FETCHING_FIRST_SLICE;
    fetchingWindow_ = false;
    spriteBeingLoadedIndex_ = 0;
    pixelsToScroll_ = 0;

    // Bg/Window data
    backgroundFIFO_.clear();

    // Sprite data
    spriteFIFO_.clear();

    for (auto& queue : orderedSprites_)
    {
        queue.clear();
    }

    // Pixel fetcher
    backgroundFetcher_ = {};
    spriteFetcher_ = {};
    fetcherX_ = 0;
}

std::optional<Pixel> PixelFIFO::Clock()
{
    switch (fifoState_)
    {
        case FifoState::SPRITE_AWAITING_FETCHER:
            ClockBackgroundFetcher();
            break;
        case FifoState::SPRITE_BEING_FETCHED:
            ClockSpriteFetcher();
            break;
        case FifoState::SWITCHING_TO_WINDOW:
            ClockBackgroundFetcher();
            break;
        case FifoState::FETCHING_FIRST_SLICE:
        {
            if (SwitchToWindow())
            {
                fetchingWindow_ = true;
                backgroundFetcher_ = {};
                backgroundFIFO_.clear();
            }

            ClockBackgroundFetcher();
            break;
        }
        case FifoState::SCROLLING_FIRST_SLICE:
        {
            if (pixelsToScroll_ > 0)
            {
                backgroundFIFO_.pop_front();
                ClockBackgroundFetcher();
                --pixelsToScroll_;
            }

            if (pixelsToScroll_ == 0)
            {
                fifoState_ = FifoState::RENDERING_PIXELS;
            }
            break;
        }
        case FifoState::RENDERING_PIXELS:
        {
            if (SwitchToWindow())
            {
                fetchingWindow_ = true;
                backgroundFetcher_ = {};
                backgroundFIFO_.clear();
                fifoState_ = FifoState::SWITCHING_TO_WINDOW;
                ClockBackgroundFetcher();
            }
            else if (!orderedSprites_[ppuPtr_->LX_].empty())
            {
                fifoState_ = FifoState::SPRITE_AWAITING_FETCHER;
                ClockBackgroundFetcher();
            }
            else
            {
                Pixel pixel = GetPixel();
                ClockBackgroundFetcher();
                return pixel;
            }
            break;
        }
    }

    return {};
}

void PixelFIFO::LoadSprites(std::vector<OamEntry> const& sprites)
{
    Reset();
    uint_fast8_t index = 0;

    for (auto const& sprite : sprites)
    {
        if ((sprite.xPos == 0) || (sprite.xPos >= 168))
        {
            continue;
        }

        uint_fast8_t leftEdge = (sprite.xPos < 8) ? 0 : (sprite.xPos - 8);
        orderedSprites_[leftEdge].push_back({sprite, index});
        ++index;
    }
}

bool PixelFIFO::WindowVisible() const
{
    return fetchingWindow_;
}

bool PixelFIFO::SwitchToWindow() const
{
    return (!fetchingWindow_ &&
            ppuPtr_->WindowEnabled() &&
            (ppuPtr_->cgbMode_ || (!ppuPtr_->cgbMode_ && ppuPtr_->WindowAndBackgroundEnabled())) &&
            ppuPtr_->wyCondition_ &&
            (ppuPtr_->LX_ + 7 >= ppuPtr_->reg_.WX));
}

void PixelFIFO::ClockSpriteFetcher()
{
    ++spriteFetcher_.cycle;

    switch(spriteFetcher_.cycle)
    {
        case 1:
            break;
        case 2:     // Get tile
        {
            auto spriteInfo = orderedSprites_[ppuPtr_->LX_].front();
            OamEntry spriteToLoad = spriteInfo.first;
            spriteBeingLoadedIndex_ = spriteInfo.second;
            pixelsToScroll_ = (ppuPtr_->LX_ + 8) - spriteToLoad.xPos;

            spriteFetcher_.tileId = spriteToLoad.tileIndex;
            spriteFetcher_.priority = spriteToLoad.flags.priority;
            spriteFetcher_.verticalFlip = spriteToLoad.flags.yFlip;
            spriteFetcher_.horizontalFlip = spriteToLoad.flags.xFlip;
            spriteFetcher_.vramBank = ppuPtr_->cgbMode_ ? spriteToLoad.flags.cgbTileBank : 0;
            spriteFetcher_.palette = ppuPtr_->cgbMode_ ? spriteToLoad.flags.cgbPalette : spriteToLoad.flags.dmgPalette;

            uint_fast8_t spriteY = (ppuPtr_->reg_.LY + 16) - spriteToLoad.yPos;

            if (ppuPtr_->TallSpriteMode())
            {
                spriteFetcher_.tileId &= 0xFE;

                if (spriteFetcher_.verticalFlip)
                {
                    spriteY = 15 - spriteY;
                }

                if (spriteY > 7)
                {
                    ++spriteFetcher_.tileId;
                }
            }
            else if (spriteFetcher_.verticalFlip)
            {
                spriteY = 7 - spriteY;
            }

            spriteFetcher_.tileAddr = 0x8000 | (spriteFetcher_.tileId << 4) | ((spriteY % 8) << 1);

            orderedSprites_[ppuPtr_->LX_].pop_front();
            break;
        }
        case 3:
            break;
        case 4:     // Get tile data low
        {
            spriteFetcher_.tileDataLow = ppuPtr_->VRAM_[spriteFetcher_.vramBank][spriteFetcher_.tileAddr - 0x8000];
            break;
        }
        case 5:
            break;
        case 6:     // Get tile data high
            spriteFetcher_.tileAddr |= 0x01;
            spriteFetcher_.tileDataHigh = ppuPtr_->VRAM_[spriteFetcher_.vramBank][spriteFetcher_.tileAddr - 0x8000];
            break;
        default:    // Push
            PushSpritePixels();
            spriteFetcher_ = {};
            fifoState_ = FifoState::RENDERING_PIXELS;
            break;
    }
}

void PixelFIFO::PushSpritePixels()
{
    bool cgbMode = ppuPtr_->cgbMode_;

    for (uint_fast8_t i = 0; i < 8; ++i)
    {
        uint_fast8_t color = 0x00;

        if (spriteFetcher_.horizontalFlip)
        {
            color = ((spriteFetcher_.tileDataHigh & 0x01) << 1) | (spriteFetcher_.tileDataLow & 0x01);
            spriteFetcher_.tileDataHigh >>= 1;
            spriteFetcher_.tileDataLow >>= 1;
        }
        else
        {
            color = ((spriteFetcher_.tileDataHigh & 0x80) >> 6) | ((spriteFetcher_.tileDataLow & 0x80) >> 7);
            spriteFetcher_.tileDataHigh <<= 1;
            spriteFetcher_.tileDataLow <<= 1;
        }

        if (pixelsToScroll_ > 0)
        {
            --pixelsToScroll_;
            continue;
        }

        Pixel pixel = {color, spriteFetcher_.palette, spriteBeingLoadedIndex_, spriteFetcher_.priority, PixelSource::SPRITE};
        uint_fast8_t size = i + 1;

        if (spriteFIFO_.size() >= size)
        {
            if (color != 0x00)
            {
                if ((spriteFIFO_[i].color == 0x00) || (cgbMode && (pixel.spritePriority < spriteFIFO_[i].spritePriority)))
                {
                    spriteFIFO_[i] = pixel;
                }
            }
        }
        else
        {
            spriteFIFO_.push_back(pixel);
        }
    }
}

Pixel PixelFIFO::GetSpritePixel()
{
    if (spriteFIFO_.empty())
    {
        return {};
    }

    auto pixel = spriteFIFO_.front();
    spriteFIFO_.pop_front();
    return pixel;
}

void PixelFIFO::ClockBackgroundFetcher()
{
    ++backgroundFetcher_.cycle;

    switch (backgroundFetcher_.cycle)
    {
        case 1:
            break;
        case 2:     // Get tile
        {
            if (fetchingWindow_)
            {
                backgroundFetcher_.tileAddr = ppuPtr_->WindowTileMapBaseAddr();
                fetcherX_ = ((fifoState_ == FifoState::SWITCHING_TO_WINDOW) ||
                             (fifoState_ == FifoState::FETCHING_FIRST_SLICE)) ? 0 : (fetcherX_ + 1) % 32;
                backgroundFetcher_.tileAddr |= ((ppuPtr_->windowY_ / 8) << 5);
                backgroundFetcher_.tileAddr |= fetcherX_;
            }
            else
            {
                backgroundFetcher_.tileAddr = ppuPtr_->BackgroundTileMapBaseAddr();
                fetcherX_ = (fifoState_ == FifoState::FETCHING_FIRST_SLICE) ? ppuPtr_->reg_.SCX / 8 : (fetcherX_ + 1) % 32;
                uint8_t temp = ppuPtr_->reg_.LY + ppuPtr_->reg_.SCY;
                backgroundFetcher_.tileAddr |= ((temp / 8) << 5);
                backgroundFetcher_.tileAddr |= fetcherX_;
            }

            backgroundFetcher_.tileId = ppuPtr_->VRAM_[0][backgroundFetcher_.tileAddr - 0x8000];

            if (ppuPtr_->cgbMode_)
            {
                uint8_t attributes = ppuPtr_->VRAM_[1][backgroundFetcher_.tileAddr - 0x8000];

                backgroundFetcher_.priority = attributes & 0x80;
                backgroundFetcher_.verticalFlip = attributes & 0x40;
                backgroundFetcher_.horizontalFlip = attributes & 0x20;
                backgroundFetcher_.vramBank = (attributes & 0x08) >> 3;
                backgroundFetcher_.palette = attributes & 0x07;
            }
            else
            {
                backgroundFetcher_.verticalFlip = false;
                backgroundFetcher_.horizontalFlip = false;
                backgroundFetcher_.vramBank = 0;
            }

            break;
        }
        case 3:
            break;
        case 4:     // Get tile data low
        {
            uint_fast8_t row = fetchingWindow_ ? (ppuPtr_->windowY_ % 8) :
                                                 ((ppuPtr_->reg_.LY + ppuPtr_->reg_.SCY) % 8);

            if (backgroundFetcher_.verticalFlip)
            {
                row = ~row & 0x07;
            }

            if (ppuPtr_->BackgroundAndWindowTileAddrMode())
            {
                backgroundFetcher_.tileAddr = 0x8000 | (backgroundFetcher_.tileId << 4) | (row << 1);
            }
            else if (backgroundFetcher_.tileId & 0x80)
            {
                backgroundFetcher_.tileId &= 0x7F;
                backgroundFetcher_.tileAddr = 0x8800 | (backgroundFetcher_.tileId << 4) | (row << 1);
            }
            else
            {
                backgroundFetcher_.tileAddr = 0x9000 | (backgroundFetcher_.tileId << 4) | (row << 1);
            }

            backgroundFetcher_.tileDataLow = ppuPtr_->VRAM_[backgroundFetcher_.vramBank][backgroundFetcher_.tileAddr - 0x8000];
            break;
        }
        case 5:
            break;
        case 6:     // Get tile data high
            backgroundFetcher_.tileAddr |= 0x01;
            backgroundFetcher_.tileDataHigh = ppuPtr_->VRAM_[backgroundFetcher_.vramBank][backgroundFetcher_.tileAddr - 0x8000];
            break;
        default:    // Attempt to push
        {
            if (backgroundFIFO_.empty())
            {
                PushBackgroundPixels();
                backgroundFetcher_ = {};

                if (fifoState_ == FifoState::FETCHING_FIRST_SLICE)
                {
                    pixelsToScroll_ = fetchingWindow_ ? ((ppuPtr_->LX_ + 7) - ppuPtr_->reg_.WX) :
                                                        (ppuPtr_->reg_.SCX % 8);

                    fifoState_ = (pixelsToScroll_ > 0) ? FifoState::SCROLLING_FIRST_SLICE :
                                                         FifoState::RENDERING_PIXELS;
                }
                else if (fifoState_ == FifoState::SWITCHING_TO_WINDOW)
                {
                    fifoState_ = FifoState::RENDERING_PIXELS;
                }
            }
            else if (fifoState_ == FifoState::SPRITE_AWAITING_FETCHER)
            {
                fifoState_ = FifoState::SPRITE_BEING_FETCHED;
            }

            break;
        }
    }
}

void PixelFIFO::PushBackgroundPixels()
{
    for (uint_fast8_t i = 0; i < 8; ++i)
    {
        uint_fast8_t color = 0x00;

        if (backgroundFetcher_.horizontalFlip)
        {
            color = ((backgroundFetcher_.tileDataHigh & 0x01) << 1) | (backgroundFetcher_.tileDataLow & 0x01);
            backgroundFetcher_.tileDataHigh >>= 1;
            backgroundFetcher_.tileDataLow >>= 1;
        }
        else
        {
            color = ((backgroundFetcher_.tileDataHigh & 0x80) >> 6) | ((backgroundFetcher_.tileDataLow & 0x80) >> 7);
            backgroundFetcher_.tileDataHigh <<= 1;
            backgroundFetcher_.tileDataLow <<= 1;
        }

        backgroundFIFO_.push_back({color, backgroundFetcher_.palette, 0x00, backgroundFetcher_.priority, PixelSource::BACKGROUND});
    }
}

Pixel PixelFIFO::GetBackgroundPixel()
{
    auto pixel = backgroundFIFO_.front();
    backgroundFIFO_.pop_front();
    return pixel;
}

Pixel PixelFIFO::GetPixel()
{
    Pixel bgPixel = GetBackgroundPixel();
    Pixel spritePixel = GetSpritePixel();
    bool bgEnabled = ppuPtr_->cgbMode_ ? true : ppuPtr_->WindowAndBackgroundEnabled();
    bool spritesEnabled = ppuPtr_->SpritesEnabled();

    // First check if either one or both pixel types are disabled
    if (bgEnabled && !spritesEnabled)
    {
        return bgPixel;
    }
    else if (!bgEnabled && spritesEnabled)
    {
        if (spritePixel.color == 0x00)
        {
            return {};
        }

        return spritePixel;
    }
    else if (!bgEnabled && !spritesEnabled)
    {
        return {};
    }

    // Both are enabled, so now check color indexes
    bool bgIndex0 = bgPixel.color == 0x00;
    bool spriteIndex0 = spritePixel.color == 0x00;

    if (bgIndex0 && !spriteIndex0)
    {
        return spritePixel;
    }
    else if (!bgIndex0 && spriteIndex0)
    {
        return bgPixel;
    }
    else if (bgIndex0 && spriteIndex0)
    {
        return bgPixel;
    }

    // Both pixels have non-zero color indexes.
    if (ppuPtr_->cgbMode_)
    {
        if (ppuPtr_->SpriteMasterPriority())
        {
            return spritePixel;
        }
        else if (!bgPixel.priority && !spritePixel.priority)
        {
            return spritePixel;
        }
        else
        {
            return bgPixel;
        }
    }
    else
    {
        if (spritePixel.priority)
        {
            return bgPixel;
        }

        return spritePixel;
    }
}
