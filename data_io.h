#ifndef DATA_IO_INCLUDED
#define DATA_IO_INCLUDED

#include <stdlib.h>

#include "structures.h"

extern struct sector *sectors;
extern int numSectors;
extern struct player player;

void LoadMapFile();
void UnloadData();

#endif