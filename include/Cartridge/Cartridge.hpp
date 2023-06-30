#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class Cartridge
{
public:
    virtual ~Cartridge() {}
    virtual void Reset() = 0;

    virtual uint8_t ReadROM(uint16_t addr) = 0;
    virtual void WriteROM(uint16_t addr, uint8_t data) = 0;

    virtual uint8_t ReadRAM(uint16_t addr) = 0;
    virtual void WriteRAM(uint16_t addr, uint8_t data) = 0;

protected:
    bool containsRAM_;
    bool batteryBacked_;
    fs::path savePath_;
};
