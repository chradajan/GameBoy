from functools import partial
from typing import List
from PyQt6 import QtCore, QtWidgets

class ColorTab(QtWidgets.QWidget):
    def __init__(self, parent:QtWidgets.QWidget):
        super().__init__(parent)

        self.bg_select = None
        self.win_select = None
        self.obp0_select = None
        self.obp1_select = None

        self.r_text = None
        self.g_text = None
        self.b_text = None

        self.r_slider = None
        self.g_slider = None
        self.b_slider = None

        self.colors: List[QtWidgets.QPushButton] = []

        self.saved_palettes = None

        self._init_ui()


    def _init_ui(self):
        main_layout = QtWidgets.QHBoxLayout()

        # Palette selection
        palette_selection = QtWidgets.QGroupBox("Palette Selection")
        palette_selection_layout = QtWidgets.QVBoxLayout()

        overwrite_box = QtWidgets.QCheckBox("Use custom colors")
        overwrite_box.toggled.connect(self._custom_colors_toggle)
        palette_selection_layout.addWidget(overwrite_box)

        sync_box = QtWidgets.QCheckBox("Singular palette")
        sync_box.toggled.connect(self._sync_box_toggle)
        palette_selection_layout.addWidget(sync_box)

        bg_label = QtWidgets.QLabel("BG")
        self.bg_select = QtWidgets.QComboBox()
        self.bg_select.addItems(["Foo", "Bar"])
        self.bg_select.setCurrentIndex(0)
        self.bg_select.activated.connect(self._set_bg_palette)
        palette_selection_layout.addWidget(bg_label)
        palette_selection_layout.addWidget(self.bg_select)

        win_label = QtWidgets.QLabel("WIN")
        self.win_select = QtWidgets.QComboBox()
        self.win_select.addItems(["Foo", "Bar"])
        self.win_select.setCurrentIndex(0)
        self.win_select.activated.connect(self._set_win_palette)
        palette_selection_layout.addWidget(win_label)
        palette_selection_layout.addWidget(self.win_select)

        obp0 = QtWidgets.QLabel("OBP0")
        self.obp0_select = QtWidgets.QComboBox()
        self.obp0_select.addItems(["Foo", "Bar"])
        self.obp0_select.setCurrentIndex(0)
        self.obp0_select.activated.connect(self._set_obp0_palette)
        palette_selection_layout.addWidget(obp0)
        palette_selection_layout.addWidget(self.obp0_select)

        obp1 = QtWidgets.QLabel("OBP1")
        self.obp1_select = QtWidgets.QComboBox()
        self.obp1_select.addItems(["Foo", "Bar"])
        self.obp1_select.setCurrentIndex(0)
        self.obp1_select.activated.connect(self._set_obp1_palette)
        palette_selection_layout.addWidget(obp1)
        palette_selection_layout.addWidget(self.obp1_select)

        palette_selection.setLayout(palette_selection_layout)
        main_layout.addWidget(palette_selection)

        # Palette creator
        palette_creation = QtWidgets.QGroupBox("Create Palette")
        palette_creation_layout = QtWidgets.QGridLayout()

        self.r_text = QtWidgets.QLineEdit()
        self.r_text.setMaximumWidth(40)
        self.r_slider = QtWidgets.QSlider(QtCore.Qt.Orientation.Horizontal)
        self.r_slider.setMinimum(0)
        self.r_slider.setMaximum(255)
        self.r_slider.setSingleStep(1)
        self.r_slider.setPageStep(0)
        palette_creation_layout.addWidget(self.r_slider, 0, 0, 1, 3)
        palette_creation_layout.addWidget(self.r_text, 0, 3)

        self.g_text = QtWidgets.QLineEdit()
        self.g_text.setMaximumWidth(40)
        self.g_slider = QtWidgets.QSlider(QtCore.Qt.Orientation.Horizontal)
        self.g_slider.setMinimum(0)
        self.g_slider.setMaximum(255)
        self.g_slider.setSingleStep(1)
        self.g_slider.setPageStep(0)
        palette_creation_layout.addWidget(self.g_slider, 1, 0, 1, 3)
        palette_creation_layout.addWidget(self.g_text, 1, 3)

        self.b_text = QtWidgets.QLineEdit()
        self.b_text.setMaximumWidth(40)
        self.b_slider = QtWidgets.QSlider(QtCore.Qt.Orientation.Horizontal)
        self.b_slider.setMinimum(0)
        self.b_slider.setMaximum(255)
        self.b_slider.setSingleStep(1)
        self.b_slider.setPageStep(0)
        palette_creation_layout.addWidget(self.b_slider, 2, 0, 1, 3)
        palette_creation_layout.addWidget(self.b_text, 2, 3)

        colors = QtWidgets.QGroupBox()
        colors_layout = QtWidgets.QHBoxLayout()

        for i in range(4):
            self.colors.append(QtWidgets.QPushButton())
            self.colors[-1].setFixedSize(50, 50)
            self.colors[-1].setStyleSheet(f"background-color: rgb(0, 0, 0)")
            self.colors[-1].clicked.connect(partial(self._color_clicked, i))
            colors_layout.addWidget(self.colors[-1])

        colors.setLayout(colors_layout)
        palette_creation_layout.addWidget(colors, 3, 0, 1, 4)

        self.saved_palettes = QtWidgets.QComboBox()
        self.saved_palettes.setMinimumWidth(100)
        self.saved_palettes.setMaximumWidth(100)
        self.saved_palettes.addItems(["Foo", "Bar"])
        self.saved_palettes.setCurrentIndex(0)
        palette_creation_layout.addWidget(self.saved_palettes, 4, 0)

        create_button = QtWidgets.QPushButton()
        create_button.setText("Create")
        create_button.clicked.connect(self._create_button)
        palette_creation_layout.addWidget(create_button, 4, 1)

        update_button = QtWidgets.QPushButton()
        update_button.setText("Update")
        update_button.clicked.connect(self._update_button)
        palette_creation_layout.addWidget(update_button, 4, 2)

        delete_button = QtWidgets.QPushButton()
        delete_button.setText("Delete")
        delete_button.clicked.connect(self._delete_button)
        palette_creation_layout.addWidget(delete_button, 4, 3)

        palette_creation.setLayout(palette_creation_layout)
        main_layout.addWidget(palette_creation)

        self.setLayout(main_layout)


    def _custom_colors_toggle(self):
        pass


    def _sync_box_toggle(self):
        pass


    def _set_bg_palette(self):
        pass


    def _set_win_palette(self):
        pass


    def _set_obp0_palette(self):
        pass


    def _set_obp1_palette(self):
        pass


    def _create_button(self):
        pass


    def _update_button(self):
        pass


    def _delete_button(self):
        pass


    def _color_clicked(self, button_index: int):
        for i in range(4):
            if i == button_index:
                self.colors[i].setFixedSize(60, 60)
                self.colors[i].setStyleSheet(f"background-color: rgb(255, 0, 0); border: 2px solid black")
            else:
                self.colors[i].setFixedSize(50, 50)
                self.colors[i].setStyleSheet(f"background-color: rgb(0, 0, 0)")
