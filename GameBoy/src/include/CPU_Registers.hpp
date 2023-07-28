#pragma once

#include <cstdint>
#include <fstream>

constexpr uint8_t ZERO_FLAG = 0x80;
constexpr uint8_t SUBTRACTION_FLAG = 0x40;
constexpr uint8_t HALF_CARRY_FLAG = 0x20;
constexpr uint8_t CARRY_FLAG = 0x10;

class CPU_Registers
{
public:
    CPU_Registers() :
        A(AF_.high_), F(AF_.low_), AF(AF_.pair_),
        B(BC_.high_), C(BC_.low_), BC(BC_.pair_),
        D(DE_.high_), E(DE_.low_), DE(DE_.pair_),
        H(HL_.high_), L(HL_.low_), HL(HL_.pair_)
    {
        Reset();
    }

    void Reset()
    {
        A = 0x11;
        F = 0x80;
        B = 0x00;
        C = 0x00;
        D = 0xFF;
        E = 0x56;
        H = 0x00;
        L = 0x0D;
        PC = 0x0100;
        SP = 0xFFFE;
    }

    void Serialize(std::ofstream& out);
    void Deserialize(std::ifstream& in);

    uint8_t& A;
    uint8_t& F;
    uint16_t& AF;

    uint8_t& B;
    uint8_t& C;
    uint16_t& BC;

    uint8_t& D;
    uint8_t& E;
    uint16_t& DE;

    uint8_t& H;
    uint8_t& L;
    uint16_t& HL;

    uint16_t SP;
    uint16_t PC;

    bool IsZeroFlagSet() const { return (F & ZERO_FLAG) == ZERO_FLAG; }
    bool IsSubtractionFlagSet() const { return (F & SUBTRACTION_FLAG) == SUBTRACTION_FLAG; }
    bool IsHalfCarryFlagSet() const { return (F & HALF_CARRY_FLAG) == HALF_CARRY_FLAG; }
    bool IsCarryFlagSet() const { return (F & CARRY_FLAG) == CARRY_FLAG; }

    void SetZeroFlag(bool val)
    {
        if (val)
        {
            F |= ZERO_FLAG;
        }
        else
        {
            F &= ~ZERO_FLAG;
        }
    }

    void SetSubtractionFlag(bool val)
    {
        if (val)
        {
            F |= SUBTRACTION_FLAG;
        }
        else
        {
            F &= ~SUBTRACTION_FLAG;
        }
    }

    void SetHalfCarryFlag(bool val)
    {
        if (val)
        {
            F |= HALF_CARRY_FLAG;
        }
        else
        {
            F &= ~HALF_CARRY_FLAG;
        }
    }

    void SetCarryFlag(bool val)
    {
        if (val)
        {
            F |= CARRY_FLAG;
        }
        else
        {
            F &= ~CARRY_FLAG;
        }
    }

private:
    struct RegisterPair
    {
        union
        {
            struct
            {
                uint8_t low_;
                uint8_t high_;
            };

            uint16_t pair_;
        };
    };

    RegisterPair AF_;
    RegisterPair BC_;
    RegisterPair DE_;
    RegisterPair HL_;
};
