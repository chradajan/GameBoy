from PyQt6 import QtCore, QtGui, QtWidgets

import game_boy.game_boy as game_boy
from qt.color_tab import ColorTab
from qt.sound_tab import SoundTab

class PreferencesWindow(QtWidgets.QTabWidget):
    def __init__(self):
        super().__init__()

        # Gui components
        self.volume_slider = None
        self.saved_volume = 100
        self.mute_box = None
        self.muted = False

        self._init_ui()
        self.setWindowFlags(QtCore.Qt.WindowType.MSWindowsFixedSizeDialogHint)

    def _init_ui(self):
        self.sound_tab = SoundTab(self)
        self.color_tab = ColorTab(self)

        self.addTab(self.sound_tab, "Sound")
        self.addTab(self.color_tab, "Colors")

        self.setWindowTitle("Preferences")

