#include <Cartridge/MBC3.hpp>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

MBC3::MBC3(std::array<uint8_t, 0x4000> const& bank0,
         std::ifstream& rom,
         fs::path savePath,
         uint8_t cartridgeType,
         uint16_t romBankCount,
         uint8_t ramBankCount) :
    romBankCount_(romBankCount),
    ramBankCount_(ramBankCount)
{
    savePath_ = savePath;
    batteryBacked_ = (cartridgeType == 0x0F) || (cartridgeType == 0x10) || (cartridgeType == 0x13);
    containsRTC_ = (cartridgeType == 0x0F) || (cartridgeType == 0x10);
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

            if (containsRTC_)
            {
                save.read(reinterpret_cast<char*>(&S_internal_), sizeof(S_internal_));
                save.read(reinterpret_cast<char*>(&M_internal_), sizeof(M_internal_));
                save.read(reinterpret_cast<char*>(&H_internal_), sizeof(H_internal_));
                save.read(reinterpret_cast<char*>(&DL_internal_), sizeof(DL_internal_));
                save.read(reinterpret_cast<char*>(&DH_internal_), sizeof(DH_internal_));
                rtcHalted_ = DH_internal_ & 0x40;

                std::chrono::system_clock::rep serializedTime{};
                save.read(reinterpret_cast<char*>(&serializedTime), sizeof(serializedTime));
                referencePoint_ = std::chrono::system_clock::time_point{std::chrono::system_clock::duration{serializedTime}};
            }
        }
    }
    else
    {
        for (auto& bank : RAM_)
        {
            bank.fill(0x00);
        }

        rtcHalted_ = false;
        referencePoint_ = std::chrono::system_clock::now();
        S_internal_ = 0x00;
        M_internal_ = 0x00;
        H_internal_ = 0x00;
        DL_internal_ = 0x00;
        DH_internal_ = 0x00;
    }

    Reset();
}

MBC3::~MBC3()
{
    SaveRAM();
}

void MBC3::Reset()
{
    romBank_ = 1;
    ramBank_ = 0;
    ramEnabled_ = false;

    S_ = 0x00;
    M_ = 0x00;
    H_ = 0x00;
    DL_ = 0x00;
    DH_ = 0x00;

    latchInitiated_ = false;
}

uint8_t MBC3::ReadROM(uint16_t addr)
{
    if (addr < 0x4000)
    {
        return ROM_[0][addr];
    }
    else
    {
        return ROM_[romBank_][addr - 0x4000];
    }
}

void MBC3::WriteROM(uint16_t addr, uint8_t data)
{
    if (addr < 0x2000)
    {
        ramEnabled_ = ((data & 0x0A) == 0x0A);
    }
    else if (addr < 0x4000)
    {
        romBank_ = data % romBankCount_;

        if (romBank_ == 0x00)
        {
            romBank_ = 0x01;
        }
    }
    else if (addr < 0x6000)
    {
        if (data < 0x04)
        {
            if (containsRAM_)
            {
                ramBank_ = data % ramBankCount_;
            }
        }
        else
        {
            ramBank_ = data;
        }
    }
    else
    {
        if (!containsRTC_)
        {
            return;
        }
        else if (!data && !latchInitiated_)
        {
            latchInitiated_ = true;
        }
        else if ((data == 0x01) && latchInitiated_)
        {
            latchInitiated_ = false;

            if (!rtcHalted_)
            {
                UpdateInternalRTC();
            }

            S_ = S_internal_;
            M_ = M_internal_;
            H_ = H_internal_;
            DL_ = DL_internal_;
            DH_ = DH_internal_;
        }
    }
}

uint8_t MBC3::ReadRAM(uint16_t addr)
{
    if (!ramEnabled_)
    {
        return 0xFF;
    }
    else if (containsRAM_ && (ramBank_ < 0x04))
    {
        return RAM_[ramBank_][addr - 0xA000];
    }
    else if (containsRTC_)
    {
        switch (ramBank_)
        {
            case 0x08:
                return S_;
            case 0x09:
                return M_;
            case 0x0A:
                return H_;
            case 0x0B:
                return DL_;
            case 0x0C:
                return DH_ | 0x3E;
            default:
                break;
        }
    }

    return 0xFF;
}

void MBC3::WriteRAM(uint16_t addr, uint8_t data)
{
    if (!ramEnabled_)
    {
        return;
    }
    else if (containsRAM_ && (ramBank_ < 0x04))
    {
        RAM_[ramBank_][addr - 0xA000] = data;
    }
    else if (containsRTC_)
    {
        if (!rtcHalted_)
        {
            UpdateInternalRTC();
        }

        switch (ramBank_)
        {
            case 0x08:
                S_internal_ = data;
                break;
            case 0x09:
                M_internal_ = data;
                break;
            case 0x0A:
                H_internal_ = data;
                break;
            case 0x0B:
                DL_internal_ = data;
                break;
            case 0x0C:
            {
                DH_internal_ = data;
                bool initiateHalt = data & 0x40;

                if (!rtcHalted_ && initiateHalt)
                {
                    rtcHalted_ = true;
                }
                else if (rtcHalted_ && !initiateHalt)
                {
                    rtcHalted_ = false;
                    referencePoint_ = std::chrono::system_clock::now();
                }

                break;
            }
            default:
                break;
        }
    }
}

void MBC3::SaveRAM()
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

            if (containsRTC_)
            {
                save.write(reinterpret_cast<char*>(&S_internal_), sizeof(S_internal_));
                save.write(reinterpret_cast<char*>(&M_internal_), sizeof(M_internal_));
                save.write(reinterpret_cast<char*>(&H_internal_), sizeof(H_internal_));
                save.write(reinterpret_cast<char*>(&DL_internal_), sizeof(DL_internal_));
                save.write(reinterpret_cast<char*>(&DH_internal_), sizeof(DH_internal_));

                std::chrono::system_clock::rep serializedTime = referencePoint_.time_since_epoch().count();
                save.write(reinterpret_cast<char*>(&serializedTime), sizeof(serializedTime));
            }
        }
    }
}

void MBC3::UpdateInternalRTC()
{
    TimePoint now = std::chrono::system_clock::now();
    auto secondsElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - referencePoint_).count();
    referencePoint_ = now;

    uint_fast16_t D_internal = ((DH_internal_ & 0x01) << 8) | DL_internal_;
    uint_fast32_t currentRegTime = S_internal_ + (M_internal_ * 60) + (H_internal_ * 3600) + (D_internal * 86400) + secondsElapsed;

    uint_fast32_t days = currentRegTime / 86400;
    currentRegTime %= 86400;

    H_internal_ = currentRegTime / 3600;
    currentRegTime %= 3600;

    M_internal_ = currentRegTime / 60;
    currentRegTime %= 60;

    S_internal_ = currentRegTime;

    if (days > 0x1FF)
    {
        DH_internal_ |= 0x80;
    }

    DH_internal_ = (DH_internal_ & 0xFE) | ((days & 0x0100) >> 8);
    DL_internal_ = days & 0xFF;
}
