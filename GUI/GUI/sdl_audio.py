import ctypes
import sdl2
from GameBoy import GAME_BOY

SAMPLE_RATE = 44100
AUDIO_BUFFER_SIZE = 512
GAME_BOY.GetAudioSample.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_float)]

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

def initialize_sdl_audio() -> int:
    audio_spec = sdl2.SDL_AudioSpec(0, 0, 0, 0)
    audio_spec.freq = SAMPLE_RATE
    audio_spec.format = sdl2.AUDIO_F32SYS
    audio_spec.channels = 2
    audio_spec.samples = AUDIO_BUFFER_SIZE
    audio_spec.callback = audio_callback
    audio_device = sdl2.SDL_OpenAudioDevice(None, 0, audio_spec, None, 0)

    return audio_device

def lock_audio(audio_device: int):
    sdl2.SDL_LockAudioDevice(audio_device)
    sdl2.SDL_PauseAudioDevice(audio_device, 1)
    sdl2.SDL_Delay(20)

def unlock_audio(audio_device: int):
    sdl2.SDL_UnlockAudioDevice(audio_device)
    sdl2.SDL_PauseAudioDevice(audio_device, 0)
