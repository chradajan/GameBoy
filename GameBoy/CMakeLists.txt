cmake_minimum_required(VERSION 3.19)

project (GameBoy
    LANGUAGES CXX
    VERSION 1.0
    DESCRIPTION "GameBoy library"
)

set(SOURCES
    src/APU.cpp
    src/Cartridge/MBC0.cpp
    src/Cartridge/MBC1.cpp
    src/Cartridge/MBC5.cpp
    src/CGB.cpp
    src/Channel2.cpp
    src/CPU.cpp
    src/CPU_Instructions.cpp
    src/GameBoy.cpp
    src/GameBoy_Clocks.cpp
    src/GameBoy_Memory.cpp
    src/PixelFIFO.cpp
    src/PPU.cpp
)

add_library(GameBoy SHARED ${SOURCES})

set_target_properties(GameBoy PROPERTIES
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER ${PROJECT_SOURCE_DIR}/include/CGB.hpp
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${PROJECT_SOURCE_DIR}/src/include
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)
