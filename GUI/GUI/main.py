import controller
import sdl_video
import ctypes
import sys
from sdl2 import *

FPS = 60
TIME_PER_FRAME = 1000 // 60

LOG_PATH = ctypes.create_string_buffer(b"./logs/")
SAVE_PATH = ctypes.create_string_buffer(b"./saves/")
BOOT_ROM_PATH = ctypes.create_string_buffer(b"./boot/")

def main(rom_path: bytes):
    window, renderer = sdl_video.initialize_sdl_video()

    GAME_BOY = ctypes.CDLL("./GameBoy/lib/libGameBoy.dll", winmode=0)
    GAME_BOY.Initialize(sdl_video.FRAME_BUFFER, LOG_PATH, SAVE_PATH, BOOT_ROM_PATH)

    if rom_path:
        rom_path = ctypes.create_string_buffer(rom_path)
        GAME_BOY.InsertCartridge(rom_path)

    GAME_BOY.PowerOn()

    running = True
    event = SDL_Event()

    while running:
        start_time = SDL_GetTicks64()

        while SDL_PollEvent(ctypes.byref(event)) != 0:
            if event.type == SDL_QUIT:
                running = False
                break

        joypad = controller.get_joypad()
        GAME_BOY.SetInputs(joypad.down, joypad.up, joypad.left, joypad.right, joypad.start, joypad.select, joypad.b, joypad.a)

        while not GAME_BOY.FrameReady():
            GAME_BOY.Clock()

        sdl_video.update_screen(renderer)

        frame_time = SDL_GetTicks64() - start_time

        if frame_time < TIME_PER_FRAME:
            SDL_Delay(TIME_PER_FRAME - frame_time)

    SDL_DestroyRenderer(renderer)
    SDL_DestroyWindow(window)
    SDL_Quit()
    GAME_BOY.PowerOff()

    return 0

if __name__ == '__main__':
    rom_path = b""

    if len(sys.argv) > 1:
        rom_path = str.encode(sys.argv[1])

    sys.exit(main(rom_path))
