// Local Includes
#include "data_io.h"
#include "game.h"
#include "graphics.h"
#include "worldmath.h"

int main()
{
    LoadMapFile();
    InitGraphics();
    LoadTexture("textures/WallWood1.jpeg");
    // DrawTexture(0);
    GameLoop();
    TearDownGraphics();
    UnloadData();
    return 0;
}
