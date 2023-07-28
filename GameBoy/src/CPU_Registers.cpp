#include <CPU_Registers.hpp>
#include <fstream>

void CPU_Registers::Serialize(std::ofstream& out)
{
    out.write(reinterpret_cast<char*>(&AF_), sizeof(AF_));
    out.write(reinterpret_cast<char*>(&BC_), sizeof(BC_));
    out.write(reinterpret_cast<char*>(&DE_), sizeof(DE_));
    out.write(reinterpret_cast<char*>(&HL_), sizeof(HL_));
    out.write(reinterpret_cast<char*>(&PC), sizeof(PC));
    out.write(reinterpret_cast<char*>(&SP), sizeof(SP));
}

void CPU_Registers::Deserialize(std::ifstream& in)
{
    in.read(reinterpret_cast<char*>(&AF_), sizeof(AF_));
    in.read(reinterpret_cast<char*>(&BC_), sizeof(BC_));
    in.read(reinterpret_cast<char*>(&DE_), sizeof(DE_));
    in.read(reinterpret_cast<char*>(&HL_), sizeof(HL_));
    in.read(reinterpret_cast<char*>(&PC), sizeof(PC));
    in.read(reinterpret_cast<char*>(&SP), sizeof(SP));
}
