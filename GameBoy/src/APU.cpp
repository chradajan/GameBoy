#include <APU.hpp>

void APU::Clock()
{
    channel2_.Clock();
}

float APU::GetSample()
{
    float channel2Sample = channel2_.GetSample();
    return channel2Sample;
}

void APU::ClockDIV(bool const doubleSpeed)
{
    ++divCounter_;

    if (divCounter_ == 64)
    {
        bool wasApuDivBitSet = DIV_ & (doubleSpeed ? 0x20 : 0x10);

        ++DIV_;
        divCounter_ = 0;
        bool isApuDivBitSet = DIV_ & (doubleSpeed ? 0x20 : 0x10);

        if (wasApuDivBitSet && !isApuDivBitSet)
        {
            channel2_.ApuDiv();
        }
    }
}

void APU::ResetDIV(bool const doubleSpeed)
{
    if (DIV_ & (doubleSpeed ? 0x20 : 0x10))
    {
        channel2_.ApuDiv();
    }

    DIV_ = 0;
    divCounter_ = 0;
}

uint8_t APU::Read(uint8_t ioAddr)
{
    switch (ioAddr)
    {
        case 0x16 ... 0x19:
            return channel2_.Read(ioAddr);
        default:
            return 0xFF;
    }
}

void APU::Write(uint8_t ioAddr, uint8_t data)
{
    switch (ioAddr)
    {
    case 0x16 ... 0x19:
        channel2_.Write(ioAddr, data);
        break;
    default:
        break;
    }
}

void APU::Reset()
{
    divCounter_ = 0;
    channel2_.Reset();
}
