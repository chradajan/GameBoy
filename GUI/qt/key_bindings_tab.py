from functools import partial
from pathlib import Path
from typing import List

from PyQt6 import QtCore, QtGui, QtWidgets

import config.config as config
import controller.controller as controller

GAMEPAD_INPUT: str = None

class KeyboardBindingDialog(QtWidgets.QDialog):
    def __init__(self, joypad_button: str, icon_directory: Path):
        super().__init__()

        self.joypad_button_to_bind = joypad_button
        self.setWindowTitle(joypad_button.capitalize())
        self.setFixedSize(150, 50)
        self.setWindowIcon(QtGui.QIcon(str(icon_directory / "keyboard.ico")))

        layout = QtWidgets.QVBoxLayout()
        label = QtWidgets.QLabel("Press a key...")
        label.setAlignment(QtCore.Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(label)
        self.setLayout(layout)


    def keyPressEvent(self, event: QtGui.QKeyEvent | None):
        super().keyPressEvent(event)

        if event is None:
            return

        if event.key() != QtCore.Qt.Key.Key_Escape:
            config.set_keyboard_binding(self.joypad_button_to_bind, event.key())

        self.accept()


class GetGamepadRebind(QtCore.QThread):
    def run(self):
        global GAMEPAD_INPUT
        GAMEPAD_INPUT = controller.get_first_gamepad_key()


class GamepadBindingDialog(QtWidgets.QDialog):
    def __init__(self, joypad_button: str, icon_directory: Path):
        super().__init__()

        self.joypad_button_to_bind = joypad_button
        self.setWindowTitle(joypad_button.capitalize())
        self.setFixedSize(150, 50)
        self.setWindowIcon(QtGui.QIcon(str(icon_directory / "gamepad.ico")))

        layout = QtWidgets.QVBoxLayout()
        label = QtWidgets.QLabel("Press a button..")
        label.setAlignment(QtCore.Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(label)
        self.setLayout(layout)

        self.get_binding_thread = GetGamepadRebind()
        self.get_binding_thread.finished.connect(self._update_binding)
        self.get_binding_thread.start()


    def __del__(self):
        self.get_binding_thread.terminate()


    def _update_binding(self):
        global GAMEPAD_INPUT

        if GAMEPAD_INPUT is not None:
            config.set_gamepad_binding(self.joypad_button_to_bind, GAMEPAD_INPUT)

        self.accept()


    def keyPressEvent(self, event: QtGui.QKeyEvent | None):
        super().keyPressEvent(event)

        if event is None:
            return

        if event.key() == QtCore.Qt.Key.Key_Escape:
            self.accept()


class KeyBindingsTab(QtWidgets.QWidget):
    def __init__(self, icon_directory: Path):
        super().__init__()

        self.icon_directory = icon_directory

        self.button_to_bind: str = None
        self.keyboard_index_to_update: int = None
        self.keyboard_buttons: List[QtWidgets.QPushButton] = []
        self.gamepad_buttons: List[QtWidgets.QPushButton] = []

        self._init_ui()


    def _init_ui(self):
        layout = QtWidgets.QGridLayout()
        keyboard_bindings = config.get_keyboard_bindings()
        gamepad_bindings = config.get_gamepad_bindings()

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
            self.keyboard_buttons[-1].setText(QtCore.Qt.Key(keyboard_bindings[key]).name[4:])
            self.keyboard_buttons[-1].clicked.connect(partial(self._keyboard_button_trigger, i, key))

            self.gamepad_buttons.append(QtWidgets.QPushButton())
            self.gamepad_buttons[-1] = QtWidgets.QPushButton()
            self.gamepad_buttons[-1].setMaximumWidth(150)
            self.gamepad_buttons[-1].setText(gamepad_bindings[key])
            self.gamepad_buttons[-1].clicked.connect(partial(self._gamepad_button_trigger, i, key))

            layout.addWidget(joypad_label, i + 1, 0)
            layout.addWidget(self.keyboard_buttons[-1], i + 1, 1)
            layout.addWidget(self.gamepad_buttons[-1], i + 1, 2)

        spacer = QtWidgets.QWidget()
        spacer.setSizePolicy(QtWidgets.QSizePolicy.Policy.Preferred, QtWidgets.QSizePolicy.Policy.Expanding)
        layout.addWidget(spacer, 9, 0)

        self.setLayout(layout)


    def _keyboard_button_trigger(self, index: int, joypad_button: str):
        dialog = KeyboardBindingDialog(joypad_button, self.icon_directory)
        dialog.exec()

        keyboard_bindings = config.get_keyboard_bindings()
        self.keyboard_buttons[index].setText(QtCore.Qt.Key(keyboard_bindings[joypad_button]).name[4:])


    def _gamepad_button_trigger(self, index: int, joypad_button: str):
        dialog = GamepadBindingDialog(joypad_button, self.icon_directory)
        dialog.exec()

        gamepad_bindings = config.get_gamepad_bindings()
        self.gamepad_buttons[index].setText(gamepad_bindings[joypad_button])


    def restore_defaults(self):
        config.restore_default_controls()
        gamepad_bindings = config.get_gamepad_bindings()
        keyboard_bindings = config.get_keyboard_bindings()

        for index, ((_, gamepad_binding), (_, keyboard_binding))\
            in enumerate(zip(gamepad_bindings.items(), keyboard_bindings.items())):
            self.gamepad_buttons[index].setText(gamepad_binding)
            self.keyboard_buttons[index].setText(QtCore.Qt.Key(keyboard_binding).name[4:])
