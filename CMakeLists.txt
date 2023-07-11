cmake_minimum_required(VERSION 3.17)

project (GameBoy
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -O2")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

find_package(SDL2 REQUIRED)

set(SOURCES
    src/Cartridge/MBC0.cpp
    src/Cartridge/MBC1.cpp
    src/Controller.cpp
    src/CPU.cpp
    src/CPU_Instructions.cpp
    src/GameBoy.cpp
    src/GameWindow.cpp
    src/PixelFIFO.cpp
    src/PPU.cpp
    src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/library/SDL2/include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        SDL2::SDL2
)
