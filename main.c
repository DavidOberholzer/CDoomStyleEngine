// Local Includes
#include "data_io.h"
#include "game.h"
#include "graphics.h"
#include "worldmath.h"
#include "structures.h"

int main()
{
    LoadMapFile("theoverlook.txt");
    InitGraphics();
    GameLoop();
    TearDownGraphics();
    UnloadData();
    return 0;
}
