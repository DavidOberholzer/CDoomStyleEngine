#ifndef STRUCTURES_INCLUDED
#define STRUCTURES_INCLUDED

#include <stdlib.h>

// Vertex
struct xy
{
	float x, y;
};

struct line
{
	float x1, y1, x2, y2;
	signed char adjacent;
	signed char wallTexture, ceilingTexture, floorTexture;
};

// Sectors
struct sector
{
	float floorheight, ceilingheight, lightlevel;
	struct line *lineDef;
	unsigned lineNum;
};

// Player location
struct player
{
	struct xy position, velocity;
	float angle;
	unsigned sector;
};

// Texture Structure
struct texture
{
	unsigned char *pixels;
	int width;
	int height;
	int components;
};

#endif