#include <GBC.hpp>
#include <GameBoy.hpp>
#include <memory>

std::unique_ptr<GameBoy> gb = std::make_unique<GameBoy>();
void (*frameUpdateCallback)() = nullptr;

constexpr int SAMPLE_RATE = 44100;
constexpr float SAMPLE_PERIOD = 1.0 / SAMPLE_RATE;
constexpr int CPU_CLOCK_FREQUENCY = 1048576;
int EMULATED_CPU_FREQUENCY = CPU_CLOCK_FREQUENCY;
float CPU_CLOCK_PERIOD = 1.0 / EMULATED_CPU_FREQUENCY;

void Initialize(uint8_t* frameBuffer,
                char* const savePath,
                char* const bootRomPath)
{
    gb->Initialize(frameBuffer, savePath, bootRomPath);
}

void InsertCartridge(char* romPath)
{
    std::filesystem::path romPathFS = romPath;

    gb->InsertCartridge(romPath);
}

void PowerOn()
{
    gb->PowerOn();
}

void PowerOff()
{
    gb.reset();
}

void Clock()
{
    gb->Clock();
}

bool FrameReady()
{
    return gb->FrameReady();
}

void GetAudioSample(float* left, float* right)
{
    static float audioTime = 0.0;

    while (audioTime < SAMPLE_PERIOD)
    {
        gb->Clock();
        audioTime += CPU_CLOCK_PERIOD;

        if (frameUpdateCallback && gb->FrameReady())
        {
            frameUpdateCallback();
        }
    }

    audioTime -= SAMPLE_PERIOD;

    gb->GetAudioSample(left, right);
}

void SetFrameReadyCallback(void(*callback)())
{
    frameUpdateCallback = callback;
}

void SetInputs(bool const down,
               bool const up,
               bool const left,
               bool const right,
               bool const start,
               bool const select,
               bool const b,
               bool const a)
{
    gb->SetInputs(down, up, left, right, start, select, b, a);
}

void SetClockMultiplier(float const multiplier)
{
    EMULATED_CPU_FREQUENCY = CPU_CLOCK_FREQUENCY * multiplier;
    CPU_CLOCK_PERIOD = 1.0 / EMULATED_CPU_FREQUENCY;
}
