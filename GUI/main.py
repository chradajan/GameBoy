import game_boy.game_boy as game_boy
import sdl.controller as controller
import sdl.sdl_audio as sdl_audio
from qt.main_window import MainWindow

import ctypes
import sys
from pathlib import Path
from PyQt6 import QtCore, QtWidgets

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
    game_boy.initialize_game_boy(Path(__file__).resolve().parents[1])

    if len(sys.argv) > 1:
        game_boy.insert_cartridge(sys.argv[1])

    game_boy.power_on()

    audio_device_id = sdl_audio.initialize_sdl_audio()

    app = QtWidgets.QApplication([])
    MAIN_WINDOW = MainWindow(audio_device_id)
    game_boy.set_frame_ready_callback(refresh_screen_callback)

    sdl_audio.unlock_audio(audio_device_id)
    val = app.exec()
    sdl_audio.destroy_audio_device(audio_device_id)

    return val

if __name__ == '__main__':
    sys.exit(main())
