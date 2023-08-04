"""Wrappers for C++ Game Boy Color emulator library."""

import ctypes
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import List

WIDTH = 160
HEIGHT = 144
CHANNELS = 3
BUFFER_SIZE = WIDTH * HEIGHT * CHANNELS

GAME_BOY: ctypes.CDLL
FRAME_BUFFER = (ctypes.c_uint8 * BUFFER_SIZE)()

if sys.platform == "darwin":
    GAME_BOY = ctypes.CDLL("./GameBoy/lib/libGameBoy.dylib", winmode=0)
elif sys.platform == "win32":
    GAME_BOY = ctypes.CDLL("./GameBoy/lib/libGameBoy.dll", winmode=0)

GAME_BOY.Initialize.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.CFUNCTYPE(None)]
GAME_BOY.InsertCartridge.argtypes = [ctypes.POINTER(ctypes.c_char), ctypes.POINTER(ctypes.c_char), ctypes.POINTER(ctypes.c_char)]
GAME_BOY.InsertCartridge.restype = ctypes.c_bool
GAME_BOY.PowerOn.argtypes = [ctypes.POINTER(ctypes.c_char)]
GAME_BOY.CollectAudioSamples.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.c_int]
GAME_BOY.SetInputs.argtypes = [
    ctypes.c_bool,
    ctypes.c_bool,
    ctypes.c_bool,
    ctypes.c_bool,
    ctypes.c_bool,
    ctypes.c_bool,
    ctypes.c_bool,
    ctypes.c_bool
]
GAME_BOY.SetClockMultiplier.argtypes = [ctypes.c_float]
GAME_BOY.CreateSaveState.argtypes = [ctypes.POINTER(ctypes.c_char)]
GAME_BOY.LoadSaveState.argtypes = [ctypes.POINTER(ctypes.c_char)]
GAME_BOY.EnableSoundChannel.argtypes = [ctypes.c_int, ctypes.c_bool]
GAME_BOY.SetMonoAudio.argtypes = [ctypes.c_bool]
GAME_BOY.SetVolume.argtypes = [ctypes.c_float]
GAME_BOY.SetSampleRate.argtypes = [ctypes.c_int]
GAME_BOY.PreferDmgColors.argtypes = [ctypes.c_bool]
GAME_BOY.UseIndividualPalettes.argtypes = [ctypes.c_bool]
GAME_BOY.SetCustomPalette.argtypes = [ctypes.c_uint8, ctypes.POINTER(ctypes.c_uint8)]

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


def initialize_game_boy(update_screen_callback: ctypes.CFUNCTYPE(None)):
    """Initialize the Game Boy library.

    Args:
        update_screen_callback: Function to call to refresh screen.
    """
    GAME_BOY.Initialize(FRAME_BUFFER, update_screen_callback)


def insert_cartridge(rom_path: str, save_directory: str) -> str:
    """Load specified Game Boy ROM file.

    Args:
        rom_path: Path to ROM file.
        save_directory: Path to directory to save/load cartridge SRAM from.

    Returns:
        ROM name from cartridge header.
    """
    if not save_directory:
        save_directory = str(Path(rom_path).parents[0])

    rom_path_buffer = ctypes.create_string_buffer(str.encode(rom_path))
    save_directory_buffer = ctypes.create_string_buffer(str.encode(save_directory))
    rom_name = ctypes.create_string_buffer(16)

    success = GAME_BOY.InsertCartridge(rom_path_buffer, save_directory_buffer, rom_name)

    if success:
        rom_str = rom_name.value.decode()
    else:
        rom_str = ""

    return rom_str


def power_on(boot_rom_path: str):
    """Power on the Game Boy. This function will reset the Game Boy to its initialize power on state.

    Args:
        boot_rom_path: Path to boot ROM. If empty, game will bypass boot up screen and launch directly.
    """
    boot_rom_path_buffer = ctypes.create_string_buffer(str.encode(boot_rom_path))
    GAME_BOY.PowerOn(boot_rom_path_buffer)


