import time
from typing import TypedDict

import inputs

MAX_TRIGGER_VALUE = 2 ** 8
MAX_JOYSTICK_VALUE = 2 ** 15


class Gamepad(TypedDict):
    """Class representing which buttons are currently pressed on a gamepad."""
    left_stick_y_up: int
    left_stick_y_down: int
    left_stick_x_right: int
    left_stick_x_left: int
    right_stick_y_up: int
    right_stick_y_down: int
    right_stick_x_right: int
    right_stick_x_left: int
    left_trigger: int
    right_trigger: int
    left_bumper: int
    right_bumper: int
    a: int
    b: int
    x: int
    y: int
    left_thumb: int
    right_thumb: int
    select: int
    start: int
    d_pad_left: int
    d_pad_right: int
    d_pad_up: int
    d_pad_down: int


def get_first_gamepad_key() -> str:
    """Get the first button pressed on the gamepad.

    Returns:
        Name of gamepad button pressed.
    """
    while True:
        events = inputs.get_gamepad()
        for event in events:
            if event.code == 'ABS_Y':
                val = event.state / MAX_JOYSTICK_VALUE # [-1, 1]

                if val > 0.15:
                    return "left_stick_y_up"
                elif val < -0.15:
                    return "left_stick_y_down"
            elif event.code == 'ABS_X':
                val = event.state / MAX_JOYSTICK_VALUE # [-1, 1]

                if val > 0.15:
                    return "left_stick_x_right"
                elif val < -0.15:
                    return "left_stick_x_left"
            elif event.code == 'ABS_RY':
                val = event.state / MAX_JOYSTICK_VALUE # [-1, 1]

                if val > 0.15:
                    return "right_stick_y_up"
                elif val < -0.15:
                    return "right_stick_y_down"
            elif event.code == 'ABS_RX':
                val = event.state / MAX_JOYSTICK_VALUE # [-1, 1]

                if val > 0.15:
                    return "right_stick_x_right"
                elif val < -0.15:
                    return "right_stick_x_left"
            elif event.code == 'ABS_Z':
                val = event.state / MAX_TRIGGER_VALUE # [0, 1]

                if val > 0.03:
                    return "left_trigger"
            elif event.code == 'ABS_RZ':
                val = event.state / MAX_TRIGGER_VALUE # [0, 1]

                if val > 0.03:
                    return "right_trigger"
            elif event.code == 'BTN_TL':
                return "left_bumper"
            elif event.code == 'BTN_TR':
                return "right_bumper"
            elif event.code == 'BTN_SOUTH':
                return "a"
            elif event.code == 'BTN_EAST':
                return "b"
            elif event.code == 'BTN_WEST':
                return "x"
            elif event.code == 'BTN_NORTH':
                return "y"
            elif event.code == 'BTN_THUMBL':
                return "left_thumb"
            elif event.code == 'BTN_THUMBR':
                return "right_thumb"
            elif event.code == 'BTN_SELECT':
                return "start"
            elif event.code == 'BTN_START':
                return "select"
            elif event.code == 'ABS_HAT0Y':
                if event.state == 1:
                    return "d_pad_down"
                elif event.state == -1:
                    return "d_pad_up"
            elif event.code == 'ABS_HAT0X':
                if event.state == 1:
                    return "d_pad_right"
                elif event.state == -1:
                    return "d_pad_left"


