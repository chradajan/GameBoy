import game_boy.game_boy as game_boy
import sdl.controller as controller
import sdl.sdl_audio as sdl_audio
import sdl.sdl_video as sdl_video
import ctypes
import sdl2
import sys

SAVE_PATH = "./saves/"
BOOT_ROM_PATH = "./boot/"

def main(rom_path: str):
    sdl2.SDL_Init(sdl2.SDL_INIT_VIDEO | sdl2.SDL_INIT_AUDIO)
    window, renderer = sdl_video.initialize_sdl_video()
    audio_device = sdl_audio.initialize_sdl_audio()
    game_boy.initialize_game_boy(SAVE_PATH, BOOT_ROM_PATH)

    if rom_path:
        game_boy.insert_cartridge(rom_path)

    game_boy.power_on()

    running = True
    event = sdl2.SDL_Event()
    sdl_audio.unlock_audio(audio_device)

    while running:
        while sdl2.SDL_PollEvent(ctypes.byref(event)) != 0:
            if event.type == sdl2.SDL_QUIT:
                running = False
                break
            elif event.type == sdl2.SDL_DROPFILE:
                sdl_audio.lock_audio(audio_device)
                game_boy.insert_cartridge(event.drop.file)
                game_boy.power_on()
                sdl_audio.unlock_audio(audio_device)

        joypad = controller.get_joypad()
        game_boy.set_joypad_state(joypad)

        if game_boy.frame_ready():
            sdl_video.update_screen(renderer)

        sdl2.SDL_Delay(1)

    sdl_audio.destroy_audio_device(audio_device)
    sdl_video.destroy_window_and_renderer(window, renderer)
    game_boy.power_off()

    return 0

if __name__ == '__main__':
    rom_path = ""

    if len(sys.argv) > 1:
        rom_path = sys.argv[1]

    sys.exit(main(rom_path))
