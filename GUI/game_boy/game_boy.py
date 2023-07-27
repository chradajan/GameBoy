"""Wrappers for C++ Game Boy Color emulator library."""

import ctypes
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Tuple

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


def initialize_game_boy(base_path: Path):
    """Initialize the Game Boy library.

    Args:
        base_path: Directory containing the save and boot ROM paths.
    """
    save_path = base_path / "saves"
    boot_rom_dir = base_path / "boot"
    boot_rom_path = boot_rom_dir / "cgb_boot.bin"
    save_path.mkdir(parents=True, exist_ok=True)
    boot_rom_dir.mkdir(parents=True, exist_ok=True)

    save_path_buffer = ctypes.create_string_buffer(str.encode(str(save_path.absolute())))
    boot_rom_path_buffer = ctypes.create_string_buffer(str.encode(str(boot_rom_path.absolute())))

    GAME_BOY.Initialize(FRAME_BUFFER, save_path_buffer, boot_rom_path_buffer)

def insert_cartridge(path: str | bytes | Path):
    """Load specified Game Boy ROM file.

    Args:
        path: Path to ROM file.
    """
    if type(path) == str:
        path_buffer = ctypes.create_string_buffer(str.encode(path))
    elif type(path) == bytes:
        path_buffer = ctypes.create_string_buffer(path)
    elif type(path) == Path:
        path_buffer = ctypes.create_string_buffer(str.encode(str(path.absolute())))

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
