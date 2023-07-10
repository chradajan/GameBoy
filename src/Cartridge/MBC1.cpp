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
    romBankCount_ = romBanks;
    ramBankCount_ = ramBanks;

    ROM_.resize(romBanks);
    RAM_.resize(ramBanks);

    for (auto& bank : RAM_)
    {
        bank.fill(0x00);
    }

    std::copy(bank0.begin(), bank0.end(), ROM_[0].begin());

    for (size_t i = 1; i < ROM_.size(); ++i)
    {
        rom.read((char*)ROM_[i].data(), 0x4000);
    }

    if (batteryBacked_)
    {
        std::ifstream save(savePath_, std::ios::binary);

        if (!save.fail())
        {
            for (auto& bank : RAM_)
            {
                save.read((char*)bank.data(), 0x2000);
            }
        }
    }

    if (romBanks <= 16)
    {
        romBankMask_ = romBanks - 1;
    }
    else
    {
        romBankMask_ = 0x1F;
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
            for (auto& bank : RAM_)
            {
                save.write((char*)bank.data(), 0x2000);
            }
        }
    }
}

void MBC1::Reset()
{
    ramEnabled_ = false;
    romBank_ = 1;
    ramBank_ = 0;
    advancedBankMode_ = false;
}

uint8_t MBC1::ReadROM(uint16_t addr)
{
    if (addr < 0x4000)
    {
        if (advancedBankMode_ && (romBankCount_ > 32))
        {
            uint_fast8_t bank = ramBank_ * 0x20;

            if (bank > romBankCount_)
            {
                bank = romBankCount_ - 1;
            }

            return ROM_[bank][addr];
        }
        else
        {
            return ROM_[0][addr];
        }
    }
    else
    {
        addr -= 0x4000;

        if (romBankCount_ > 32)
        {
            uint_fast16_t fullAddr = (ramBank_ << 19) | (romBank_ << 9) | addr;
            uint_fast16_t bank = fullAddr / 0x4000;

            if (bank > romBankCount_)
            {
                bank = romBankCount_ - 1;
            }

            return ROM_[bank][addr];
        }
        else
        {
            return ROM_[romBank_][addr];
        }
    }

    return 0xFF;
}

void MBC1::WriteROM(uint16_t addr, uint8_t data)
{
    if (addr < 0x2000)
    {
        ramEnabled_ = ((data & 0x0A) == 0x0A);
    }
    else if (addr < 0x4000)
    {
        uint_fast8_t maskedBankNum = data & romBankMask_;

        if ((data & 0x1F) == 0x00)
        {
            romBank_ = 0x01;
        }
        else
        {
            romBank_ = maskedBankNum;
        }
    }
    else if (addr < 0x6000)
    {
        ramBank_ = data & 0x03;
    }
    else
    {
        advancedBankMode_ = (data & 0x01);
    }
}

uint8_t MBC1::ReadRAM(uint16_t addr)
{
    if (containsRAM_ && ramEnabled_)
    {
        addr -= 0xA000;

        if (ramBankCount_ == 1)
        {
            return RAM_[0][addr];
        }
        else
        {
            return RAM_[ramBank_][addr];
        }
    }

    return 0xFF;
}

void MBC1::WriteRAM(uint16_t addr, uint8_t data)
{
    if (containsRAM_ && ramEnabled_)
    {
        addr -= 0xA000;

        if (ramBankCount_ == 1)
        {
            RAM_[0][addr] = data;
        }
        else
        {
            RAM_[ramBank_][addr] = data;
        }
    }
}
