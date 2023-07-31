import game_boy.game_boy as game_boy
from PyQt6 import QtCore, QtGui, QtWidgets

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
        self.sound_tab = QtWidgets.QWidget(self)

        self._init_sound_tab()

        self.addTab(self.sound_tab, "Sound")

        self.setWindowTitle("Preferences")

    def _init_sound_tab(self):
        main_layout = QtWidgets.QGridLayout()

        # Channels
        channels = QtWidgets.QGroupBox("Channels")
        channel_layout = QtWidgets.QVBoxLayout()

        for i in range(1, 5):
            widget = QtWidgets.QCheckBox(f"Channel {i}")
            widget.setChecked(True)
            widget.toggled.connect(self._toggle_sound_channel)
            channel_layout.addWidget(widget)

        channels.setLayout(channel_layout)
        main_layout.addWidget(channels, 0, 0)

        # Miscellaneous
        misc = QtWidgets.QGroupBox("Options")
        misc_layout = QtWidgets.QVBoxLayout()

        self.mute_box = QtWidgets.QCheckBox("Mute")
        self.mute_box.clicked.connect(self._toggle_mute)
        misc_layout.addWidget(self.mute_box)

        mono_audio_box = QtWidgets.QCheckBox("Mono output")
        mono_audio_box.toggled.connect(self._toggle_mono_audio)
        misc_layout.addWidget(mono_audio_box)

        sample_rate_label = QtWidgets.QLabel("Sample Rate")
        sample_rate = QtWidgets.QComboBox()
        sample_rate.addItems(["8000", "11025", "22050", "24000", "44100", "48000"])
        sample_rate.setCurrentIndex(4)
        sample_rate.activated.connect(self._set_sample_rate)
        misc_layout.addWidget(sample_rate_label)
        misc_layout.addWidget(sample_rate)

        misc.setLayout(misc_layout)
        main_layout.addWidget(misc, 0, 1)

        volume = QtWidgets.QGroupBox("Volume")
        volume_layout = QtWidgets.QVBoxLayout()
        self.volume_slider = QtWidgets.QSlider(QtCore.Qt.Orientation.Horizontal)
        self.volume_slider.setMinimum(0)
        self.volume_slider.setMaximum(100)
        self.volume_slider.setSingleStep(1)
        self.volume_slider.setValue(self.saved_volume)
        self.volume_slider.sliderReleased.connect(self._set_volume)
        volume_layout.addWidget(self.volume_slider)

        volume.setLayout(volume_layout)
        main_layout.addWidget(volume, 1, 0, 1, 2)

        self.sound_tab.setLayout(main_layout)

    def _toggle_sound_channel(self):
        enabled = self.sender().isChecked()
        channel = self.sender().text()[-1]

        if enabled:
            print(f"{channel} enabled")
        else:
            print(f"{channel} disabled")

    def _toggle_mute(self):
        if self.sender().isChecked():
            self.saved_volume = self.volume_slider.value()
            self.volume_slider.setValue(0)
            self.muted = True
            print("Muted")
        else:
            self.volume_slider.setValue(self.saved_volume)
            print("Unmuted")

    def _toggle_mono_audio(self):
        if self.sender().isChecked():
            print("Mono")
        else:
            print("Stereo")

    def _set_sample_rate(self):
        sample_rate = int(self.sender().currentText())
        print(sample_rate)

    def _set_volume(self):
        new_volume = self.sender().value()

        if self.muted and new_volume > 0:
            self.mute_box.setChecked(False)

        print(new_volume)
