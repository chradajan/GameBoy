from PyQt6 import QtCore, QtWidgets

import game_boy.game_boy as game_boy
from qt.color_tab import ColorTab
from qt.key_bindings_tab import KeyBindingsTab
from qt.paths_tab import PathsTab
from qt.sound_tab import SoundTab

class PreferencesWindow(QtWidgets.QWidget):
    def __init__(self):
        super().__init__()

        self._init_ui()
        self.setWindowFlags(QtCore.Qt.WindowType.MSWindowsFixedSizeDialogHint)
        self.setWindowTitle("Preferences")


    def _init_ui(self):
        main_layout = QtWidgets.QVBoxLayout()

        # Tabs
        self.sound_tab = SoundTab()
        self.color_tab = ColorTab()
        self.key_bindings_tab = KeyBindingsTab()
        self.paths_tab = PathsTab()

        tab_widget = QtWidgets.QTabWidget(self)
        tab_widget.addTab(self.sound_tab, "Sound")
        tab_widget.addTab(self.color_tab, "Colors")
        tab_widget.addTab(self.key_bindings_tab, "Controls")
        tab_widget.addTab(self.paths_tab, "Paths")

        main_layout.addWidget(tab_widget)

        # Buttons
        buttons = QtWidgets.QDialogButtonBox(QtWidgets.QDialogButtonBox.StandardButton.Ok |
                                                QtWidgets.QDialogButtonBox.StandardButton.Cancel |
                                                QtWidgets.QDialogButtonBox.StandardButton.Apply |
                                                QtWidgets.QDialogButtonBox.StandardButton.RestoreDefaults,
                                             QtCore.Qt.Orientation.Horizontal,
                                             self)

        buttons.button(QtWidgets.QDialogButtonBox.StandardButton.Ok).clicked.connect(self.okay_button_press)
        buttons.button(QtWidgets.QDialogButtonBox.StandardButton.Cancel).clicked.connect(self.cancel_button_press)
        buttons.button(QtWidgets.QDialogButtonBox.StandardButton.Apply).clicked.connect(self.apply_button_press)
        buttons.button(QtWidgets.QDialogButtonBox.StandardButton.RestoreDefaults).clicked.connect(self.defaults_button_press)
        main_layout.addWidget(buttons)

        # Set layout
        self.setLayout(main_layout)


    def okay_button_press(self):
        self.close()


    def cancel_button_press(self):
        self.color_tab.cancel()
        self.close()


    def apply_button_press(self):
        self.color_tab.apply()


    def defaults_button_press(self):
        self.sound_tab.restore_defaults()
        self.color_tab.restore_defaults()
