from functools import partial
from typing import List

from PyQt6 import QtCore, QtGui, QtWidgets

import config.config as config

class KeyBindingDialog(QtWidgets.QMessageBox):
    def __init__(self, joypad_button: str):
        super().__init__()

        self.joypad_button_to_bind = joypad_button
        self.setWindowTitle(joypad_button.capitalize())
        self.setText("Press a key...")
        self.setStandardButtons(QtWidgets.QMessageBox.StandardButton.NoButton)


    def keyPressEvent(self, event: QtGui.QKeyEvent | None):
        super().keyPressEvent(event)

        if event is None:
            return

        if event.key() != QtCore.Qt.Key.Key_Escape:
            config.set_key_binding(self.joypad_button_to_bind, event.key())

        self.accept()


class KeyBindingsTab(QtWidgets.QWidget):
    def __init__(self, parent: QtWidgets.QWidget):
        super().__init__(parent)

        self.button_to_bind: str = None
        self.keyboard_index_to_update: int = None
        self.keyboard_buttons: List[QtWidgets.QPushButton] = []
        self.gamepad_buttons: List[QtWidgets.QPushButton] = []

        self._init_ui()


    def _init_ui(self):
        layout = QtWidgets.QGridLayout()
        bindings = config.get_current_key_bindings()

        keyboard_label = QtWidgets.QLabel("Keyboard")
        keyboard_label.setAlignment(QtCore.Qt.AlignmentFlag.AlignCenter)

        gamepad_label = QtWidgets.QLabel("Gamepad")
        gamepad_label.setAlignment(QtCore.Qt.AlignmentFlag.AlignCenter)

        layout.addWidget(keyboard_label, 0, 1)
        layout.addWidget(gamepad_label, 0, 2)

        for i, key in enumerate(["up", "down", "left", "right", "start", "select", "b", "a"]):
            joypad_label = QtWidgets.QLabel(key.capitalize())
            joypad_label.setMaximumWidth(40)

            self.keyboard_buttons.append(QtWidgets.QPushButton())
            self.keyboard_buttons[-1].setMaximumWidth(150)
            self.keyboard_buttons[-1].setText(QtCore.Qt.Key(bindings[key]).name[4:])
            self.keyboard_buttons[-1].clicked.connect(partial(self._keyboard_button_trigger, i, key))

            self.gamepad_buttons.append(QtWidgets.QPushButton())
            self.gamepad_buttons[-1] = QtWidgets.QPushButton()
            self.gamepad_buttons[-1].setMaximumWidth(150)
            self.gamepad_buttons[-1].clicked.connect(partial(self._gamepad_button_trigger, i, key))

            layout.addWidget(joypad_label, i + 1, 0)
            layout.addWidget(self.keyboard_buttons[-1], i + 1, 1)
            layout.addWidget(self.gamepad_buttons[-1], i + 1, 2)

        spacer = QtWidgets.QWidget()
        spacer.setSizePolicy(QtWidgets.QSizePolicy.Policy.Preferred, QtWidgets.QSizePolicy.Policy.Expanding)
        layout.addWidget(spacer, 9, 0)

        self.setLayout(layout)


    def _keyboard_button_trigger(self, index: int, joypad_button: str):
        dialog = KeyBindingDialog(joypad_button)
        dialog.exec()

        key_bindings = config.get_current_key_bindings()
        self.keyboard_buttons[index].setText(QtCore.Qt.Key(key_bindings[joypad_button]).name[4:])


    def _gamepad_button_trigger(self, index: int, joypad_button: str):
        pass
