from dataclasses import dataclass
from sdl2 import *

@dataclass
class JoyPad:
    """Current joypad state."""
    down: bool
    up : bool
    left: bool
    right: bool
    start: bool
    select: bool
    b: bool
    a: bool

def get_joypad() -> JoyPad:
    key_states = SDL_GetKeyboardState(None)
    return JoyPad(key_states[SDL_SCANCODE_S],
                  key_states[SDL_SCANCODE_W],
                  key_states[SDL_SCANCODE_A],
                  key_states[SDL_SCANCODE_D],
                  key_states[SDL_SCANCODE_RETURN],
                  key_states[SDL_SCANCODE_RSHIFT],
                  key_states[SDL_SCANCODE_K],
                  key_states[SDL_SCANCODE_L])
