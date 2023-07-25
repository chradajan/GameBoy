import game_boy.game_boy as game_boy
import sdl2

WINDOW_SCALE = 4

def initialize_sdl_video() -> tuple:
    """Initialize an SDL window.

    Returns:
        Tuple of window and renderer.
    """
    sdl2.SDL_Init(sdl2.SDL_INIT_VIDEO | sdl2.SDL_INIT_AUDIO)
    window = sdl2.SDL_CreateWindow(b"GameBoy",
                              sdl2.SDL_WINDOWPOS_UNDEFINED,
                              sdl2.SDL_WINDOWPOS_UNDEFINED,
                              game_boy.WIDTH * WINDOW_SCALE,
                              game_boy.HEIGHT * WINDOW_SCALE,
                              sdl2.SDL_WINDOW_SHOWN | sdl2.SDL_WINDOW_ALLOW_HIGHDPI)

    sdl2.SDL_SetWindowMinimumSize(window, game_boy.WIDTH * 2, game_boy.HEIGHT * 2)
    sdl2.SDL_SetHint(sdl2.SDL_HINT_RENDER_SCALE_QUALITY, b"0")
    sdl2.SDL_SetWindowTitle(window, b"GameBoy")
    renderer = sdl2.SDL_CreateRenderer(window, -1, sdl2.SDL_RENDERER_ACCELERATED)

    return (window, renderer)

def update_screen(renderer):
    """Update the window with the current state of the frame buffer.

    Args:
        renderer: Renderer to update with frame buffer data.
    """
    surface = sdl2.SDL_CreateRGBSurfaceFrom(game_boy.FRAME_BUFFER,
                                            game_boy.WIDTH,
                                            game_boy.HEIGHT,
                                            game_boy.DEPTH,
                                            game_boy.PITCH,
                                            0x0000FF,
                                            0x00FF00,
                                            0xFF0000,
                                            0)

    texture = sdl2.SDL_CreateTextureFromSurface(renderer, surface)
    sdl2.SDL_RenderCopy(renderer, texture, None, None)
    sdl2.SDL_RenderPresent(renderer)
    sdl2.SDL_FreeSurface(surface)
    sdl2.SDL_DestroyTexture(texture)

def destroy_window_and_renderer(window, renderer):
    """Destroy the specified SDL renderer and window."""
    sdl2.SDL_DestroyRenderer(renderer)
    sdl2.SDL_DestroyWindow(window)
    sdl2.SDL_Quit()
