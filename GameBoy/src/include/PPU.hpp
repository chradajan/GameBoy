#pragma once

#include <PixelFIFO.hpp>
#include <array>
#include <cstdint>
#include <fstream>
#include <memory>
#include <queue>
#include <vector>

namespace IO
{
    /// @brief Enum of PPU I/O Register addresses. Only use lower byte since upper byte is 0xFF for all addresses.
enum : uint8_t
{
    LCDC = 0x40, // LCD control
    STAT = 0x41, // LCD status
    SCY = 0x42,  // Viewport Y position
    SCX = 0x43,  // Viewport X position
    LY = 0x44,   // LCD Y coordinate
    LYC = 0x45,  // LY compare
    BGP = 0x47,  // BG palette data (Non-CGB mode only)
    OBP0 = 0x48, // OBJ palette 0 data (Non-CGB mode only)
    OBP1 = 0x49, // OBJ palette 1 data (Non-CGB mode only)
    WY = 0x4A,   // Window y position
    WX = 0x4B,   // Window x position
    VBK = 0x4F, // VRAM bank (CGB mode only)
    BCPS = 0x68, // Background color palette specification (CGB mode only)
    BCPD = 0x69, // Background color palette data (CGB mode only)
    OCPS = 0x6A, // OBJ color palette specification (CGB mode only)
    OCPD = 0x6B, // OBJ color palette data (CGB mode only)
    OPRI = 0x6C, // OBJ priority mode (CGB mode only)
};
};

class PPU
{
public:
    PPU(bool const& cgbMode);
    void PowerOn(bool skipBootRom);

    void Clock();

    uint8_t Read(uint16_t addr) const;
    void Write(uint16_t addr, uint8_t data, bool oamDmaWrite = false);

    bool FrameReady();
    bool VBlank();

    uint8_t GetMode() const { return STAT_ & 0x03; }

    void SetFrameBuffer(uint8_t* frameBuffer) { frameBuffer_ = frameBuffer; }

    // DMG color control

    /// @brief Force PPU to render pixels with DMG palettes when skipping boot ROM.
    /// @param useDmgColors True if DMG colors should be used.
    void ForceDmgColors(bool useDmgColors) { forceDmgColors_ = useDmgColors; }

    /// @brief Use custom DMG palettes when playing GB games. Toggle through GUI.
    /// @param useDmgColors True if DMG colors should be used.
    void PreferDmgColors(bool useDmgColors) { preferDmgColors_ = useDmgColors; }

    /// @brief Determine whether background, window, obp0, and obp1 should use the same palette or individual ones.
    /// @param individualPalettes True if each pixel type should use its own palette.
    void UseIndividualPalettes(bool individualPalettes) { useIndividualPalettes_ = individualPalettes; }

    /// @brief Specify colors in one of the custom DMG palettes.
    /// @param index Index of palette to update.
    ///                 0 = Universal palette
    ///                 1 = Background
    ///                 2 = Window
    ///                 3 = OBP0
    ///                 4 = OBP1
    /// @param data Pointer to RGB data (12 0-255 values)
    void SetCustomPalette(uint8_t index, uint8_t* data);

    // Register access
    bool LCDEnabled() const { return LCDC_ & 0x80; }
    uint8_t STAT() const { return STAT_; }

    bool IsSerializable() const;
    void Serialize(std::ofstream& out);
    void Deserialize(std::ifstream& in);

private:
    uint16_t WindowTileMapBaseAddr() const { return (LCDC_ & 0x40) ? 0x9C00 : 0x9800; }
    bool WindowEnabled() const { return LCDC_ & 0x20; }
    bool BackgroundAndWindowTileAddrMode() const { return LCDC_ & 0x10; }
    uint16_t BackgroundTileMapBaseAddr() const { return (LCDC_ & 0x08) ? 0x9C00 : 0x9800; }
    bool TallSpriteMode() const { return LCDC_ & 0x04; }
    bool SpritesEnabled() const { return LCDC_ & 0x02; }
    bool WindowAndBackgroundEnabled() const { return LCDC_ & 0x01; }
    bool SpriteMasterPriority() const { return !(LCDC_ & 0x01); }

    uint8_t ReadIoReg(uint8_t ioAddr) const;
    void WriteIoReg(uint8_t ioAddr, uint8_t data);

    // LCD Registers
    uint8_t LCDC_;  // LCD control
    uint8_t STAT_;  // LCD status
    uint8_t SCY_;  // Viewport Y position
    uint8_t SCX_;  // Viewport X position
    uint8_t LY_;  // LCD Y coordinate
    uint8_t LYC_;  // LY compare
    uint8_t WY_;  // Window y position
    uint8_t WX_;  // Window x position

    // Palette
    uint8_t BGP_;  // BG palette data (DMG)
    uint8_t OBP0_;  // OBJ palette 0 data (DMG)
    uint8_t OBP1_;  // OBJ palette 1 data (DMG)
    uint8_t BCPS_;  // Background color palette specification (CGB mode only)
    uint8_t OCPS_;  // OBJ color palette specification (CGB mode only)
    std::array<uint8_t, 0x40> BG_CRAM_;  // BG palette data (CGB)
    std::array<uint8_t, 0x40> OBJ_CRAM_;  // OBJ palette data (CGB)

    // OAM
    uint8_t OPRI_;  // OBJ priority mode (CGB)
    std::array<uint8_t, 0xA0> OAM_;  // OAM memory

    // VRAM
    uint8_t VBK_;  // VRAM Bank (CGB)
    std::array<std::array<uint8_t, 0x2000>, 2> VRAM_;

    // STAT modes
    void SetMode(uint8_t mode) { STAT_ = (STAT_ & 0xFC) | (mode & 0x03); }
    void SetLYC()
    {
        if (LY_ == LYC_)
        {
            STAT_ |= 0x04;
        }
        else
        {
            STAT_ &= 0xFB;
        }
    }

    // GUI overrides
    bool preferDmgColors_;
    bool useIndividualPalettes_;

    // Disabled state
    void DisabledClock();

    // Rendering
    void RenderPixel(Pixel pixel);
    void RenderDmgPixel(Pixel pixel);
    void OamScan();

    // Data from bus
    bool const& cgbMode_;
    uint8_t* frameBuffer_;
    uint32_t framePointer_;

    // State
    uint16_t dot_;
    uint8_t LX_;
    uint8_t windowY_;
    bool frameReady_;
    bool vBlank_;
    bool wyCondition_;
    bool forceDmgColors_;
    bool oamDmaInProgress_;

    // Disabled state
    uint8_t disabledY_;
    bool firstEnabledFrame_;

    // FIFO
    friend class PixelFIFO;
    std::unique_ptr<PixelFIFO> pixelFifoPtr_;
};
