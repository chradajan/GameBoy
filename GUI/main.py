import ctypes
import sys
from pathlib import Path

from PyQt6 import QtCore, QtWidgets

import config.config as config
import game_boy.game_boy as game_boy
import sdl.sdl_audio as sdl_audio
from qt.main_window import MainWindow

MAIN_WINDOW: MainWindow

class Refresher(QtCore.QObject):
    trigger = QtCore.pyqtSignal()

    def __init__(self):
        super().__init__()
        self.trigger.connect(MAIN_WINDOW.refresh_screen)
        self.trigger.emit()

@ctypes.CFUNCTYPE(None)
def refresh_screen_callback():
    Refresher()

def main() -> int:
    global MAIN_WINDOW
    config_path = Path(__file__).resolve().parents[0]
    game_boy.initialize_game_boy(refresh_screen_callback)
    game_boy.set_sample_rate(44100)
    sdl_audio.initialize_sdl_audio(44100)
    config.load_config(config_path)

    volume = config.get_saved_volume()
    game_boy.set_volume(volume / 100)

    app = QtWidgets.QApplication([])
    MAIN_WINDOW = MainWindow()

    sdl_audio.unlock_audio()
    val = app.exec()
    sdl_audio.destroy_audio_device()

    return val

if __name__ == '__main__':
    sys.exit(main())
