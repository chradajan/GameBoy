#include <Controller.hpp>
#include <SDL2/SDL.h>

Buttons GetButtons()
{
    uint8_t const* keyStates = SDL_GetKeyboardState(nullptr);
    Buttons buttons;

    buttons.down = keyStates[SDL_SCANCODE_S];
    buttons.up = keyStates[SDL_SCANCODE_W];
    buttons.left = keyStates[SDL_SCANCODE_A];
    buttons.right = keyStates[SDL_SCANCODE_D];
    buttons.start = keyStates[SDL_SCANCODE_RETURN];
    buttons.select = keyStates[SDL_SCANCODE_RSHIFT];
    buttons.b = keyStates[SDL_SCANCODE_K];
    buttons.a = keyStates[SDL_SCANCODE_L];

    return buttons;
}
