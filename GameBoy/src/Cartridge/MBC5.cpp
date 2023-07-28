#include <Cartridge/MBC5.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

MBC5::MBC5(std::array<uint8_t, 0x4000> const& bank0,
           std::ifstream& rom,
           fs::path savePath,
           uint8_t cartridgeType,
           uint16_t romBankCount,
           uint8_t ramBankCount) :
    romBankCount_(romBankCount),
    ramBankCount_(ramBankCount)
{
    savePath_ = savePath;
    batteryBacked_ = (cartridgeType == 0x1B) || (cartridgeType == 0x1E);
    containsRAM_ = ramBankCount > 0;

    ROM_.resize(romBankCount);
    RAM_.resize(ramBankCount);

    std::copy(bank0.begin(), bank0.end(), ROM_[0].begin());

    for (size_t i = 1; i < ROM_.size(); ++i)
    {
        rom.read(reinterpret_cast<char*>(ROM_[i].data()), 0x4000);
    }

    if (batteryBacked_ && !savePath_.empty())
    {
        std::ifstream save(savePath_, std::ios::binary);

        if (!save.fail())
        {
            for (auto& bank : RAM_)
            {
                save.read(reinterpret_cast<char*>(bank.data()), 0x2000);
            }
        }
    }
    else
    {
        for (auto& bank : RAM_)
        {
            bank.fill(0x00);
        }
    }

    Reset();
}

MBC5::~MBC5()
{
    SaveRAM();
}

void MBC5::Reset()
{
    romBankIndex_ = 0x0001;
    ramEnabled_ = false;
    romBankLsb_ = 0x01;
    romBankMsb_ = 0x00;
    ramBank_ = 0x00;
}

uint8_t MBC5::ReadROM(uint16_t addr)
{
    if (addr < 0x4000)
    {
        return ROM_[0][addr];
    }
    else
    {
        return ROM_[romBankIndex_][addr - 0x4000];
    }
}

void MBC5::WriteROM(uint16_t addr, uint8_t data)
{
    if (addr < 0x2000)
    {
        ramEnabled_ = (data & 0x0A) == 0x0A;
    }
    else if (addr < 0x4000)
    {
        if (addr < 0x3000)
        {
            romBankLsb_ = data;
        }
        else
        {
            romBankMsb_ = data;
        }

        romBankIndex_ = (((romBankMsb_ & 0x01) << 8) | romBankLsb_) % romBankCount_;
    }
    else if (containsRAM_ && (addr < 0x6000))
    {
        ramBank_ = data % ramBankCount_;
    }
}

uint8_t MBC5::ReadRAM(uint16_t addr)
{
    if (containsRAM_ && ramEnabled_)
    {
        return RAM_[ramBank_][addr - 0xA000];
    }

    return 0xFF;
}

void MBC5::WriteRAM(uint16_t addr, uint8_t data)
{
    if (containsRAM_ && ramEnabled_)
    {
        RAM_[ramBank_][addr - 0xA000] = data;
    }
}

void MBC5::SaveRAM()
{
    if (batteryBacked_ && !savePath_.empty())
    {
        std::ofstream save(savePath_, std::ios::binary);

        if (!save.fail())
        {
            for (auto& bank : RAM_)
            {
                save.write(reinterpret_cast<char*>(bank.data()), 0x2000);
            }
        }
    }
}
