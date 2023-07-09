#define SDL_MAIN_HANDLED
#include <GameBoy.hpp>
#include <GameWindow.hpp>
#include <Paths.hpp>
#include <array>
#include <filesystem>

int main(int argc, char** argv)
{
    if (!std::filesystem::is_directory(LOG_PATH))
    {
        std::filesystem::create_directory(LOG_PATH);
    }

    if (!std::filesystem::is_directory(SAVE_PATH))
    {
        std::filesystem::create_directory(SAVE_PATH);
    }

    std::array<uint8_t, SCREEN_WIDTH * SCREEN_HEIGHT * CHANNELS> frameBuffer;
    frameBuffer.fill(0x00);

    auto gameBoy = GameBoy(frameBuffer.data());
    auto gameWindow = GameWindow(&gameBoy, frameBuffer.data());

    if (argc > 1)
    {
        gameBoy.InsertCartridge(argv[1]);
    }

    gameWindow.Run();

    return 0;
}