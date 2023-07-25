#pragma once

#include <cstdint>

extern "C"
{
void Initialize(uint8_t* frameBuffer,
                char* savePath,
                char* bootRomPath);

void InsertCartridge(char* romPath);

void PowerOn();

void PowerOff();

void Clock();

bool FrameReady();

void GetAudioSample(float* left, float* right);

void SetInputs(bool down, bool up, bool left, bool right, bool start, bool select, bool b, bool a);
}
