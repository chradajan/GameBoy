#include <CGB.hpp>
#include <GameBoy.hpp>
#include <filesystem>
#include <iostream>
#include <memory>

std::unique_ptr<GameBoy> gb = std::make_unique<GameBoy>();

void Initialize(uint8_t* frameBuffer,
                char* const logPath,
                char* const savePath,
                char* const bootRomPath)
{
    std::filesystem::path logPathFS = logPath;
    std::filesystem::path savePathFS = savePath;
    std::filesystem::path bootRomPathFS = bootRomPath;

    if (!std::filesystem::is_directory(logPathFS))
    {
        std::filesystem::create_directory(logPathFS);
    }

    if (!std::filesystem::is_directory(savePathFS))
    {
        std::filesystem::create_directory(savePathFS);
    }

    gb->Initialize(frameBuffer, logPath, savePath, bootRomPath);
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

void Run()
{
    gb->Run();
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
