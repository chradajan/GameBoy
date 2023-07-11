#include <GameWindow.hpp>
#include <Controller.hpp>
#include <GameBoy.hpp>
#include <SDL2/SDL.h>
#include <cstdint>

GameWindow::GameWindow(GameBoy* gameBoyPtr, uint8_t* frameBuffer) :
    gameBoyPtr_(gameBoyPtr),
    frameBuffer_(frameBuffer)
{
    InitializeSDL();
}

GameWindow::~GameWindow()
{
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
    SDL_Quit();
}

void GameWindow::Run()
{
    bool quit = false;
    SDL_Event e;

    while (!quit)
    {
        uint64_t startTime = SDL_GetTicks64();

        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }
        }

        gameBoyPtr_->SetInputs(GetButtons());
        gameBoyPtr_->Run();
        uint64_t frameTicks = SDL_GetTicks64() - startTime;

        if (frameTicks < TICKS_PER_FRAME)
        {
            SDL_Delay(TICKS_PER_FRAME - frameTicks);
        }

        UpdateScreen();
    }
}

void GameWindow::InitializeSDL()
{
    SDL_Init(SDL_INIT_VIDEO);

    window_ = SDL_CreateWindow("GameBoy",
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               SCREEN_WIDTH * WINDOW_SCALE,
                               SCREEN_HEIGHT * WINDOW_SCALE,
                               SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    SDL_SetWindowMinimumSize(window_, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_SetWindowTitle(window_, "GameBoy");

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
}

void GameWindow::UpdateScreen()
{
    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(frameBuffer_,
                                                    SCREEN_WIDTH,
                                                    SCREEN_HEIGHT,
                                                    DEPTH,
                                                    PITCH,
                                                    0x0000FF,
                                                    0x00FF00,
                                                    0xFF0000,
                                                    0);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_RenderCopy(renderer_, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}
