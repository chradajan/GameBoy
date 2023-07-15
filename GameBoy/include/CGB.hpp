#pragma once

#include <cstdint>

extern "C"
{
void Initialize(uint8_t* frameBuffer,
                char* logPath,
                char* savePath,
                char* bootRomPath);

void InsertCartridge(char* romPath);

void PowerOn();

void PowerOff();

void Run();

void SetInputs(bool down, bool up, bool left, bool right, bool start, bool select, bool b, bool a);
}
