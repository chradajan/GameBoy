#pragma once

#include <Cartridge/Cartridge.hpp>
#include <array>
#include <cstdint>
#include <fstream>

namespace fs = std::filesystem;

class MBC0 : public virtual Cartridge
{
public:
    MBC0(std::array<uint8_t, 0x4000> const& bank0,
         std::ifstream& rom,
         fs::path savePath,
         uint8_t cartridgeType,
         uint8_t ramBankCount);

    ~MBC0();

    void Reset() override;

    uint8_t ReadROM(uint16_t addr) override;
    void WriteROM(uint16_t addr, uint8_t data) override;

    uint8_t ReadRAM(uint16_t addr) override;
    void WriteRAM(uint16_t addr, uint8_t data) override;

    void SaveRAM() override;

    void Serialize(std::ofstream& out) override;
    void Deserialize(std::ifstream& in) override;

private:
    std::array<std::array<uint8_t, 0x4000>, 2> ROM_;
    std::array<uint8_t, 0x2000> RAM_;
};
