import ctypes
import game_boy.game_boy as game_boy
import sdl2

SAMPLE_RATE = 44100
AUDIO_BUFFER_SIZE = 512

@ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.POINTER(sdl2.Uint8), ctypes.c_int)
def audio_callback(userdata, stream, len):
    """Callback function to fill audio samples buffer.

    Args:
        userdata: Unused.
        stream: Byte buffer to save samples too.
        len: Number of bytes to populate sample buffer with.
    """
    buffer = ctypes.cast(stream, ctypes.POINTER(ctypes.c_float))
    num_samples = (len // ctypes.sizeof(ctypes.c_float))
    game_boy.collect_audio_samples(buffer, num_samples)

def initialize_sdl_audio() -> int:
    """Set up SDL audio for 2 channels of 32-bit floating point PCM samples at the desired sample rate.

    Returns:
        ID of opened audio device.
    """
    sdl2.SDL_Init(sdl2.SDL_INIT_AUDIO)
    audio_spec = sdl2.SDL_AudioSpec(0, 0, 0, 0)
    audio_spec.freq = SAMPLE_RATE
    audio_spec.format = sdl2.AUDIO_F32SYS
    audio_spec.channels = 2
    audio_spec.samples = AUDIO_BUFFER_SIZE
    audio_spec.callback = audio_callback
    audio_device = sdl2.SDL_OpenAudioDevice(None, 0, audio_spec, None, 0)

    return audio_device

def lock_audio(audio_device: int):
    """Stop audio playback. This pauses emulation.

    Args:
        audio_devices: ID of audio device to pause.
    """
    sdl2.SDL_LockAudioDevice(audio_device)
    sdl2.SDL_PauseAudioDevice(audio_device, 1)
    sdl2.SDL_Delay(20)

def unlock_audio(audio_device: int):
    """Resume audio playback. This unpauses emulation.

    Args:
        audio_devices: ID of audio device to unpause.
    """
    sdl2.SDL_UnlockAudioDevice(audio_device)
    sdl2.SDL_PauseAudioDevice(audio_device, 0)

def destroy_audio_device(audio_device):
    """Destroy the specified audio device.

    Args:
        audio_device: ID of audio device to close.
    """
    sdl2.SDL_UnlockAudioDevice(audio_device)
    sdl2.SDL_CloseAudioDevice(audio_device)