def set_joypad_state(joypad: JoyPad):
    """Update the Game Boy joypad state based on currently pressed keys.

    Args:
        joypad: Current state of each joypad button.
    """
    GAME_BOY.SetInputs(joypad.down, joypad.up, joypad.left, joypad.right, joypad.start, joypad.select, joypad.b, joypad.a)


def collect_audio_samples(buffer: ctypes.POINTER(ctypes.c_float), len: int):
    """Run the Game Boy and fill the buffer with audio samples.

    Args:
        buffer: Pointer to audio buffer to be filled.
        len: Number of samples to collect.
    """
    GAME_BOY.CollectAudioSamples(buffer, len)


def set_frame_ready_callback(callback: ctypes.CFUNCTYPE(None)):
    """Set the callback function used to render the frame buffer whenever it's full.

    Args:
        callback: Callback function for rendering.
    """
    GAME_BOY.SetFrameReadyCallback(callback)


def power_off():
    """Turn off Game Boy. If the current game supports battery-backed saves, create a save file."""
    GAME_BOY.PowerOff()


def get_frame_buffer() -> bytes:
    """Get a bytes representation of the frame buffer.

    Returns:
        Frame buffer array.
    """
    return bytes(FRAME_BUFFER)


def change_emulation_speed(multiplier: float):
    """Alter the emulation speed by changing the Game Boy frequency.

    Args:
        multiplier: Multiplier to alter clock speed by.
    """
    GAME_BOY.SetClockMultiplier(ctypes.c_float(multiplier))


def create_save_state(save_state_path: Path):
    """Generate a save state at the specified path.

    Args:
        save_state_path: Path to save state to.
    """
    save_state_path_buffer = ctypes.create_string_buffer(str.encode(str(save_state_path.absolute())))
    GAME_BOY.CreateSaveState(save_state_path_buffer)


def load_save_state(save_state_path: Path):
    """Load a save state from the specified path.

    Args:
        save_state_path: Path to load state from.
    """
    save_state_path_buffer = ctypes.create_string_buffer(str.encode(str(save_state_path.absolute())))
    GAME_BOY.LoadSaveState(save_state_path_buffer)


def enable_sound_channel(channel: int, enabled: bool):
    """Toggle a specific sound channel.

    Args:
        channel: Channel number to enable/disable (between 1-4).
        enabled: True to enable channel, False to disable it.
    """
    GAME_BOY.EnableSoundChannel(ctypes.c_int(channel), ctypes.c_bool(enabled))


def set_mono_audio(mono_audio: bool):
    """Toggle between mono and stereo audio.

    Args:
        mono_audio: True to use mono audio, False for stereo.
    """
    GAME_BOY.SetMonoAudio(ctypes.c_bool(mono_audio))


def set_volume(volume: float):
    """Set the volume output level.

    Args:
        volume: Output level (between 0.0 and 1.0).
    """
    GAME_BOY.SetVolume(ctypes.c_float(volume))


def set_sample_rate(rate: int):
    """Set the sampling frequency used by SDL.

    Args:
        rate: Sample frequency in Hz.
    """
    GAME_BOY.SetSampleRate(ctypes.c_int(rate))


def prefer_dmg_colors(use_dmg_colors: bool):
    """If playing a GB game, prefer to use a custom DMG palette instead of the one determined by boot ROM.

    Args:
        use_dmg_colors: If True, use custom palettes.
    """
    GAME_BOY.PreferDmgColors(ctypes.c_bool(use_dmg_colors))


def use_individual_palettes(individual_palettes: bool):
    """When using DMG palettes, determine whether each pixel type should share a palette or use its own custom one.

    Args:
        individual_palettes: True if pixel sources should use their own palettes, False if they should all use the same palette."""
    GAME_BOY.UseIndividualPalettes(ctypes.c_bool(individual_palettes))


def set_custom_palette(index: int, data: List[int]):
    """Set custom DMG palette.

    Args:
        index: Index of palette to update.
            0 = Universal palette
            1 = Background
            2 = Window
            3 = OBP0
            4 = OBP1
        data: List of 12 0-255 rgb values that define 4 colors.
    """
    arr = (ctypes.c_uint8 * len(data))(*data)
    GAME_BOY.SetCustomPalette(ctypes.c_uint8(index), arr)
