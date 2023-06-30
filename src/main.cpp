#include <GameBoy.hpp>
#include <Paths.hpp>
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

    GameBoy gb = GameBoy();

    if (argc > 1)
    {
        gb.InsertCartridge(argv[1]);
        gb.Run();
    }

    return 0;
}