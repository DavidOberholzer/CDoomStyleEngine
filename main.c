// Local Includes
#include "data_io.h"
#include "game.h"
#include "graphics.h"
#include "worldmath.h"

int main()
{
    LoadMapFile("testmap1.txt");
    InitGraphics();
    GameLoop();
    TearDownGraphics();
    UnloadData();
    return 0;
}
