from typing import List

from PyQt6 import QtWidgets

import config.config as config

class PathsTab(QtWidgets.QWidget):
    def __init__(self, parent: QtWidgets.QWidget):
        super().__init__(parent)
        self._init_ui()


    def _init_ui(self):
        main_layout = QtWidgets.QGridLayout()

        for index, text in enumerate(["Boot ROM", "Save Files", "Save States"]):
            label = QtWidgets.QLabel(text)
            main_layout.addWidget(label, index, 0)

        self.boot_rom_line_edit = QtWidgets.QLineEdit()
        self.boot_rom_line_edit.setReadOnly(True)
        self.boot_rom_line_edit.setText(config.get_boot_rom_path())
        main_layout.addWidget(self.boot_rom_line_edit, 0, 1)
        self.boot_rom_button = QtWidgets.QPushButton()
        self.boot_rom_button.setText("...")
        self.boot_rom_button.clicked.connect(self._boot_rom_select_trigger)
        main_layout.addWidget(self.boot_rom_button, 0, 2)

        self.save_directory_line_edit = QtWidgets.QLineEdit()
        self.save_directory_line_edit.setReadOnly(True)
        self.save_directory_line_edit.setText(config.get_saves_directory())
        main_layout.addWidget(self.save_directory_line_edit, 1, 1)
        self.save_directory_button = QtWidgets.QPushButton()
        self.save_directory_button.setText("...")
        self.save_directory_button.clicked.connect(self._save_directory_select_trigger)
        main_layout.addWidget(self.save_directory_button, 1, 2)

        self.save_state_directory_line_edit = QtWidgets.QLineEdit()
        self.save_state_directory_line_edit.setReadOnly(True)
        self.save_state_directory_line_edit.setText(config.get_save_states_directory())
        main_layout.addWidget(self.save_state_directory_line_edit, 2, 1)
        self.save_state_directory_button = QtWidgets.QPushButton()
        self.save_state_directory_button.setText("...")
        self.save_state_directory_button.clicked.connect(self._save_state_directory_select_trigger)
        main_layout.addWidget(self.save_state_directory_button, 2, 2)

        spacer = QtWidgets.QWidget()
        spacer.setSizePolicy(QtWidgets.QSizePolicy.Policy.Preferred, QtWidgets.QSizePolicy.Policy.Expanding)
        main_layout.addWidget(spacer, 3, 0)

        self.setLayout(main_layout)


    def _boot_rom_select_trigger(self):
        file_dialog = QtWidgets.QFileDialog(self)
        file_dialog.setFileMode(QtWidgets.QFileDialog.FileMode.ExistingFile)
        boot_rom_path, _ = file_dialog.getOpenFileName(self,
                                                       "GBC Boot ROM",
                                                       None,
                                                       "*.bin")

        if boot_rom_path:
            self.boot_rom_line_edit.setText(boot_rom_path)
            config.set_boot_rom_path(boot_rom_path)


    def _save_directory_select_trigger(self):
        file_dialog = QtWidgets.QFileDialog(self)
        file_dialog.setFileMode(QtWidgets.QFileDialog.FileMode.Directory)
        save_directory_path = file_dialog.getExistingDirectory(self, "Saves Directory")

        if save_directory_path:
            self.save_directory_line_edit.setText(save_directory_path)
            config.set_saves_directory(save_directory_path)


    def _save_state_directory_select_trigger(self):
        file_dialog = QtWidgets.QFileDialog(self)
        file_dialog.setFileMode(QtWidgets.QFileDialog.FileMode.Directory)
        save_state_directory_path = file_dialog.getExistingDirectory(self, "Save States Directory")

        if save_state_directory_path:
            self.save_state_directory_line_edit.setText(save_state_directory_path)
            config.set_save_states_directory(save_state_directory_path)
