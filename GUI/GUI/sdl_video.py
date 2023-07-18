import ctypes
from sdl2 import *

WIDTH = 160
HEIGHT = 144
CHANNELS = 3
DEPTH = CHANNELS * 8
PITCH = WIDTH * CHANNELS
BUFFER_SIZE = WIDTH * HEIGHT * CHANNELS
WINDOW_SCALE = 4

FRAME_BUFFER = (ctypes.c_uint8 * BUFFER_SIZE)()

def initialize_sdl_video() -> tuple:
    window = SDL_CreateWindow(b"GameBoy",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              WIDTH * WINDOW_SCALE,
                              HEIGHT * WINDOW_SCALE,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI)

    SDL_SetWindowMinimumSize(window, WIDTH * 2, HEIGHT * 2)
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, b"0")
    SDL_SetWindowTitle(window, b"GameBoy")
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED)

    return (window, renderer)

def update_screen(renderer):
    surface = SDL_CreateRGBSurfaceFrom(FRAME_BUFFER,
                                       WIDTH,
                                       HEIGHT,
                                       DEPTH,
                                       PITCH,
                                       0x0000FF,
                                       0x00FF00,
                                       0xFF0000,
                                       0)

    texture = SDL_CreateTextureFromSurface(renderer, surface)
    SDL_RenderCopy(renderer, texture, None, None)
    SDL_RenderPresent(renderer)
    SDL_FreeSurface(surface)
    SDL_DestroyTexture(texture)
