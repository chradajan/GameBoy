import datetime
import threading
from functools import partial
from pathlib import Path
from typing import Set

from PyQt6 import QtCore, QtGui, QtWidgets

import config.config as config
import controller.controller as controller
import game_boy.game_boy as game_boy
import sdl.sdl_audio as sdl_audio
from qt.preferences_window import PreferencesWindow

WIDTH = 160
HEIGHT = 144

class MainWindow(QtWidgets.QMainWindow):
    window_sizes = {"2x2": 2, "3x3": 3, "4x4": 4, "5x5": 5, "6x6": 6}
    game_speeds = {"x1/4": 0.25, "x1/2": 0.5, "x1": 1.0, "x2": 2.0, "x3": 3.0, "x4": 4.0}

    def __init__(self, root_directory: Path):
        super().__init__()
        self.game_speed = 1.0
        self.root_directory = root_directory

        # File menu
        self.recents_menu: QtWidgets.QMenu = None

        # Options menu
        self.preferences_window = None

        # Window information
        self.window_scale = 4
        self.game_title = "Game Boy"
        self.game_loaded = False

        # I/O
        self.keys_pressed: Set[int] = set()
        self.gamepad: controller.Gamepad = {
            "left_stick_y_up": 0.0,
            "left_stick_y_down": 0.0,
            "left_stick_x_right": 0.0,
            "left_stick_x_left": 0.0,
            "right_stick_y_up": 0.0,
            "right_stick_y_down": 0.0,
            "right_stick_x_right": 0.0,
            "right_stick_x_left": 0.0,
            "left_trigger": 0,
            "right_trigger": 0,
            "left_bumper": 0,
            "right_bumper": 0,
            "a": 0,
            "b": 0,
            "x": 0,
            "y": 0,
            "left_thumb": 0,
            "right_thumb": 0,
            "select": 0,
            "start": 0,
            "d_pad_left": 0,
            "d_pad_right": 0,
            "d_pad_up": 0,
            "d_pad_down": 0,
        }

        # Gamepad
        self.gamepad_thread = threading.Thread(target=partial(controller.monitor_gamepad, self.gamepad))
        self.gamepad_thread.daemon = True
        self.gamepad_thread.start()

        # GUI components
        self.last_checked_window_size: QtGui.QAction = None
        self.last_checked_game_speed: QtGui.QAction = None
        self.save_state_actions = []
        self.load_state_actions = []
        self.save_state_timer = QtCore.QTimer(self)
        self.save_state_timer.setSingleShot(True)
        self.save_state_timer.setInterval(150)
        self.save_state_timer.timeout.connect(self._refresh_save_state_menus)

        # FPS info
        self.frame_counter = 0
        self.fps_timer = QtCore.QTimer(self)
        self.fps_timer.timeout.connect(self._update_fps_counter)
        self.fps_timer.start(1000)

        self._init_ui()
        self.show()


    def _init_ui(self):
        # Set window settings
        self.setWindowTitle(f"{self.game_title} (0 fps)")
        self.setContentsMargins(0, 0, 0, 0)

        # Create menu bar
        file = self.menuBar().addMenu("File")
        emulation = self.menuBar().addMenu("Emulation")
        options = self.menuBar().addMenu("Options")

        # File menu
        file_loadrom_action = QtGui.QAction("Load ROM...", self)
        file_loadrom_action.triggered.connect(self._load_rom_trigger)
        file.addAction(file_loadrom_action)

        self.recents_menu = file.addMenu("Recent ROMs")
        self._refresh_recent_roms()

        file_exit_action = QtGui.QAction("Exit", self)
        file_exit_action.triggered.connect(QtWidgets.QApplication.quit)
        file.addAction(file_exit_action)

        # Emulation menu
        emulation_pause_action = QtGui.QAction("Pause", self)
        emulation_pause_action.triggered.connect(self._pause_emulation_trigger)
        emulation_pause_action.setCheckable(True)
        emulation.addAction(emulation_pause_action)

        emulation_gamespeed = emulation.addMenu("Game speed")

        for speed_str, speed_float in self.game_speeds.items():
            action = QtGui.QAction(speed_str, self)
            action.setCheckable(True)

            if speed_float == self.game_speed:
                action.setChecked(True)
                self.last_checked_game_speed = action

            action.triggered.connect(self._game_speed_trigger)
            emulation_gamespeed.addAction(action)

        emulation.addSeparator()

        emulation_savestate = emulation.addMenu("Save state")
        emulation_loadstate = emulation.addMenu("Load state")

        for i in range(1, 6):
            self.save_state_actions.append(QtGui.QAction(f"Save to slot {i} - Empty", self))
            self.load_state_actions.append(QtGui.QAction(f"Load from slot {i} - Empty", self))
            self.save_state_actions[-1].triggered.connect(self._create_save_state_trigger)
            self.load_state_actions[-1].triggered.connect(self._load_save_state_trigger)
            self.save_state_actions[-1].setShortcut(QtGui.QKeySequence(f"{i}"))
            self.load_state_actions[-1].setShortcut(QtGui.QKeySequence(f"Ctrl+{i}"))
            emulation_savestate.addAction(self.save_state_actions[-1])
            emulation_loadstate.addAction(self.load_state_actions[-1])

        emulation.addSeparator()

        emulation_reset_action = QtGui.QAction("Restart", self)
        emulation_reset_action.triggered.connect(self._reset_trigger)
        emulation.addAction(emulation_reset_action)

        # Options menu
        options_windowsize = options.addMenu("Window size")

        for size_str, size_int in self.window_sizes.items():
            action = QtGui.QAction(size_str, self)
            action.setCheckable(True)

            if size_int == self.window_scale:
                action.setChecked(True)
                self.last_checked_window_size = action

            action.triggered.connect(self._window_size_trigger)
            options_windowsize.addAction(action)

        options.addSeparator()

        preferences_action = QtGui.QAction("Preferences", self)
        preferences_action.triggered.connect(self._preferences_window_trigger)
        options.addAction(preferences_action)

        # Create LCD output label
        self.lcd = QtWidgets.QLabel(self)
        self.setCentralWidget(self.lcd)

        # Update window size
        self._resize_window()


    def _update_joypad(self):
        keyboard_bindings = config.get_keyboard_bindings()
        gamepad_bindings = config.get_gamepad_bindings()

        joypad = game_boy.JoyPad(
            (keyboard_bindings["down"] in self.keys_pressed) or (self.gamepad[gamepad_bindings["down"]]),
            (keyboard_bindings["up"] in self.keys_pressed) or (self.gamepad[gamepad_bindings["up"]]),
            (keyboard_bindings["left"] in self.keys_pressed) or (self.gamepad[gamepad_bindings["left"]]),
            (keyboard_bindings["right"] in self.keys_pressed) or (self.gamepad[gamepad_bindings["right"]]),
            (keyboard_bindings["start"] in self.keys_pressed) or (self.gamepad[gamepad_bindings["start"]]),
            (keyboard_bindings["select"] in self.keys_pressed) or (self.gamepad[gamepad_bindings["select"]]),
            (keyboard_bindings["b"] in self.keys_pressed) or (self.gamepad[gamepad_bindings["b"]]),
            (keyboard_bindings["a"] in self.keys_pressed) or (self.gamepad[gamepad_bindings["a"]]),
        )
        game_boy.set_joypad_state(joypad)

    ##########################
    #      ____ _   _ ___    #
    #     / ___| | | |_ _|   #
    #    | |  _| | | || |    #
    #    | |_| | |_| || |    #
    #     \____|\___/|___|   #
    #                        #
    ##########################

    def refresh_screen(self):
        """Update the LCD label with the latest frame buffer contents."""
        self.frame_counter += 1
        self._update_joypad()

        image = QtGui.QImage(game_boy.get_frame_buffer(),
                             WIDTH,
                             HEIGHT,
                             QtGui.QImage.Format.Format_RGB888)

        self.lcd.setPixmap(QtGui.QPixmap.fromImage(image).scaled(self.lcd.width(), self.lcd.height()))


    def _update_fps_counter(self):
        """Update the fps counter in the window title based on performance over the last second."""
        self.setWindowTitle(f"{self.game_title} ({self.frame_counter} fps)")
        self.frame_counter = 0


    def _resize_window(self):
        """Resize the window based on window_scale."""
        width = WIDTH * self.window_scale
        lcd_height = HEIGHT * self.window_scale
        window_height = self.menuBar().height() + lcd_height
        self.setFixedSize(width, window_height)
        self.lcd.resize(width, lcd_height)


    def _refresh_save_state_menus(self):
        """Check if any save states exist for the current game. For any found save states, update the corresponding submenu."""
        if not self.game_loaded:
            for i in range(0, 5):
                self.save_state_actions[i].setText(f"Save to slot {i+1} - Empty")
                self.load_state_actions[i].setText(f"Load from slot {i+1} - Empty")

            return

        save_state_path = self.root_directory / "save_states"

        for i in range(0, 5):
            save_state = Path(save_state_path / f"{self.game_title}.s{i+1}")

            if save_state.is_file():
                last_modified = datetime.datetime.fromtimestamp(save_state.stat().st_mtime)
                last_modified_str = last_modified.strftime("%m/%d/%Y %I:%M:%S %p")
                self.save_state_actions[i].setText(f"Save to slot {i+1} - {last_modified_str}")
                self.load_state_actions[i].setText(f"Load from slot {i+1} - {last_modified_str}")
            else:
                self.save_state_actions[i].setText(f"Save to slot {i+1} - Empty")
                self.load_state_actions[i].setText(f"Load from slot {i+1} - Empty")


    def _refresh_recent_roms(self):
        """Refresh the recent roms list with the current 10 most recent roms."""
        self.recents_menu.clear()
        recent_roms = config.get_recent_roms()

        if not recent_roms:
            self.recents_menu.setDisabled(True)
        else:
            self.recents_menu.setDisabled(False)

        for rom_path in recent_roms:
            action = QtGui.QAction(rom_path, self)
            action.triggered.connect(partial(self._load_recent_rom_trigger, rom_path))
            self.recents_menu.addAction(action)

    ########################################################################
    #     __  __                                 _   _                     #
    #    |  \/  |                      /\       | | (_)                    #
    #    | \  / | ___ _ __  _   _     /  \   ___| |_ _  ___  _ __  ___     #
    #    | |\/| |/ _ \ '_ \| | | |   / /\ \ / __| __| |/ _ \| '_ \/ __|    #
    #    | |  | |  __/ | | | |_| |  / ____ \ (__| |_| | (_) | | | \__ \    #
    #    |_|  |_|\___|_| |_|\__,_| /_/    \_\___|\__|_|\___/|_| |_|___/    #
    #                                                                      #
    ########################################################################

    def _load_rom_trigger(self):
        """Open a file dialog for user to select a Game Boy ROM file to load."""
        file_dialog = QtWidgets.QFileDialog(self)
        file_dialog.setFileMode(QtWidgets.QFileDialog.FileMode.ExistingFile)
        search_directory = config.get_last_rom_directory()
        rom_path, _ = file_dialog.getOpenFileName(self,
                                                  "Load ROM...",
                                                  search_directory,
                                                  "Game Boy files (*.gb *.gbc)")

        if rom_path:
            config.update_last_rom_directory(rom_path)
            config.add_recent_rom(rom_path)
            self._refresh_recent_roms()
            sdl_audio.lock_audio()
            self.game_title = game_boy.insert_cartridge(rom_path)

            if self.game_title:
                self.game_loaded = True
                game_boy.power_on()
            else:
                self.game_loaded = False
                self.game_title = "Game Boy"

            self._refresh_save_state_menus()
            sdl_audio.unlock_audio()


    def _pause_emulation_trigger(self):
        """Halt execution of the emulator until pause button is clicked again."""
        if self.sender().isChecked():
            sdl_audio.lock_audio()
        else:
            sdl_audio.unlock_audio()


    def _create_save_state_trigger(self):
        """Create a save state and store to the selected slot."""
        if not self.game_loaded:
            return

        save_state_path = self.root_directory / "save_states"
        save_state_path.mkdir(parents=True, exist_ok=True)
        index = self.sender().text()[13]
        save_state_path = save_state_path / f"{self.game_title}.s{index}"
        game_boy.create_save_state(save_state_path)
        self.save_state_timer.start()


    def _load_save_state_trigger(self):
        """Load a save state from the selected slot."""
        if not self.game_loaded:
            return

        save_state_path = self.root_directory / "save_states"
        index = self.sender().text()[15]
        save_state_path = save_state_path / f"{self.game_title}.s{index}"
        game_boy.load_save_state(save_state_path)


    def _game_speed_trigger(self):
        """Change the Game Boy clock multiplier to speed up or slow down gameplay."""
        self.last_checked_game_speed.setChecked(False)
        self.last_checked_game_speed = self.sender()

        sdl_audio.lock_audio()
        game_boy.change_emulation_speed(self.game_speeds[self.sender().text()])
        sdl_audio.unlock_audio()


    def _reset_trigger(self):
        """Restart the Game Boy."""
        game_boy.power_on()


    def _window_size_trigger(self):
        """Set the size of the screen to the selected option."""
        self.last_checked_window_size.setChecked(False)
        self.last_checked_window_size = self.sender()
        self.window_scale = self.window_sizes[self.sender().text()]

        sdl_audio.lock_audio()
        self._resize_window()
        sdl_audio.unlock_audio()


    def _preferences_window_trigger(self):
        "Open the preferences window."
        if self.preferences_window is None:
            self.preferences_window = PreferencesWindow()

        self.preferences_window.show()


    def _load_recent_rom_trigger(self, rom_path: Path):
        """Load a ROM from the recents menu."""
        config.add_recent_rom(rom_path)
        self._refresh_recent_roms()

        sdl_audio.lock_audio()
        self.game_title = game_boy.insert_cartridge(rom_path)

        if self.game_title:
            self.game_loaded = True
            game_boy.power_on()
        else:
            self.game_loaded = False
            self.game_title = "Game Boy"

        self._refresh_save_state_menus()
        sdl_audio.unlock_audio()


    ########################################
    #     ______                           #
    #    |  ____|             | |          #
    #    | |____   _____ _ __ | |_ ___     #
    #    |  __\ \ / / _ \ '_ \| __/ __|    #
    #    | |___\ V /  __/ | | | |_\__ \    #
    #    |______\_/ \___|_| |_|\__|___/    #
    #                                      #
    ########################################

    def keyPressEvent(self, event: QtGui.QKeyEvent | None):
        if event is None:
            return

        self.keys_pressed.add(event.key())
        super().keyPressEvent(event)


    def keyReleaseEvent(self, event: QtGui.QKeyEvent | None):
        if event is None:
            return

        self.keys_pressed.discard(event.key())
        super().keyReleaseEvent(event)


    def closeEvent(self, event: QtGui.QCloseEvent | None):
        if event is None:
            return

        super().closeEvent(event)
        QtWidgets.QApplication.quit()