def monitor_gamepad(gamepad: Gamepad):
    """Monitor for gamepad inputs. Never returns.

    Args:
        gamepad: Gamepad object to keep updating with current state.
    """
    while True:
        try:
            events = inputs.get_gamepad()
            for event in events:
                if event.code == 'ABS_Y':
                    val = event.state / MAX_JOYSTICK_VALUE # [-1, 1]

                    if val > 0.20:
                        gamepad["left_stick_y_up"] = 1
                        gamepad["left_stick_y_down"] = 0
                    elif val < -0.20:
                        gamepad["left_stick_y_up"] = 0
                        gamepad["left_stick_y_down"] = 1
                    else:
                        gamepad["left_stick_y_up"] = 0
                        gamepad["left_stick_y_down"] = 0
                elif event.code == 'ABS_X':
                    val = event.state / MAX_JOYSTICK_VALUE # [-1, 1]

                    if val > 0.20:
                        gamepad["left_stick_x_right"] = 1
                        gamepad["left_stick_x_left"] = 0
                    elif val < -0.20:
                        gamepad["left_stick_x_right"] = 0
                        gamepad["left_stick_x_left"] = 1
                    else:
                        gamepad["left_stick_x_right"] = 0
                        gamepad["left_stick_x_left"] = 0
                elif event.code == 'ABS_RY':
                    val = event.state / MAX_JOYSTICK_VALUE # [-1, 1]

                    if val > 0.20:
                        gamepad["right_stick_y_up"] = 1
                        gamepad["right_stick_y_down"] = 0
                    elif val < -0.20:
                        gamepad["right_stick_y_up"] = 0
                        gamepad["right_stick_y_down"] = 1
                    else:
                        gamepad["right_stick_y_up"] = 0
                        gamepad["right_stick_y_down"] = 0
                elif event.code == 'ABS_RX':
                    val = event.state / MAX_JOYSTICK_VALUE # [-1, 1]

                    if val > 0.20:
                        gamepad["right_stick_x_right"] = 1
                        gamepad["right_stick_x_left"] = 0
                    elif val < -0.20:
                        gamepad["right_stick_x_right"] = 0
                        gamepad["right_stick_x_left"] = 1
                    else:
                        gamepad["right_stick_x_right"] = 0
                        gamepad["right_stick_x_left"] = 0
                elif event.code == 'ABS_Z':
                    val = event.state / MAX_TRIGGER_VALUE # [0, 1]
                    gamepad["left_trigger"] = 1 if val > 0.03 else 0
                elif event.code == 'ABS_RZ':
                    val = event.state / MAX_TRIGGER_VALUE # [0, 1]
                    gamepad["right_trigger"] = 1 if val > 0.03 else 0
                elif event.code == 'BTN_TL':
                    gamepad["left_bumper"] = event.state
                elif event.code == 'BTN_TR':
                    gamepad["right_bumper"] = event.state
                elif event.code == 'BTN_SOUTH':
                    gamepad["a"] = event.state
                elif event.code == 'BTN_EAST':
                    gamepad["b"] = event.state
                elif event.code == 'BTN_WEST':
                    gamepad["x"] = event.state
                elif event.code == 'BTN_NORTH':
                    gamepad["y"] = event.state
                elif event.code == 'BTN_THUMBL':
                    gamepad["left_thumb"] = event.state
                elif event.code == 'BTN_THUMBR':
                    gamepad["right_thumb"] = event.state
                elif event.code == 'BTN_SELECT':
                    gamepad["start"] = event.state
                elif event.code == 'BTN_START':
                    gamepad["select"] = event.state
                elif event.code == 'ABS_HAT0Y':
                    if event.state == 1:
                        gamepad["d_pad_up"] = 0
                        gamepad["d_pad_down"] = 1
                    elif event.state == -1:
                        gamepad["d_pad_up"] = 1
                        gamepad["d_pad_down"] = 0
                    else:
                        gamepad["d_pad_up"] = 0
                        gamepad["d_pad_down"] = 0
                elif event.code == 'ABS_HAT0X':
                    if event.state == 1:
                        gamepad["d_pad_left"] = 0
                        gamepad["d_pad_right"] = 1
                    elif event.state == -1:
                        gamepad["d_pad_left"] = 1
                        gamepad["d_pad_right"] = 0
                    else:
                        gamepad["d_pad_left"] = 0
                        gamepad["d_pad_right"] = 0
        except inputs.UnpluggedError:
            time.sleep(5)
