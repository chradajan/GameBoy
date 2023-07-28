#pragma once

#include <Cartridge/Cartridge.hpp>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;
using TimePoint = std::chrono::system_clock::time_point;

class MBC3 : public virtual Cartridge
{
public:
    MBC3(std::array<uint8_t, 0x4000> const& bank0,
         std::ifstream& rom,
         fs::path savePath,
         uint8_t cartridgeType,
         uint16_t romBankCount,
         uint8_t ramBankCount);

    ~MBC3();

    void Reset() override;

    uint8_t ReadROM(uint16_t addr) override;
    void WriteROM(uint16_t addr, uint8_t data) override;

    uint8_t ReadRAM(uint16_t addr) override;
    void WriteRAM(uint16_t addr, uint8_t data) override;

    void SaveRAM() override;

    void Serialize(std::ofstream& out) override;
    void Deserialize(std::ifstream& in) override;

private:
    void UpdateInternalRTC();

    std::vector<std::array<uint8_t, 0x4000>> ROM_;
    std::vector<std::array<uint8_t, 0x2000>> RAM_;

    // Bank counts
    uint16_t romBankCount_;
    uint8_t ramBankCount_;

    // Bank registers
    uint8_t romBank_;
    uint8_t ramBank_;
    bool ramEnabled_;

    // RTC
    bool containsRTC_;
    bool rtcHalted_;
    bool latchInitiated_;
    TimePoint referencePoint_;

    // RTC registers
    uint8_t S_;
    uint8_t M_;
    uint8_t H_;
    uint8_t DL_;
    uint8_t DH_;

    uint8_t S_internal_;
    uint8_t M_internal_;
    uint8_t H_internal_;
    uint8_t DL_internal_;
    uint8_t DH_internal_;
};
