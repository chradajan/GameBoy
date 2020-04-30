#include <cstdint>

class CPU
{
public:
private:
    CPU_Reg reg;
};

struct CPU_Reg
{
    uint8_t A, B, C, D, E, H, L;
    uint16_t BC = B_C();
    uint16_t DE = D_E();
    uint16_t HL = H_L();
private:
    uint16_t B_C();
    uint16_t D_E();
    uint16_t H_L();
};