import ctypes
import sys

GAME_BOY = None

if sys.platform == "darwin":
    GAME_BOY = ctypes.CDLL("./GameBoy/lib/libGameBoy.dylib", winmode=0)
elif sys.platform == "win32":
    GAME_BOY = ctypes.CDLL("./GameBoy/lib/libGameBoy.dll", winmode=0)
