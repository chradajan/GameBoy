#pragma once
#include <SDL2/SDL.h>

constexpr int SCREEN_WIDTH = 160;
constexpr int SCREEN_HEIGHT = 144;
constexpr int CHANNELS = 3;
constexpr int DEPTH = CHANNELS * 8;
constexpr int PITCH = SCREEN_WIDTH * CHANNELS;
constexpr int WINDOW_SCALE = 4;
constexpr int FPS = 60;
constexpr int TICKS_PER_FRAME = 1000 / FPS;

class GameBoy;

class GameWindow
{
public:
    GameWindow(GameBoy* gameBoyPtr, uint8_t* frameBuffer);
    ~GameWindow();

    void Run();

private:
    // SDL
    void InitializeSDL();
    void UpdateScreen();

    // GameBoy
    GameBoy* const gameBoyPtr_;
    uint8_t* const frameBuffer_;

    // SDL data
    SDL_Window* window_;
    SDL_Renderer* renderer_;
};
