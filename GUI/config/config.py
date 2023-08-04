import configparser
from pathlib import Path
from typing import Dict, List

CONFIG = configparser.ConfigParser()
CONFIG.optionxform = str
CONFIG_PATH: Path = None

def load_config(config_path: Path):
    """Load the .ini file from the specified path, or create a default one if it doesn't exist.

    Args:
        config_path: Path of .ini file to load/create.
    """
    global CONFIG
    global CONFIG_PATH

    CONFIG_PATH = config_path / "config.ini"

    if CONFIG_PATH.is_file():
        CONFIG.read(CONFIG_PATH)
    else:
        CONFIG["Paths"] = {
            "last_rom_dir": CONFIG_PATH.parents[0],
            "boot_rom_dir": "",
            "recent_0": "",
            "recent_1": "",
            "recent_2": "",
            "recent_3": "",
            "recent_4": "",
            "recent_5": "",
            "recent_6": "",
            "recent_7": "",
            "recent_8": "",
            "recent_9": "",
        }

        CONFIG["Colors"] = {
            "Green": "afcb46 79aa6d 226f5f 082955",
            "Grey":  "e8e8e8 a0a0a0 585858 101010",
            "Yellow": "f8f078 b0a848 686830 202010",
            "Red": "ffc0c0 ff6060 c00000 600000",
            "Blue": "c0c0ff 5f60ff 0000c0 000060",
        }

        CONFIG["GamepadControls"] = {
            "down": "d_pad_down",
            "up": "d_pad_up",
            "left": "d_pad_left",
            "right": "d_pad_right",
            "start": "start",
            "select": "select",
            "b": "b",
            "a": "a",
        }

        CONFIG["Controls"] = {
            "down": 83,
            "up": 87,
            "left": 65,
            "right": 68,
            "start": 16777220,
            "select": 16777248,
            "b": 75,
            "a": 76,
        }

        save_config()


def get_keyboard_bindings() -> Dict[str, int]:
    """Get the current keyboard bindings.

    Returns:
        Dictionary mapping joypad key name to keyboard scancode.
    """
    global CONFIG
    bindings = {}

    for key, scancode in CONFIG["Controls"].items():
        bindings[key] = int(scancode)

    return bindings


def set_keyboard_binding(key: str, scancode: int):
    """Change a keyboard binding.

    Args:
        key: Name of joypad to update binding of.
        scancode: Keyboard key scancode to bind.
    """
    global CONFIG
    CONFIG["Controls"][key] = str(scancode)
    save_config()


def get_gamepad_bindings() -> Dict[str, str]:
    """Get the current gamepad bindings.

    Returns:
        Dictionary mapping joypad key name to inputs gamepad names.
    """
    global CONFIG
    bindings = {}

    for key, gamepad_key in CONFIG["GamepadControls"].items():
        bindings[key] = gamepad_key

    return bindings


def set_gamepad_binding(key: str, gamepad_key: int):
    """Change a keyboard binding.

    Args:
        key: Name of joypad to update binding of.
        gamepad_key: Inputs gamepad name of button to bind.
    """
    global CONFIG
    CONFIG["GamepadControls"][key] = gamepad_key
    save_config()


def get_last_rom_directory() -> str:
    """Get the most recent directory that a ROM was loaded from.

    Returns:
        Path of directory to start search in.
    """
    global CONFIG
    return CONFIG["Paths"]["last_rom_dir"]


def update_last_rom_directory(rom_path: str):
    """Set the most recent directory that a ROM was loaded from.

    Args:
        rom_path: Path of most recently loaded ROM file.
    """
    global CONFIG
    CONFIG["Paths"]["last_rom_dir"] = rom_path
    save_config()


def add_recent_rom(rom_path: str):
    """Add a ROM path to the list of recently opened ROMs.

    Args:
        rom_path: Path to add to recents list.
    """
    global CONFIG
    start_index = 8
    recent_roms = get_recent_roms()

    if rom_path in recent_roms:
        start_index = recent_roms.index(rom_path) - 1

    for i in range(start_index, -1, -1):
        CONFIG["Paths"][f"recent_{i+1}"] = CONFIG["Paths"][f"recent_{i}"]

    CONFIG["Paths"]["recent_0"] = rom_path
    save_config()


def get_recent_roms() -> List[str]:
    """Get a list of recently loaded ROMs.

    Returns:
        List of paths to recent ROMs.
    """
    global CONFIG
    recent_roms: List[Path] = []

    for i in range(10):
        if not CONFIG["Paths"][f"recent_{i}"]:
            break

        recent_roms.append(CONFIG["Paths"][f"recent_{i}"])

    return recent_roms


def save_config():
    """Save the current config to the .ini file."""
    global CONFIG
    global CONFIG_PATH

    with open(CONFIG_PATH, 'w') as config_file:
            CONFIG.write(config_file)


def get_color_schemes() -> Dict[str, List[int]]:
    """Get a dictionary of each saved color scheme.

    Returns:
        Dictionary connecting color scheme names to their colors.
    """
    global CONFIG

    color_schemes = {}

    for name, colors in CONFIG["Colors"].items():
        color_list = []

        for color_str in colors.split(' '):
            color_list.append(int(color_str[0:2], 16))
            color_list.append(int(color_str[2:4], 16))
            color_list.append(int(color_str[4:6], 16))

        color_schemes[name] = color_list

    return color_schemes


def add_color_scheme(name: str, colors: List[int]):
    """Insert a new color scheme. If a scheme with the same name already exists, it will be updated.

    Args:
        name: Name of color scheme.
        colors: Colors associated with this scheme.
    """
    global CONFIG

    hex_strs = []

    for i in range(0, 4):
        hex_strs.append(f"{colors[i*3]:02x}{colors[i*3 + 1]:02x}{colors[i*3 + 2]:02x}")

    hex_str = " ".join(hex_strs)
    CONFIG["Colors"][name] = hex_str
    save_config()


def delete_color_scheme(name: str):
    """Delete a color scheme from the config.

    Args:
        name: Name of color scheme to delete.
    """
    global CONFIG

    if name in CONFIG["Colors"]:
        CONFIG.remove_option("Colors", name)

    save_config()
