#pragma once

#include <Cartridge/Cartridge.hpp>
#include <array>
#include <cstdint>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

class MBC1 : public virtual Cartridge
{
public:
    MBC1(std::array<uint8_t, 0x4000> const& bank0,
         std::ifstream& rom,
         fs::path savePath,
         uint8_t cartridgeType,
         uint16_t romBanks,
         uint8_t ramBanks);
    ~MBC1();

    void Reset() override;

    uint8_t ReadROM(uint16_t addr) override;
    void WriteROM(uint16_t addr, uint8_t data) override;

    uint8_t ReadRAM(uint16_t addr) override;
    void WriteRAM(uint16_t addr, uint8_t data) override;

private:
    std::vector<std::array<uint8_t, 0x4000>> ROM_;
    std::vector<std::array<uint8_t, 0x2000>> RAM_;

// Registers
private:
    bool ramEnabled_;
    uint8_t romBank_;
    uint8_t romBankMask_;
    uint16_t romBankCount_;

    uint8_t ramBank_;
    uint8_t ramBankCount_;

    bool advancedBankMode_;
};
