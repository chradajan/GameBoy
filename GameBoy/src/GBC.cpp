#include <GBC.hpp>
#include <GameBoy.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>

std::unique_ptr<GameBoy> gb = std::make_unique<GameBoy>();
void (*frameUpdateCallback)() = nullptr;

int SAMPLE_RATE = 44100;
float SAMPLE_PERIOD = 1.0 / SAMPLE_RATE;
constexpr int CPU_CLOCK_FREQUENCY = 1048576;
int EMULATED_CPU_FREQUENCY = CPU_CLOCK_FREQUENCY;
float CPU_CLOCK_PERIOD = 1.0 / EMULATED_CPU_FREQUENCY;

// Save states
bool createSaveState = false;
bool loadSaveState = false;
std::filesystem::path saveStatePathFS = "";


void Initialize(uint8_t* frameBuffer,
                void(*updateScreen)(),
                char* const savePath,
                char* const bootRomPath)
{
    frameUpdateCallback = updateScreen;
    gb->Initialize(frameBuffer, savePath, bootRomPath);
}

bool InsertCartridge(char* romPath, char* romName)
{
    return gb->InsertCartridge(romPath, romName);
}

void PowerOn()
{
    gb->PowerOn();
}

void PowerOff()
{
    gb.reset();
}

void CollectAudioSamples(float* buffer, int numSamples)
{
    static float audioTime = 0.0;

    for (int i = 0; i < numSamples; i += 2)
    {
        int mCycles = ((SAMPLE_PERIOD - audioTime) / CPU_CLOCK_PERIOD) + 1;
        gb->Clock(mCycles);
        audioTime = (mCycles * CPU_CLOCK_PERIOD) - SAMPLE_PERIOD;

        if (frameUpdateCallback && gb->FrameReady())
        {
            frameUpdateCallback();

            if (createSaveState && gb->IsSerializable())
            {
                std::ofstream out(saveStatePathFS, std::ios::binary);
                createSaveState = false;

                if (!out.fail())
                {
                    gb->Serialize(out);
                }
            }
            else if (loadSaveState && gb->IsSerializable())
            {
                std::ifstream in(saveStatePathFS, std::ios::binary);
                loadSaveState = false;

                if (!in.fail())
                {
                    gb->Deserialize(in);
                }
            }
        }

        float left, right;
        gb->GetAudioSample(&left, &right);
        buffer[i] = left;
        buffer[i + 1] = right;
    }
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
    gb->SetButtons(down, up, left, right, start, select, b, a);
}

void SetClockMultiplier(float const multiplier)
{
    EMULATED_CPU_FREQUENCY = CPU_CLOCK_FREQUENCY * multiplier;
    CPU_CLOCK_PERIOD = 1.0 / EMULATED_CPU_FREQUENCY;
}

void CreateSaveState(char* saveStatePath)
{
    createSaveState = true;
    saveStatePathFS = saveStatePath;
}

void LoadSaveState(char* saveStatePath)
{
    loadSaveState = true;
    saveStatePathFS = saveStatePath;
}

void EnableSoundChannel(int const channel, bool const enabled)
{
    gb->EnableSoundChannel(channel, enabled);
}

void SetMonoAudio(bool const monoAudio)
{
    gb->SetMonoAudio(monoAudio);
}

void SetVolume(float const volume)
{
    gb->SetVolume(volume);
}

void SetSampleRate(int const sampleRate)
{
    SAMPLE_RATE = sampleRate;
    SAMPLE_PERIOD = 1.0 / SAMPLE_RATE;
}

void PreferDmgColors(bool useDmgColors)
{
    gb->PreferDmgColors(useDmgColors);
}

void UseIndividualPalettes(bool individualPalettes)
{
    gb->UseIndividualPalettes(individualPalettes);
}

void SetCustomPalette(uint8_t index, uint8_t* data)
{
    gb->SetCustomPalette(index, data);
}
