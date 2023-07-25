import game_boy.game_boy as game_boy
from sdl2 import *

def get_joypad() -> game_boy.JoyPad:
    """Get the state of each key.

    Returns:
        State of each mapped joypad button.
    """
    key_states = SDL_GetKeyboardState(None)
    return game_boy.JoyPad(key_states[SDL_SCANCODE_S],
                           key_states[SDL_SCANCODE_W],
                           key_states[SDL_SCANCODE_A],
                           key_states[SDL_SCANCODE_D],
                           key_states[SDL_SCANCODE_RETURN],
                           key_states[SDL_SCANCODE_RSHIFT],
                           key_states[SDL_SCANCODE_K],
                           key_states[SDL_SCANCODE_L])
