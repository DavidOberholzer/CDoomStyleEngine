#ifndef DATA_IO_INCLUDED
#define DATA_IO_INCLUDED

#include <stdlib.h>

#include "structures.h"

extern struct texture *textures;
extern int numTextures;
extern struct sector *sectors;
extern int numSectors;
extern struct player player;

void LoadMapFile(char *);
void LoadTexture(char *);
void UnloadData();

#endif