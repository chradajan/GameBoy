cmake_minimum_required(VERSION 3.17)

project (GameBoy
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -DPRINT_SERIAL")

set(SOURCES
    src/Cartridge/MBC0.cpp
    src/Cartridge/MBC1.cpp
    src/CPU.cpp
    src/CPU_Instructions.cpp
    src/GameBoy.cpp
    src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)
