from functools import partial
from typing import List
from PyQt6 import QtCore, QtWidgets

import config.config as config
import game_boy.game_boy as game_boy

class ColorTab(QtWidgets.QWidget):
    def __init__(self, parent:QtWidgets.QWidget):
        super().__init__(parent)

        self.drop_downs: List[QtWidgets.QComboBox] = []

        self.color_texts: List[QtWidgets.QLineEdit] = []
        self.color_sliders: List[QtWidgets.QSlider] = []

        self.color_buttons: List[QtWidgets.QPushButton] = []
        self.current_scheme: List[int] = []
        self.selected_index = 0

        self.saved_palettes: QtWidgets.QComboBox = None

        self.update_button: QtWidgets.QPushButton = None
        self.delete_button: QtWidgets.QPushButton = None

        self._init_ui()

    def _init_ui(self):
        main_layout = QtWidgets.QHBoxLayout()
        color_schemes = config.get_color_schemes()
        color_scheme_names = [scheme for scheme in color_schemes.keys()]
        self.current_scheme = color_schemes["Green"]

        # Palette selection
        palette_selection = QtWidgets.QGroupBox("Palette Selection")
        palette_selection_layout = QtWidgets.QVBoxLayout()

        custom_colors_box = QtWidgets.QCheckBox("Use custom colors")
        custom_colors_box.toggled.connect(self._custom_colors_toggle)
        palette_selection_layout.addWidget(custom_colors_box)

        universal_palette_box = QtWidgets.QCheckBox("Universal palette")
        universal_palette_box.setChecked(True)
        universal_palette_box.toggled.connect(self._use_individual_palettes)
        palette_selection_layout.addWidget(universal_palette_box)

        for index, label_name in enumerate(["Universal", "BG", "WIN", "OBP0", "OBP1"]):
            label = QtWidgets.QLabel(label_name)
            self.drop_downs.append(QtWidgets.QComboBox())
            self.drop_downs[-1].addItems(color_scheme_names)
            self.drop_downs[-1].setCurrentIndex(0)
            self.drop_downs[-1].activated.connect(partial(self._set_custom_palette, index))
            palette_selection_layout.addWidget(label)
            palette_selection_layout.addWidget(self.drop_downs[-1])

        palette_selection.setLayout(palette_selection_layout)
        main_layout.addWidget(palette_selection)

        # Palette creator
        palette_creation = QtWidgets.QGroupBox("Create Palette")
        palette_creation_layout = QtWidgets.QGridLayout()

        for i in range(3):
            self.color_texts.append(QtWidgets.QLineEdit())
            self.color_texts[-1].setMaximumWidth(30)
            self.color_texts[-1].textEdited.connect(partial(self._color_text_changed, i))

            self.color_sliders.append(QtWidgets.QSlider(QtCore.Qt.Orientation.Horizontal))
            self.color_sliders[-1].sliderMoved.connect(partial(self._slider_moved, i))
            self.color_sliders[-1].setRange(0, 255)
            self.color_sliders[-1].setSingleStep(1)
            self.color_sliders[-1].setPageStep(0)

            palette_creation_layout.addWidget(self.color_sliders[-1], i, 0, 1, 3)
            palette_creation_layout.addWidget(self.color_texts[-1], i, 3)

        colors = QtWidgets.QGroupBox()
        colors_layout = QtWidgets.QHBoxLayout()

        for i in range(4):
            self.color_buttons.append(QtWidgets.QPushButton())
            self.color_buttons[-1].setFixedSize(50, 50)
            self.color_buttons[-1].clicked.connect(partial(self._color_clicked, i))
            colors_layout.addWidget(self.color_buttons[-1])

        self.selected_index = 0
        colors.setLayout(colors_layout)
        palette_creation_layout.addWidget(colors, 3, 0, 1, 4)

        self.saved_palettes = QtWidgets.QComboBox()
        self.saved_palettes.setFixedWidth(100)
        self.saved_palettes.addItems(color_scheme_names)
        self.saved_palettes.setCurrentIndex(0)
        self.saved_palettes.activated.connect(self._saved_palette_changed)
        palette_creation_layout.addWidget(self.saved_palettes, 4, 0)

        create_button = QtWidgets.QPushButton()
        create_button.setText("Create")
        create_button.clicked.connect(self._create_button_clicked)
        palette_creation_layout.addWidget(create_button, 4, 1)

        self.update_button = QtWidgets.QPushButton()
        self.update_button.setEnabled(False)
        self.update_button.setText("Update")
        self.update_button.clicked.connect(self._update_button_clicked)
        palette_creation_layout.addWidget(self.update_button, 4, 2)

        self.delete_button = QtWidgets.QPushButton()
        self.delete_button.setEnabled(False)
        self.delete_button.setText("Delete")
        self.delete_button.clicked.connect(self._delete_button_clicked)
        palette_creation_layout.addWidget(self.delete_button, 4, 3)

        palette_creation.setLayout(palette_creation_layout)
        main_layout.addWidget(palette_creation)

        self.setLayout(main_layout)
        self._sync_color_widgets(True)


    def _update_dropdowns_with_color_schemes(self, key_to_add: str | None = None, index_to_remove: int | None = None):
        if key_to_add is not None:
            self.saved_palettes.addItem(key_to_add)

            for dropdown in self.drop_downs:
                dropdown.addItem(key_to_add)

        elif index_to_remove is not None:
            self.saved_palettes.removeItem(index_to_remove)

            for dropdown in self.drop_downs:
                dropdown.removeItem(index_to_remove)


    def _custom_colors_toggle(self):
        is_checked = self.sender().isChecked()
        game_boy.prefer_dmg_colors(is_checked)


    def _use_individual_palettes(self):
        game_boy.use_individual_palettes(not self.sender().isChecked())


    def _set_custom_palette(self, index: int):
        color_schemes = config.get_color_schemes()
        game_boy.set_custom_palette(index, color_schemes[self.sender().currentText()])


    def _create_button_clicked(self):
        name, entered = QtWidgets.QInputDialog().getText(self, "Name", "Enter a name for this color scheme")

        if entered:
            config.add_color_scheme(name, self.current_scheme)
            self._update_dropdowns_with_color_schemes(key_to_add=name)
            self.saved_palettes.setCurrentIndex(self.saved_palettes.count() - 1)


    def _update_button_clicked(self):
        key = self.saved_palettes.currentText()
        config.add_color_scheme(key, self.current_scheme)
        color_schemes = config.get_color_schemes()

        for index, drop_down in enumerate(self.drop_downs):
            game_boy.set_custom_palette(index, color_schemes[drop_down.currentText()])


    def _delete_button_clicked(self):
        key_to_remove = self.saved_palettes.currentText()
        config.delete_color_scheme(key_to_remove)
        index_to_remove = self.saved_palettes.currentIndex()
        self._update_dropdowns_with_color_schemes(index_to_remove=index_to_remove)
        color_schemes = config.get_color_schemes()
        self.current_scheme = color_schemes[self.saved_palettes.currentText()]
        self._sync_color_widgets(True)


    def _saved_palette_changed(self):
        key = self.sender().currentText()
        index = self.sender().currentIndex()

        if index < 5:
            self.update_button.setEnabled(False)
            self.delete_button.setEnabled(False)
        else:
            self.update_button.setEnabled(True)
            self.delete_button.setEnabled(True)

        color_schemes = config.get_color_schemes()
        self.current_scheme = color_schemes[key]
        self._sync_color_widgets(True)


    def _color_clicked(self, button_index: int):
        self.selected_index = button_index
        self._sync_color_widgets(True)


    def _color_text_changed(self, color_index: int):
        try:
            new_value = int(self.sender().text())
        except ValueError:
            new_value = 0

        if new_value < 0:
            new_value = 0
        elif new_value > 255:
            new_value = 255

        self.current_scheme[self.selected_index*3 + color_index] = new_value
        self._sync_color_widgets(True)


    def _slider_moved(self, color_index: int):
       self.current_scheme[self.selected_index*3 + color_index] = self.sender().value()
       self._sync_color_widgets(False)


    def _sync_color_widgets(self, update_sliders: bool):
        for i in range(4):
            r, g, b = [self.current_scheme[i*3], self.current_scheme[i*3 + 1], self.current_scheme[i*3 + 2]]

            if i == self.selected_index:
                self.color_buttons[i].setFixedSize(60, 60)
                self.color_buttons[i].setStyleSheet(f"background-color: rgb({r}, {g}, {b}); border: 2px solid black")

                if update_sliders:
                    self.color_sliders[0].setValue(r)
                    self.color_sliders[1].setValue(g)
                    self.color_sliders[2].setValue(b)

                self.color_texts[0].setText(str(r))
                self.color_texts[1].setText(str(g))
                self.color_texts[2].setText(str(b))
            else:
                self.color_buttons[i].setFixedSize(50, 50)
                self.color_buttons[i].setStyleSheet(f"background-color: rgb({r}, {g}, {b})")
