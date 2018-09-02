#ifndef STRUCTURES_INCLUDED
#define STRUCTURES_INCLUDED

#include <stdlib.h>

// Screen Line
struct screen_line
{
	int sx1, sy1, sx2, sy2;
	float oz1, oz2;
	float uoz1, voz1, uoz2, voz2;
	int clipped;
};

// Poly Line
struct poly_line
{
	float x1, y1, x2, y2;
	float u1, v1, u2, v2;
	int clipped;
};

// Vertex
struct xy
{
	float x, y;
};

// Poly floor/roof
struct poly
{
	struct xy t1, t2, t3, t4;
};

struct line
{
	float x1, y1, x2, y2;
	signed char adjacent;
	signed char wallTexture, ceilingTexture, floorTexture;
};

// Render Portal
struct portal
{
	int sectorNo, x1, x2;
};

// Sectors
struct sector
{
	float floorheight, ceilingheight, lightlevel;
	struct line *lineDef;
	unsigned lineNum;
	struct object *objectDef;
	unsigned objectNum;
	signed char floorTexture, ceilingTexture;
	// Four texture points
	struct xy t1, t2, t3, t4;
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
	int width, height, components;
};

// Object Structure
struct object
{
	unsigned char *pixels;
	int width, height, components, sector;
	float x, y;
};

#endif