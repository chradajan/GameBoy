from functools import partial
from typing import List

from PyQt6 import QtCore, QtWidgets

import config.config as config
import game_boy.game_boy as game_boy
import sdl.sdl_audio as sdl_audio

class SoundTab(QtWidgets.QWidget):
    def __init__(self):
        super().__init__()

        self.channel_boxes: List[QtWidgets.QCheckBox] = []

        self.volume_slider: QtWidgets.QSlider = None
        self.saved_volume = 100

        self.mute_box: QtWidgets.QCheckBox = None
        self.mono_box: QtWidgets.QCheckBox = None
        self.muted = False

        self.sample_rate_dropdown: QtWidgets.QComboBox = None

        self._init_ui()


    def _init_ui(self):
        main_layout = QtWidgets.QGridLayout()

        # Channels
        channels = QtWidgets.QGroupBox("Channels")
        channel_layout = QtWidgets.QVBoxLayout()

        for i in range(1, 5):
            self.channel_boxes.append(QtWidgets.QCheckBox(f"Channel {i}"))
            self.channel_boxes[-1].setChecked(True)
            self.channel_boxes[-1].toggled.connect(partial(self._toggle_sound_channel, i))
            channel_layout.addWidget(self.channel_boxes[-1])

        channels.setLayout(channel_layout)
        main_layout.addWidget(channels, 0, 0)

        # Options
        options = QtWidgets.QGroupBox("Options")
        options_layout = QtWidgets.QVBoxLayout()

        self.mute_box = QtWidgets.QCheckBox("Mute")
        self.mute_box.clicked.connect(self._toggle_mute)
        options_layout.addWidget(self.mute_box)

        self.mono_box = QtWidgets.QCheckBox("Mono output")
        self.mono_box.toggled.connect(self._toggle_mono_audio)
        options_layout.addWidget(self.mono_box)

        self.sample_rate_dropdown = QtWidgets.QComboBox()
        self.sample_rate_dropdown.addItems(["8000", "11025", "22050", "24000", "44100", "48000"])
        self.sample_rate_dropdown.setCurrentIndex(4)
        self.sample_rate_dropdown.activated.connect(self._set_sample_rate)
        options_layout.addWidget(QtWidgets.QLabel("Sample Rate"))
        options_layout.addWidget(self.sample_rate_dropdown)

        options.setLayout(options_layout)
        main_layout.addWidget(options, 0, 1)

        # Volume
        volume = QtWidgets.QGroupBox("Volume")
        volume_layout = QtWidgets.QVBoxLayout()
        self.volume_slider = QtWidgets.QSlider(QtCore.Qt.Orientation.Horizontal)
        self.volume_slider.setMinimum(0)
        self.volume_slider.setMaximum(100)
        self.volume_slider.setSingleStep(1)
        self.volume_slider.setPageStep(0)
        self.volume_slider.setValue(config.get_saved_volume())
        self.volume_slider.sliderMoved.connect(self._slider_moved)
        self.volume_slider.sliderReleased.connect(self._slider_released)
        volume_layout.addWidget(self.volume_slider)

        volume.setLayout(volume_layout)
        main_layout.addWidget(volume, 1, 0, 1, 2)

        spacer = QtWidgets.QWidget()
        spacer.setSizePolicy(QtWidgets.QSizePolicy.Policy.Preferred, QtWidgets.QSizePolicy.Policy.Expanding)
        main_layout.addWidget(spacer, 2, 0)

        self.setLayout(main_layout)


    def _toggle_sound_channel(self, channel: int):
        enabled = self.sender().isChecked()
        game_boy.enable_sound_channel(channel, enabled)


    def _toggle_mute(self):
        if self.sender().isChecked():
            self.saved_volume = self.volume_slider.value()
            self.volume_slider.setValue(0)
            self.muted = True
            game_boy.set_volume(0.0)
        else:
            self.volume_slider.setValue(self.saved_volume)
            game_boy.set_volume(self.saved_volume / 100)


    def _toggle_mono_audio(self):
        game_boy.set_mono_audio(self.sender().isChecked())


    def _set_sample_rate(self):
        sample_rate = int(self.sender().currentText())
        game_boy.set_sample_rate(sample_rate)
        sdl_audio.update_sample_rate(sample_rate)


    def _slider_moved(self):
        new_volume = self.sender().value()

        if self.muted and new_volume > 0:
            self.mute_box.setChecked(False)

        game_boy.set_volume(new_volume / 100)


    def _slider_released(self):
        config.set_saved_volume(self.sender().value())


    def restore_defaults(self):
        for i in range(4):
            self.channel_boxes[i].setChecked(True)
            game_boy.enable_sound_channel(i+1, True)

        config.set_saved_volume(100)
        game_boy.set_volume(1.0)
        self.volume_slider.setValue(100)
        self.saved_volume = 100

        self.mute_box.setChecked(False)
        self.muted = False

        game_boy.set_mono_audio(False)
        self.mono_box.setChecked(False)

        game_boy.set_sample_rate(44100)
        sdl_audio.update_sample_rate(44100)
        self.sample_rate_dropdown.setCurrentIndex(4)
