#include <Cartridge/MBC0.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

MBC0::MBC0(std::array<uint8_t, 0x4000> const& bank0,
           std::ifstream& rom,
           fs::path savePath,
           uint8_t cartridgeType,
           uint8_t ramBankCount)
{
    savePath_ = savePath;
    batteryBacked_ = (cartridgeType == 0x09);
    containsRAM_ = (ramBankCount > 0);

    std::copy(bank0.begin(), bank0.end(), ROM_[0].begin());
    rom.read(reinterpret_cast<char*>(ROM_[1].data()), 0x4000);
    RAM_.fill(0x00);

    if (batteryBacked_)
    {
        std::ifstream save(savePath_, std::ios::binary);

        if (!save.fail())
        {
            save.read((char*)RAM_.data(), 0x2000);
        }
    }
}

MBC0::~MBC0()
{
    if (batteryBacked_)
    {
        std::ofstream save(savePath_, std::ios::binary);

        if (!save.fail())
        {
            save.write((char*)RAM_.data(), 0x2000);
        }
    }
}

void MBC0::Reset()
{

}

uint8_t MBC0::ReadROM(uint16_t addr)
{
    if (addr < 0x4000)
    {
        return ROM_[0][addr];
    }

    return ROM_[1][addr - 0x4000];
}

void MBC0::WriteROM(uint16_t addr, uint8_t data)
{
    (void)addr; (void)data;
}

uint8_t MBC0::ReadRAM(uint16_t addr)
{
    if (containsRAM_)
    {
        return RAM_[addr - 0xA000];
    }

    return 0xFF;
}

void MBC0::WriteRAM(uint16_t addr, uint8_t data)
{
    if (containsRAM_)
    {
        RAM_[addr - 0xA000] = data;
    }
}
