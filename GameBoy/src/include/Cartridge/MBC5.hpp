#pragma once

#include <Cartridge/Cartridge.hpp>
#include <array>
#include <cstdint>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

class MBC5 : public virtual Cartridge
{
public:
    MBC5(std::array<uint8_t, 0x4000> const& bank0,
         std::ifstream& rom,
         fs::path savePath,
         uint8_t cartridgeType,
         uint16_t romBankCount,
         uint8_t ramBankCount);

    ~MBC5();

    void Reset() override;

    uint8_t ReadROM(uint16_t addr) override;
    void WriteROM(uint16_t addr, uint8_t data) override;

    uint8_t ReadRAM(uint16_t addr) override;
    void WriteRAM(uint16_t addr, uint8_t data) override;

    void SaveRAM() override;

    void Serialize(std::ofstream& out) override;
    void Deserialize(std::ifstream& in) override;

private:
    // Memory
    std::vector<std::array<uint8_t, 0x4000>> ROM_;
    std::vector<std::array<uint8_t, 0x2000>> RAM_;

    // Cart info
    uint16_t romBankCount_;
    uint8_t ramBankCount_;
    uint16_t romBankIndex_;

    // Registers
    bool ramEnabled_;
    uint8_t romBankLsb_;
    uint8_t romBankMsb_;
    uint8_t ramBank_;
};
