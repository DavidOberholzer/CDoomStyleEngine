#ifndef STRUCTURES_INCLUDED
#define STRUCTURES_INCLUDED

#include <stdlib.h>
#include "graphics.h"

typedef unsigned char BYTE;
typedef short DBYTE;

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

// Player location
struct player
{
	struct xy position, velocity;
	float angle;
	unsigned sector;
};

// Pixel
struct pixel
{
	unsigned char R, G, B;
};

// Texture Structure
struct texture
{
	struct pixel *pixels;
	int width, height;
};

// Object Structure
struct object
{
	int sector, texture;
	float x, y;
};

// Object Info for sorting.
struct objInfo 
{
	float d;
	int loc;
};

// Render Portal
struct portal
{
	int sectorNo, x1, x2;
};

// Room to render objects etc.
struct room
{
	int x1, x2;
	struct object objects[MAX_SECTORS];
	int objectNum;
	int yt[WIDTH], yb[WIDTH];
	struct sector *sctr;
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

#pragma pack(push, 1)

// PCX Header Structure
struct pcx_header
{
	BYTE manufacturer;
	BYTE version;
	BYTE runLength;
	BYTE bpppbp;
	DBYTE Xmin;
	DBYTE Ymin;
	DBYTE Xmax;
	DBYTE YMax;
	DBYTE HorizontalRes;
	DBYTE VerticalRes;
	BYTE pallete[48];
	BYTE reservedB1;
	BYTE noBitPlanes;
	DBYTE bytesPerRow;
	DBYTE palleteInterpretation;
	DBYTE horizontalScreenSize;
	DBYTE verticalScreenSize;
	BYTE reserved[54];
};

#pragma pack(pop)

#endif