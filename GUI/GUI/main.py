import controller
import sdl_audio
import sdl_video
import ctypes
import sdl2
import sys
from GameBoy import GAME_BOY

LOG_PATH = ctypes.create_string_buffer(b"./logs/")
SAVE_PATH = ctypes.create_string_buffer(b"./saves/")
BOOT_ROM_PATH = ctypes.create_string_buffer(b"./boot/")

def main(rom_path: bytes):
    sdl2.SDL_Init(sdl2.SDL_INIT_VIDEO | sdl2.SDL_INIT_AUDIO)
    window, renderer = sdl_video.initialize_sdl_video()
    audio_device = sdl_audio.initialize_sdl_audio()

    GAME_BOY.Initialize(sdl_video.FRAME_BUFFER, LOG_PATH, SAVE_PATH, BOOT_ROM_PATH)

    if rom_path:
        rom_path = ctypes.create_string_buffer(rom_path)
        GAME_BOY.InsertCartridge(rom_path)

    GAME_BOY.PowerOn()

    running = True
    event = sdl2.SDL_Event()
    sdl2.SDL_PauseAudioDevice(audio_device, 0)

    while running:
        while sdl2.SDL_PollEvent(ctypes.byref(event)) != 0:
            if event.type == sdl2.SDL_QUIT:
                running = False
                break
            elif event.type == sdl2.SDL_DROPFILE:
                sdl_audio.lock_audio(audio_device)
                GAME_BOY.InsertCartridge(event.drop.file)
                GAME_BOY.PowerOn()
                sdl_audio.unlock_audio(audio_device)

        joypad = controller.get_joypad()
        GAME_BOY.SetInputs(joypad.down, joypad.up, joypad.left, joypad.right, joypad.start, joypad.select, joypad.b, joypad.a)

        if (GAME_BOY.FrameReady()):
            sdl_video.update_screen(renderer)

        sdl2.SDL_Delay(1)

    sdl2.SDL_UnlockAudioDevice(audio_device)
    sdl2.SDL_CloseAudioDevice(audio_device)
    sdl2.SDL_DestroyRenderer(renderer)
    sdl2.SDL_DestroyWindow(window)
    sdl2.SDL_Quit()
    GAME_BOY.PowerOff()

    return 0

if __name__ == '__main__':
    rom_path = b""

    if len(sys.argv) > 1:
        rom_path = str.encode(sys.argv[1])

    sys.exit(main(rom_path))
