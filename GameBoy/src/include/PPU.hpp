#pragma once

#include <PixelFIFO.hpp>
#include <array>
#include <cstdint>
#include <memory>
#include <queue>
#include <vector>

struct PPU_REG
{
    uint8_t const& LCDC;  // LCD control
    uint8_t& STAT;  // LCD status
    uint8_t const& SCY;  // Viewport Y position
    uint8_t const& SCX;  // Viewport X position
    uint8_t& LY;  // LCD Y coordinate
    uint8_t const& LYC;  // LY compare
    uint8_t const& WY;  // Window y position
    uint8_t const& WX;  // Window x position
};

struct PALETTE_DATA
{
    uint8_t const& BGP;  // BG palette data (DMG)
    uint8_t const& OBP0;  // OBJ palette 0 data (DMG)
    uint8_t const& OBP1;  // OBJ palette 1 data (DMG)
    std::array<uint8_t, 0x40> const& BG_CRAM;  // BG palette data (CGB)
    std::array<uint8_t, 0x40> const& OBJ_CRAM;  // OBJ palette data (CGB)
};

struct OAM_DATA
{
    uint8_t const& OPRI;  // OBJ priority mode (CGB)
    std::array<uint8_t, 0xA0> const& OAM;  // OAM memory
};

class PPU
{
public:
    PPU(PPU_REG reg,
        PALETTE_DATA palette,
        OAM_DATA oam,
        std::array<std::array<uint8_t, 0x2000>, 2> const& vram,
        bool const& cgbMode);

    void SetFrameBuffer(uint8_t* frameBuffer) { frameBuffer_ = frameBuffer; }
    void Clock(bool oamDmaInProgress);
    void Reset();

    bool FrameReady();
    bool VBlank();

    uint8_t GetMode() const { return reg_.STAT & 0x03; }

    void SetDmgColorMode(bool useDmgColors) { useDmgColors_ = useDmgColors; }

    // LCDC flags
    bool LCDEnabled() const { return reg_.LCDC & 0x80; }
    uint16_t WindowTileMapBaseAddr() const { return (reg_.LCDC & 0x40) ? 0x9C00 : 0x9800; }
    bool WindowEnabled() const { return reg_.LCDC & 0x20; }
    bool BackgroundAndWindowTileAddrMode() const { return reg_.LCDC & 0x10; }
    uint16_t BackgroundTileMapBaseAddr() const { return (reg_.LCDC & 0x08) ? 0x9C00 : 0x9800; }
    bool TallSpriteMode() const { return reg_.LCDC & 0x04; }
    bool SpritesEnabled() const { return reg_.LCDC & 0x02; }
    bool WindowAndBackgroundEnabled() const { return reg_.LCDC & 0x01; }
    bool SpriteMasterPriority() const { return !(reg_.LCDC & 0x01); }

private:
    // STAT modes
    void SetMode(uint8_t mode) { reg_.STAT = (reg_.STAT & 0xFC) | (mode & 0x03); }
    void SetLYC()
    {
        if (reg_.LY == reg_.LYC)
        {
            reg_.STAT |= 0x04;
        }
        else
        {
            reg_.STAT &= 0xFB;
        }
    }

    // Rendering
    void RenderPixel(Pixel pixel);
    void OamScan(bool oamDmaInProgress);

    // Data from bus
    PPU_REG reg_;
    PALETTE_DATA palette_;
    OAM_DATA oam_;
    std::array<std::array<uint8_t, 0x2000>, 2> const& VRAM_;
    bool const& cgbMode_;
    uint8_t* frameBuffer_;
    size_t framePointer_;

    // State
    uint16_t dot_;
    uint8_t LX_;
    uint8_t windowY_;
    bool frameReady_;
    bool vBlank_;
    bool wyCondition_;
    bool useDmgColors_;
    uint32_t disabledDots_;

    std::vector<OamEntry> currentSprites_;

    // FIFO
    friend class PixelFIFO;
    std::unique_ptr<PixelFIFO> pixelFifoPtr_;
};
