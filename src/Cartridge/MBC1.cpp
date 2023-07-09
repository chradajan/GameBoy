#include <Cartridge/MBC1.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

MBC1::MBC1(std::array<uint8_t, 0x4000> const& bank0,
           std::ifstream& rom,
           fs::path savePath,
           uint8_t cartridgeType,
           uint16_t romBanks,
           uint8_t ramBanks)
{
    savePath_ = savePath;
    batteryBacked_ = (cartridgeType == 0x03);
    containsRAM_ = (ramBanks > 0);

    ROM_.resize(romBanks * 0x4000);
    RAM_.resize(ramBanks * 0x2000, 0x00);

    std::copy(bank0.begin(), bank0.end(), ROM_.begin());
    rom.read((char*)(ROM_.data() + 0x4000), (romBanks - 1) * 0x4000);

    if (batteryBacked_)
    {
        std::ifstream save(savePath_, std::ios::binary);

        if (!save.fail())
        {
            save.read((char*)RAM_.data(), RAM_.size());
        }
    }

    Reset();
}

MBC1::~MBC1()
{
    if (batteryBacked_)
    {
        std::ofstream save(savePath_, std::ios::binary);

        if (!save.fail())
        {
            save.write((char*)RAM_.data(), RAM_.size());
        }
    }
}

void MBC1::Reset()
{
    lowBankReg_ = 0x00;
    highBankReg_ = 0x01;
    ramEnabled_ = false;
    mode1_ = false;
}

uint8_t MBC1::ReadROM(uint16_t addr)
{
    size_t actualAddr = addr;

    if (addr < 0x4000)
    {
        if (mode1_)
        {
            actualAddr = ((highBankReg_ << 19) | addr) & (ROM_.size() - 1);
        }
    }
    else
    {
        actualAddr = ((highBankReg_ << 19) | (lowBankReg_ << 14) | addr) & (ROM_.size() - 1);
    }

    return ROM_[actualAddr];
}

void MBC1::WriteROM(uint16_t addr, uint8_t data)
{
    if (addr < 0x2000)
    {
        ramEnabled_ = (data & 0x0A) == 0x0A;
    }
    else if (addr < 0x4000)
    {
        lowBankReg_ = data & 0x1F;

        if (lowBankReg_ == 0x00)
        {
            lowBankReg_ = 0x01;
        }
    }
    else if (addr < 0x6000)
    {
        highBankReg_ = data & 0x03;
    }
    else
    {
        mode1_ = data == 1;
    }
}

uint8_t MBC1::ReadRAM(uint16_t addr)
{
    if (containsRAM_ && ramEnabled_)
    {
        addr -= 0xA000;

        if (mode1_)
        {
            addr |= (highBankReg_ << 13);
            addr &= (RAM_.size() - 1);
        }

        return RAM_[addr];
    }

    return 0xFF;
}

void MBC1::WriteRAM(uint16_t addr, uint8_t data)
{
    if (containsRAM_ && ramEnabled_)
    {
        addr -= 0xA000;

        if (mode1_)
        {
            addr |= (highBankReg_ << 13);
            addr &= (RAM_.size() - 1);
        }

        RAM_[addr] = data;
    }
}
