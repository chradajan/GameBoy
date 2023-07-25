"""Wrappers for C++ Game Boy Color emulator library."""

import ctypes
import sys
from dataclasses import dataclass
from typing import Optional, Tuple

WIDTH = 160
HEIGHT = 144
CHANNELS = 3
DEPTH = CHANNELS * 8
PITCH = WIDTH * CHANNELS
BUFFER_SIZE = WIDTH * HEIGHT * CHANNELS

GAME_BOY: ctypes.CDLL
FRAME_BUFFER = (ctypes.c_uint8 * BUFFER_SIZE)()

if sys.platform == "darwin":
    GAME_BOY = ctypes.CDLL("./GameBoy/lib/libGameBoy.dylib", winmode=0)
elif sys.platform == "win32":
    GAME_BOY = ctypes.CDLL("./GameBoy/lib/libGameBoy.dll", winmode=0)

GAME_BOY.GetAudioSample.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_float)]


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


def initialize_game_boy(save_path: str, boot_rom_path: Optional[str]=None):
    """Initialize the Game Boy library.

    Args:
        save_path: Path to store save files.
        boot_rom_path: Path to Game Boy Color boot ROM. Boot up sequence is skipped if not provided.
    """
    save_path_buffer = ctypes.create_string_buffer(str.encode(save_path))

    if boot_rom_path:
        boot_rom_path_buffer = ctypes.create_string_buffer(str.encode(boot_rom_path))
    else:
        boot_rom_path_buffer = ctypes.create_string_buffer(b"")

    GAME_BOY.Initialize(FRAME_BUFFER, save_path_buffer, boot_rom_path_buffer)

def insert_cartridge(path: str | bytes):
    """Load specified Game Boy ROM file.

    Args:
        path: Path to ROM file.
    """
    if type(path) == str:
        path_buffer = ctypes.create_string_buffer(str.encode(path))
    else:
        path_buffer = ctypes.create_string_buffer(path)

    GAME_BOY.InsertCartridge(path_buffer)

def power_on():
    """Power on the Game Boy. This function will reset the Game Boy to its initialize power on state."""
    GAME_BOY.PowerOn()

def set_joypad_state(joypad: JoyPad):
    """Update the Game Boy joypad state based on currently pressed keys.

    Args:
        joypad: Current state of each joypad button.
    """
    GAME_BOY.SetInputs(joypad.down, joypad.up, joypad.left, joypad.right, joypad.start, joypad.select, joypad.b, joypad.a)

def frame_ready() -> bool:
    """Check if a frame is ready to display.

    Returns:
        True if the current state of the frame buffer is ready to be rendered.
    """
    return GAME_BOY.FrameReady()

def get_audio_sample() -> Tuple[float, float]:
    """Run the Game Boy enough cycles to gather the next audio sample from the APU.

    Returns:
        Tuple of left and right channel PCM samples.
    """
    left = ctypes.c_float(0.0)
    right = ctypes.c_float(0.0)
    GAME_BOY.GetAudioSample(ctypes.byref(left), ctypes.byref(right))
    return (left.value, right.value)

def power_off():
    """Turn off Game Boy. If the current game supports battery-backed saves, create a save file."""
    GAME_BOY.PowerOff()
