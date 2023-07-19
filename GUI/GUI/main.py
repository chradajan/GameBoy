import controller
import sdl_video
import ctypes
import sdl2
import sdl2.ext
import sys

FPS = 60
TIME_PER_FRAME = 1000 // 60
SAMPLE_RATE = 44100
AUDIO_BUFFER_SIZE = 512

LOG_PATH = ctypes.create_string_buffer(b"./logs/")
SAVE_PATH = ctypes.create_string_buffer(b"./saves/")
BOOT_ROM_PATH = ctypes.create_string_buffer(b"./boot/")

if sys.platform == "darwin":
    GAME_BOY = ctypes.CDLL("./GameBoy/lib/libGameBoy.dylib", winmode=0)
elif sys.platform == "win32":
    GAME_BOY = ctypes.CDLL("./GameBoy/lib/libGameBoy.dll", winmode=0)

GAME_BOY.GetAudioSample.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_float)]
RENDERER = None

@ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.POINTER(sdl2.Uint8), ctypes.c_int)
def audio_callback(userdata, stream, len):
    buffer = ctypes.cast(stream, ctypes.POINTER(ctypes.c_float))
    num_samples = (len // ctypes.sizeof(ctypes.c_float))
    left = ctypes.c_float(0.0)
    right = ctypes.c_float(0.0)

    for i in range(0, num_samples, 2):
        GAME_BOY.GetAudioSample(ctypes.byref(left), ctypes.byref(right))
        buffer[i] = left.value * 0.15
        buffer[i+1] = right.value * 0.15

def main(rom_path: bytes):
    global RENDERER

    sdl2.SDL_Init(sdl2.SDL_INIT_VIDEO | sdl2.SDL_INIT_AUDIO)
    window, RENDERER = sdl_video.initialize_sdl_video()

    audio_spec = sdl2.SDL_AudioSpec(0, 0, 0, 0)
    audio_spec.freq = SAMPLE_RATE
    audio_spec.format = sdl2.AUDIO_F32SYS
    audio_spec.channels = 2
    audio_spec.samples = AUDIO_BUFFER_SIZE
    audio_spec.callback = audio_callback
    audio_device = sdl2.SDL_OpenAudioDevice(None, 0, audio_spec, None, 0)

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

        joypad = controller.get_joypad()
        GAME_BOY.SetInputs(joypad.down, joypad.up, joypad.left, joypad.right, joypad.start, joypad.select, joypad.b, joypad.a)

        if (GAME_BOY.FrameReady()):
            sdl_video.update_screen(RENDERER)

        sdl2.SDL_Delay(1)

    sdl2.SDL_UnlockAudioDevice(audio_device)
    sdl2.SDL_CloseAudioDevice(audio_device)
    sdl2.SDL_DestroyRenderer(RENDERER)
    sdl2.SDL_DestroyWindow(window)
    sdl2.SDL_Quit()
    GAME_BOY.PowerOff()

    return 0

if __name__ == '__main__':
    rom_path = b""

    if len(sys.argv) > 1:
        rom_path = str.encode(sys.argv[1])

    sys.exit(main(rom_path))
