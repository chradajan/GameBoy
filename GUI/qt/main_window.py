import game_boy.game_boy as game_boy
import sdl.sdl_audio as sdl_audio
import time
from PyQt5 import QtGui, QtWidgets

WIDTH = 160
HEIGHT = 144

class MainWindow(QtWidgets.QMainWindow):
    def __init__(self, audio_device_id: int):
        super().__init__()
        self.window_scale = 4
        self.audio_device_id = audio_device_id
        self.move_timer = time.perf_counter()

        self._init_ui()
        self.show()


    def _init_ui(self):
        # Set window settings
        self.setWindowTitle("Game Boy")
        self.setContentsMargins(0, 0, 0, 0)

        # Create menu bar
        file = self.menuBar().addMenu("File")
        options = self.menuBar().addMenu("Options")
        debug = self.menuBar().addMenu("Debug")

        # File menu
        file_loadrom_action = QtWidgets.QAction("Load ROM...", self)
        file_loadrom_action.triggered.connect(self.load_rom_trigger)
        file_loadrom = file.addAction(file_loadrom_action)

        file_exit_action = QtWidgets.QAction("Exit", self)
        file_exit_action.triggered.connect(QtWidgets.qApp.quit)
        file_exit = file.addAction(file_exit_action)

        # Options menu
        options_windowsize = options.addMenu("Window size")

        for size in ["2x2", "3x3", "4x4", "5x5", "6x6"]:
            action = QtWidgets.QAction(size, self)
            action.triggered.connect(self.window_size_trigger)
            options_windowsize.addAction(action)

        options_game_speed = options.addMenu("Game speed")

        # Debug menu
        debug_sound_channels = debug.addMenu("Sound channels")

        # Create LCD output label
        self.lcd = QtWidgets.QLabel(self)
        self.setCentralWidget(self.lcd)

        # Update window size
        self._resize_window()


    def _resize_window(self):
        width = WIDTH * self.window_scale
        lcd_height = HEIGHT * self.window_scale
        window_height = self.menuBar().height() + lcd_height
        self.setFixedSize(width, window_height)
        self.lcd.resize(width, lcd_height)


    def refresh_screen(self):
        now = time.perf_counter()

        if now - self.move_timer < 0.05:
            return

        image = QtGui.QImage(game_boy.get_frame_buffer(),
                             WIDTH,
                             HEIGHT,
                             QtGui.QImage.Format.Format_RGB888)

        self.lcd.setPixmap(QtGui.QPixmap.fromImage(image).scaled(self.lcd.width(), self.lcd.height()))

    #######################################################################
    #    __  __                                 _   _                     #
    #   |  \/  |                      /\       | | (_)                    #
    #   | \  / | ___ _ __  _   _     /  \   ___| |_ _  ___  _ __  ___     #
    #   | |\/| |/ _ \ '_ \| | | |   / /\ \ / __| __| |/ _ \| '_ \/ __|    #
    #   | |  | |  __/ | | | |_| |  / ____ \ (__| |_| | (_) | | | \__ \    #
    #   |_|  |_|\___|_| |_|\__,_| /_/    \_\___|\__|_|\___/|_| |_|___/    #
    #                                                                     #
    #######################################################################

    def load_rom_trigger(self):
        pass

    def window_size_trigger(self):
        sender_label = self.sender().text()

        if sender_label == "2x2":
            self.window_scale = 2
        elif sender_label == "3x3":
            self.window_scale = 3
        elif sender_label == "4x4":
            self.window_scale = 4
        elif sender_label == "5x5":
            self.window_scale = 5
        elif sender_label == "6x6":
            self.window_scale = 6
        else:
            return

        sdl_audio.lock_audio(self.audio_device_id)
        self._resize_window()
        sdl_audio.unlock_audio(self.audio_device_id)

    ########################################
    #     ______                           #
    #    |  ____|             | |          #
    #    | |____   _____ _ __ | |_ ___     #
    #    |  __\ \ / / _ \ '_ \| __/ __|    #
    #    | |___\ V /  __/ | | | |_\__ \    #
    #    |______\_/ \___|_| |_|\__|___/    #
    #                                      #
    ########################################

    def moveEvent(self, event: QtGui.QMoveEvent):
        self.move_timer = time.perf_counter()
        super().moveEvent(event)
