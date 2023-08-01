import ctypes
import game_boy.game_boy as game_boy
import sdl2

AUDIO_BUFFER_SIZE = 512
AUDIO_DEVICE: int = None

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

def initialize_sdl_audio(sample_rate: int):
    """Set up SDL audio for 2 channels of 32-bit floating point PCM samples at the desired sample rate.

    Args:
        sample_rate: Sample rate in Hz.
    """
    global AUDIO_DEVICE
    sdl2.SDL_Init(sdl2.SDL_INIT_AUDIO)
    audio_spec = sdl2.SDL_AudioSpec(0, 0, 0, 0)
    audio_spec.freq = sample_rate
    audio_spec.format = sdl2.AUDIO_F32SYS
    audio_spec.channels = 2
    audio_spec.samples = AUDIO_BUFFER_SIZE
    audio_spec.callback = audio_callback
    AUDIO_DEVICE = sdl2.SDL_OpenAudioDevice(None, 0, audio_spec, None, 0)

def lock_audio():
    """Stop audio playback. This pauses emulation."""
    global AUDIO_DEVICE
    sdl2.SDL_LockAudioDevice(AUDIO_DEVICE)
    sdl2.SDL_PauseAudioDevice(AUDIO_DEVICE, 1)
    sdl2.SDL_Delay(20)

def unlock_audio():
    """Resume audio playback. This unpauses emulation."""
    global AUDIO_DEVICE
    sdl2.SDL_UnlockAudioDevice(AUDIO_DEVICE)
    sdl2.SDL_PauseAudioDevice(AUDIO_DEVICE, 0)

def destroy_audio_device():
    """Destroy the specified audio device."""
    global AUDIO_DEVICE
    sdl2.SDL_UnlockAudioDevice(AUDIO_DEVICE)
    sdl2.SDL_CloseAudioDevice(AUDIO_DEVICE)

def update_sample_rate(sample_rate: int):
    """Change the sample rate of the audio device.

    Args:
        sample_rate: New sampling frequency in Hz.
    """
    global AUDIO_DEVICE
    destroy_audio_device()
    initialize_sdl_audio(sample_rate)
    unlock_audio()
