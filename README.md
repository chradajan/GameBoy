# Game Boy Color

A Game Boy Color emulator written in C++ with a Python frontend.

## Requirements

- [CMake 3.19+](https://cmake.org/)
- [Python](https://www.python.org/)
- C++ compiler supporting C++17

## Build Steps

```
git clone https://github.com/chradajan/GameBoy.git
cd GameBoy/GameBoy
mkdir build
cd build
cmake ..
make
cd ../../
pip install -r GUI/requirements.txt
python GUI/GUI/main.py "path/to/rom.gbc"
```

(Optional) Create a directory in the root directory called "boot", download
"[cgb_boot.bin](https://gbdev.gg8.se/files/roms/bootroms/)", and place in the boot directory. This is required to show
the Game Boy launch screen.

## Controls

- WASD => D-Pad
- K => B
- L => A
- Enter => Start
- Right Shift => Select
