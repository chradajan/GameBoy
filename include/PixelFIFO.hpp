#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <utility>
#include <vector>

class PPU;

struct OamEntry
{
    uint8_t yPos;
    uint8_t xPos;
    uint8_t tileIndex;

    struct
    {
        uint8_t cgbPalette : 3;
        uint8_t cgbTileBank : 1;
        uint8_t dmgPalette : 1;
        uint8_t xFlip : 1;
        uint8_t yFlip : 1;
        uint8_t priority : 1;
    } flags;
};

enum class PixelSource
{
    BLANK = 0,
    BACKGROUND,
    SPRITE,
};

struct Pixel
{
    uint8_t color;
    uint8_t palette;
    uint8_t spritePriority;
    bool priority;
    PixelSource src;
};

class PixelFIFO
{
public:
    PixelFIFO(PPU* ppuPtr);

    void Reset();

    std::optional<Pixel> Clock();
    void LoadSprites(std::vector<OamEntry> const& sprites);

    bool WindowVisible() const;

private:
    void ClockSpriteFetcher();
    void PushSpritePixels();
    Pixel GetSpritePixel();

    void ClockBackgroundFetcher();
    void PushBackgroundPixels();
    Pixel GetBackgroundPixel();

    Pixel GetPixel();

    PPU* const ppuPtr_;

    // State
    enum class FifoState
    {
        SPRITE_AWAITING_FETCHER,
        SPRITE_BEING_FETCHED,
        SWITCHING_TO_WINDOW,
        FETCHING_FIRST_SLICE,
        SCROLLING_FIRST_SLICE,
        RENDERING_PIXELS,
    };

    FifoState fifoState_;
    bool fetchingWindow_;
    uint8_t spriteBeingLoadedIndex_;
    uint8_t pixelsToScroll_;

    // Bg/Window data
    std::deque<Pixel> backgroundFIFO_;

    // Sprite data
    std::deque<Pixel> spriteFIFO_;
    std::array<std::deque<std::pair<OamEntry, uint8_t>>, 160> orderedSprites_;

    // Pixel fetcher
    struct Fetcher
    {
        uint8_t cycle;
        uint8_t tileId;
        uint16_t tileAddr;
        uint8_t tileDataLow;
        uint8_t tileDataHigh;

        // Attributes
        bool priority;
        bool verticalFlip;
        bool horizontalFlip;
        uint8_t vramBank;
        uint8_t palette;
    };

    Fetcher backgroundFetcher_;
    Fetcher spriteFetcher_;
};
