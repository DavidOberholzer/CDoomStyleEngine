#include <time.h>
#include <unistd.h>

#include "data_io.h"
#include "game.h"
#include "graphics.h"
#include "worldmath.h"
#include "structures.h"

#define MOVESPEED 0.03
#define FOV 400

static float playerView = 0;
static float viewMovement = 0;
static int viewDown = 1;
static int eyeHeight = 20;
static int playerHeight = 0;
static int fallHeight = 0;
static int fallingVelocity = 0;
int showTextures = 1;

static void RenderObject(struct object *obj, struct sector *sctr, int ph, int s1, int s2, int yt[WIDTH], int yb[WIDTH])
{
	struct texture *texture = &textures[obj->texture - 1];
	float rotx, roty, lightlevel;
	rotx = (obj->x - player.position.x) * cos(player.angle) + (obj->y - player.position.y) * sin(player.angle);
	roty = (obj->y - player.position.y) * cos(player.angle) - (obj->x - player.position.x) * sin(player.angle);
	if (rotx > 0.05)
	{
		float oz;
		int sx, sy, x1, x2, fy, tx, ty;
		sx = (FOV * roty / rotx) + WIDTHD2;
		sy = ((ph - sctr->floorheight) / rotx) + HEIGHTD2;
		oz = 1 / rotx;
		lightlevel = ((oz) > 1 ? 1 : oz) * sctr->lightlevel;
		tx = texture->width * oz / 2;
		ty = texture->height * oz;
		fy = sy - ty;
		x1 = sx - tx;
		x2 = sx + tx;
		for (int y = fy; y <= sy; y++)
		{
			float t1 = (y - fy) / (float)(sy - fy);
			int v = 0 * (1 - t1) + (texture->height - 1) * t1;
			for (int x = x1; x <= x2; x++)
			{
				if (x > s1 && x < s2 && y > yt[x] && y < yb[x])
				{
					float t2 = (x - x1) / (float)(x2 - x1);
					int u = 0 * (1 - t2) + texture->width * t2;
					int index = (u + v * (texture->width));
					if (texture->pixels[index].R != 0xff && texture->pixels[index].G != 0x00 && texture->pixels[index].B != 0xaf)
					{
						SDL_SetRenderDrawColor(renderer, texture->pixels[index].R * lightlevel, texture->pixels[index].G * lightlevel, texture->pixels[index].B * lightlevel, 0x00);
						SDL_RenderDrawPoint(renderer, x, y);
					}
				}
			}
		}
	}
}

static void RenderFlatLoop(int startLine,
						   struct screen_line sLines[4], struct texture *texture, struct portal *currentPortal,
						   float sctrLightLevel, int yStart, int yEnd, int yt[WIDTH], int yb[WIDTH], int reverse)
{
	int done = 0, lineAfter, lineBefore;

	lineAfter = (startLine + 1) > 3 ? 0 : startLine + 1;
	if (abs(sLines[lineAfter].sy1 - sLines[lineAfter].sy2) == 0)
	{
		lineAfter = (lineAfter + 1) > 3 ? 0 : lineAfter + 1;
	}
	lineBefore = (startLine - 1) < 0 ? 3 : startLine - 1;
	int count = 0;
	do
	{
		struct screen_line currentLine = sLines[startLine],
						   oppositeLine = sLines[lineAfter];
		int startY, endY, xStart, xEnd, oStartY, oEndY, oStartX, oEndX, loaded = 0;
		float startUoz, endUoz, startVoz, endVoz, startOZ, endOZ,
			oStartUoz, oEndUoz, oStartVoz, oEndVoz;
		startY = currentLine.sy2;
		endY = currentLine.sy1;
		xStart = currentLine.sx2;
		xEnd = currentLine.sx1;
		startUoz = currentLine.uoz2;
		endUoz = currentLine.uoz1;
		startVoz = currentLine.voz2;
		endVoz = currentLine.voz1;
		startOZ = currentLine.oz2;
		endOZ = currentLine.oz1;
		if (!loaded)
		{
			oStartY = oppositeLine.sy1;
			oEndY = oppositeLine.sy2;
			oStartX = oppositeLine.sx1;
			oEndX = oppositeLine.sx2;
			oStartUoz = oppositeLine.uoz1;
			oEndUoz = oppositeLine.uoz2;
			oStartVoz = oppositeLine.voz1;
			oEndVoz = oppositeLine.voz2;
			loaded = 1;
		}

		if (abs(endY - startY) < 15000)
			for (int y = startY; reverse ? y >= endY : y <= endY; reverse ? y-- : y++)
			{
				if (y >= 0 && y < HEIGHT && y >= yStart && y <= yEnd)
				{
					float ts1 = (y - startY) / (float)(endY - startY),
						  ts2 = (y - oStartY) / (float)(oEndY - oStartY);
					int x1 = xStart * (1 - ts1) + xEnd * ts1,
						x2 = oStartX * (1 - ts2) + oEndX * ts2;
					float oz = startOZ * (1 - ts1) + endOZ * ts1,
						  uoz1 = startUoz * (1 - ts1) + endUoz * ts1,
						  uoz2 = oStartUoz * (1 - ts2) + oEndUoz * ts2,
						  voz1 = startVoz * (1 - ts1) + endVoz * ts1,
						  voz2 = oStartVoz * (1 - ts2) + oEndVoz * ts2;
					if (!(x2 > 8000 || x1 > 8000 || x2 < -8000 || x1 < -8000 || abs(x2 - x1) > 16000))
					{
						int xDir = x1 > x2;
						int startX = xDir ? x2 : x1,
							endX = xDir ? x1 : x2;
						float startU = xDir ? uoz2 : uoz1,
							  endU = xDir ? uoz1 : uoz2,
							  startV = xDir ? voz2 : voz1,
							  endV = xDir ? voz1 : voz2;
						for (int x = startX; x <= endX; x++)
						{
							if (x >= 0 && x < WIDTH && x > currentPortal->x1 && x < currentPortal->x2)
							{
								if (y > yt[x] && y < yb[x])
								{
									float t = (x - startX) / (float)(endX - startX);
									int u = Clamp(texture->width - 1, 0, ((startU * (1 - t) + endU * t) / oz));
									int v = Clamp(texture->height - 1, 0, ((startV * (1 - t) + endV * t) / oz));
									float lightlevel = ((oz) > 1 ? 1 : oz) * sctrLightLevel;
									int index = (u + v * texture->width);
									SDL_SetRenderDrawColor(renderer, texture->pixels[index].R * lightlevel, texture->pixels[index].G * lightlevel, texture->pixels[index].B * lightlevel, 0x00);
									SDL_RenderDrawPoint(renderer, x, y);
								}
							}
						}
					}

					if (ts2 == 1 && y != yEnd)
					{
						lineAfter = (lineAfter + 1) > 3 ? 0 : lineAfter + 1;
						oppositeLine = sLines[lineAfter];
						oStartY = oppositeLine.sy1;
						oEndY = oppositeLine.sy2;
						oStartX = oppositeLine.sx1;
						oEndX = oppositeLine.sx2;
						oStartUoz = oppositeLine.uoz1;
						oEndUoz = oppositeLine.uoz2;
						oStartVoz = oppositeLine.voz1;
						oEndVoz = oppositeLine.voz2;
					}
				}
				if (reverse ? (y == yStart) : (y == yEnd))
				{
					done = 1;
				}
			}
		count++;
		if (!done)
			startLine = lineBefore;
	} while (!done && count != 2);
}

static void RenderFlat(struct xy t1, struct xy t2, struct xy t3, struct xy t4, int ph, struct sector *sctr, struct portal *currentPortal, int yt[WIDTH], int yb[WIDTH])
{
	struct texture *fTexture = &textures[sctr->floorTexture - 1],
				   *cTexture = &textures[sctr->ceilingTexture - 1];
	struct xy points[4];
	float angle = player.angle;
	points[0].x = (t1.x - player.position.x) * cos(angle) + (t1.y - player.position.y) * sin(angle);
	points[0].y = (t1.y - player.position.y) * cos(angle) - (t1.x - player.position.x) * sin(angle);
	points[1].x = (t2.x - player.position.x) * cos(angle) + (t2.y - player.position.y) * sin(angle);
	points[1].y = (t2.y - player.position.y) * cos(angle) - (t2.x - player.position.x) * sin(angle);
	points[2].x = (t3.x - player.position.x) * cos(angle) + (t3.y - player.position.y) * sin(angle);
	points[2].y = (t3.y - player.position.y) * cos(angle) - (t3.x - player.position.x) * sin(angle);
	points[3].x = (t4.x - player.position.x) * cos(angle) + (t4.y - player.position.y) * sin(angle);
	points[3].y = (t4.y - player.position.y) * cos(angle) - (t4.x - player.position.x) * sin(angle);
	struct poly_line lines[4];
	struct xy uvs[4];
	uvs[0] = (struct xy){0, 0};
	uvs[1] = (struct xy){fTexture->width - 1, 0};
	uvs[2] = (struct xy){fTexture->width - 1, fTexture->height - 1};
	uvs[3] = (struct xy){0, fTexture->height - 1};
	for (int i = 0; i < 4; i++)
	{
		int i2 = (i + 1) < 4 ? i + 1 : 0;
		lines[i] = GetPolyLine(
			points[i].x, points[i].y, points[i2].x, points[i2].y,
			uvs[i].x, uvs[i].y, uvs[i2].x, uvs[i2].y);
	}
	int yStartB = HEIGHT - 1, yEndB = 0,
		yStartT = HEIGHT - 1, yEndT = 0,
		startLineB = -1, startLineT = -1;
	struct screen_line bLines[4], tLines[4];
	for (int i = 0; i < 4; i++)
	{
		if (!lines[i].clipped)
		{
			float oz1, oz2;
			int ust1, ust2, zb1, zb2, zt1, zt2, ylessB, ymoreB, ylessT, ymoreT;
			oz1 = 1 / lines[i].x1;
			oz2 = 1 / lines[i].x2;
			ust1 = (FOV * lines[i].y1 * oz1) + WIDTHD2;
			ust2 = (FOV * lines[i].y2 * oz2) + WIDTHD2;
			zb1 = ((ph - sctr->floorheight) * oz1) + HEIGHTD2;
			zb2 = ((ph - sctr->floorheight) * oz2) + HEIGHTD2;
			zt1 = (-(sctr->ceilingheight - ph) * oz1) + HEIGHTD2;
			zt2 = (-(sctr->ceilingheight - ph) * oz2) + HEIGHTD2;
			bLines[i] = (struct screen_line){
				ust1, zb1, ust2, zb2, oz1, oz2,
				lines[i].u1 * oz1, lines[i].v1 * oz1, lines[i].u2 * oz2, lines[i].v2 * oz2};
			tLines[i] = (struct screen_line){
				ust1, zt1, ust2, zt2, oz1, oz2,
				lines[i].u1 * oz1, lines[i].v1 * oz1, lines[i].u2 * oz2, lines[i].v2 * oz2};
			ylessB = zb2 < yStartB;
			ymoreB = zb2 > yEndB;

			if (ylessB && abs(zb1 - zb2) > 0)
			{
				yStartB = zb2;
				startLineB = i;
			}
			if (ymoreB && abs(zb1 - zb2) > 0)
			{
				yEndB = zb2;
			}

			ylessT = zt2 < yStartT;
			ymoreT = zt2 > yEndT;

			if (ylessT && abs(zt1 - zt2) > 0)
			{
				yStartT = zt2;
			}
			if (ymoreT && abs(zt1 - zt2) > 0)
			{
				yEndT = zt2;
				startLineT = i;
			}
		}
		else
		{
			bLines[i] = (struct screen_line){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
			tLines[i] = (struct screen_line){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
		}
	}
	RenderFlatLoop(startLineB, bLines, fTexture, currentPortal, sctr->lightlevel, yStartB, yEndB, yt, yb, 0);
	RenderFlatLoop(startLineT, tLines, cTexture, currentPortal, sctr->lightlevel, yStartT, yEndT, yt, yb, 1);
}

static void SegmentAndDrawFlats(struct sector *sctr, int ph, struct portal *currentPortal, int yTopLimit[WIDTH], int yBottomLimit[WIDTH])
{
	float segmentSize = 1;
	float xDiff = sctr->t3.x - sctr->t1.x,
		  yDiff = sctr->t3.y - sctr->t1.y;
	int xSegments = 1, ySegments = 1;
	if ((xDiff / segmentSize) > 1)
	{
		xSegments = (int)ceil(xDiff / segmentSize);
	}
	if ((yDiff / segmentSize) > 1)
	{
		ySegments = (int)ceil(yDiff / segmentSize);
	}
	struct poly *blocks = NULL;
	int blockCount = 0;
	for (int i = 0; i < xSegments; i++)
	{
		for (int j = 0; j < ySegments; j++)
		{
			blocks = realloc(blocks, ++blockCount * sizeof(*blocks));
			blocks[blockCount - 1].t1 = (struct xy){sctr->t1.x + (i * segmentSize), sctr->t1.y + (j * segmentSize)};
			blocks[blockCount - 1].t2 = (struct xy){sctr->t1.x + ((i + 1) * segmentSize), sctr->t1.y + (j * segmentSize)};
			blocks[blockCount - 1].t3 = (struct xy){sctr->t1.x + ((i + 1) * segmentSize), sctr->t1.y + ((j + 1) * segmentSize)};
			blocks[blockCount - 1].t4 = (struct xy){sctr->t1.x + (i * segmentSize), sctr->t1.y + ((j + 1) * segmentSize)};
		}
	}
	for (int i = 0; i < blockCount; i++)
	{
		RenderFlat(blocks[i].t1, blocks[i].t2, blocks[i].t3, blocks[i].t4, ph, sctr, currentPortal, yTopLimit, yBottomLimit);
	}
	free(blocks);
}

static void RenderWalls()
{
	int yTopLimit[WIDTH], yBottomLimit[WIDTH], yTopSLimit[WIDTH], yBottomSLimit[WIDTH];
	for (int i = 0; i < WIDTH; i++)
	{
		yTopLimit[i] = 0;
		yBottomLimit[i] = HEIGHT - 101;
		yTopSLimit[i] = 0;
		yBottomSLimit[i] = HEIGHT - 101;
	}
	struct portal queue[MAX_SECTORS];
	struct room rooms[MAX_SECTORS];
	int roomNum = 0;
	for (int i = 0; i < MAX_SECTORS; i++)
	{
		rooms[i].objectNum = 0;
	}

	// Whole screen is the initial portal.
	*queue = (struct portal){player.sector, 0, WIDTH - 1};
	int end = 0, current = -1;

	// Get current player height
	float ph = (int)playerView + playerHeight + eyeHeight;
	do
	{
		current += 1;
		struct portal *currentPortal = queue + current;
		struct sector *sctr = &sectors[currentPortal->sectorNo - 1];
		// Draw all roofs and floors if texture are there.
		if ((sctr->floorTexture > 0 || sctr->ceilingTexture > 0) && showTextures)
		{
			SegmentAndDrawFlats(sctr, ph, currentPortal, yTopLimit, yBottomLimit);
		}
		for (int i = 0; i < sctr->lineNum; i++)
		{
			// First Vector rotation of the lines around the player.
			float rotx1, roty1, rotx2, roty2;
			rotx1 = (sctr->lineDef[i].x1 - player.position.x) * cos(player.angle) + (sctr->lineDef[i].y1 - player.position.y) * sin(player.angle);
			roty1 = (sctr->lineDef[i].y1 - player.position.y) * cos(player.angle) - (sctr->lineDef[i].x1 - player.position.x) * sin(player.angle);
			rotx2 = (sctr->lineDef[i].x2 - player.position.x) * cos(player.angle) + (sctr->lineDef[i].y2 - player.position.y) * sin(player.angle);
			roty2 = (sctr->lineDef[i].y2 - player.position.y) * cos(player.angle) - (sctr->lineDef[i].x2 - player.position.x) * sin(player.angle);

			// Clip line to the view cone.
			float *ptr;
			ptr = ClipViewCone(rotx1, roty1, rotx2, roty2, VIEWANGLE);

			if (*ptr > 0 && *(ptr + 2) > 0)
			{
				int s1, s2, us1, us2, tex1, texw2, texc2, texf2, zt1, zb1, zt2, zb2, stepy1, stepy2, ceily1, ceily2;
				float z1, z2;
				// Wall X transformation
				s1 = (FOV * *(ptr + 1) / *ptr) + WIDTHD2;
				s2 = (FOV * *(ptr + 3) / *(ptr + 2)) + WIDTHD2;
				us1 = (FOV * roty1 / rotx1) + WIDTHD2;
				us2 = (FOV * roty2 / rotx2) + WIDTHD2;
				z1 = 1 / rotx1;
				z2 = 1 / rotx2;
				tex1 = 0;
				texw2 = sctr->lineDef[i].wallTexture > -1 ? (textures[sctr->lineDef[i].wallTexture - 1].width - 1) * z2 : 0;
				texc2 = sctr->lineDef[i].ceilingTexture > -1 ? (textures[sctr->lineDef[i].ceilingTexture - 1].width - 1) * z2 : 0;
				texf2 = sctr->lineDef[i].floorTexture > -1 ? (textures[sctr->lineDef[i].floorTexture - 1].width - 1) * z2 : 0;

				zt1 = (-(sctr->ceilingheight - ph) / *ptr) + HEIGHTD2;
				zb1 = ((ph - sctr->floorheight) / *ptr) + HEIGHTD2;
				zt2 = (-(sctr->ceilingheight - ph) / *(ptr + 2)) + HEIGHTD2;
				zb2 = ((ph - sctr->floorheight) / *(ptr + 2)) + HEIGHTD2;
				if (sctr->lineDef[i].adjacent > -1)
				{
					stepy1 = (ph - sectors[sctr->lineDef[i].adjacent - 1].floorheight) / *ptr + HEIGHTD2;
					stepy2 = (ph - sectors[sctr->lineDef[i].adjacent - 1].floorheight) / *(ptr + 2) + HEIGHTD2;
					ceily1 = -(sectors[sctr->lineDef[i].adjacent - 1].ceilingheight - ph) / *ptr + HEIGHTD2;
					ceily2 = -(sectors[sctr->lineDef[i].adjacent - 1].ceilingheight - ph) / *(ptr + 2) + HEIGHTD2;
				}

				// Wall is facing the player only.
				if (s1 < s2 && (!(s1 < 0 && s2 < 0) || !(s1 > WIDTH && s2 > WIDTH)))
				{
					int x1 = Max(currentPortal->x1, s1);
					int x2 = Min(currentPortal->x2, s2);
					for (int x = x1; x <= x2; x++)
					{
						float t = (x - s1) / (float)(s2 - s1);
						float t2 = (x - us1) / (float)(us2 - us1);
						float z = z1 * (1 - t2) + z2 * t2;
						int yTop = zt1 * (1 - t) + zt2 * t + 0.5;	// +0.5 to remove jaggies.
						int yBottom = zb1 * (1 - t) + zb2 * t + 0.5; // +0.5 to remove jaggies.
						int yt = Clamp(yBottomLimit[x], yTopLimit[x], yTop);
						int yb = Clamp(yBottomLimit[x], yTopLimit[x], yBottom);
						float dx = *ptr * (1 - t) + *(ptr + 2) * t;
						float dy = *(ptr + 1) * (1 - t) + *(ptr + 3) * t;
						float distance = dx * dx + dy * dy;
						// Draw roofs and floors.
						if (sctr->ceilingTexture == -1 || !showTextures)
						{
							RenderLine(x, yTopLimit[x], yt, yTop, yBottom, 0x78, 0x78, 0x78, distance, sctr->lightlevel, -1, 1, 0, -1, current);
						}
						if (sctr->floorTexture == -1 || !showTextures)
						{
							RenderLine(x, yb, yBottomLimit[x], yTop, yBottom, 0x79, 0x79, 0x79, distance, sctr->lightlevel, -1, 0, 1, -1, current);
						}
						if (sctr->lineDef[i].adjacent > -1)
						{
							if (sectors[sctr->lineDef[i].adjacent - 1].floorheight > sctr->floorheight)
							{
								// Create a floor wall for the change in height.
								float tex = tex1 * (1 - t2) + texf2 * t2;
								int u = tex / z;
								int realStepY = stepy1 * (1 - t) + stepy2 * t + 0.5;
								int stepY = Clamp(yBottomLimit[x], yTopLimit[x], realStepY); // +0.5 to remove jaggies.
								RenderLine(x, stepY, yb, realStepY, yBottom, 0x37, 0xcd, 0xc1, distance, sctr->lightlevel, u, 0, 0, sctr->lineDef[i].floorTexture, current);
								yBottomLimit[x] = stepY;
							}
							else
							{
								yBottomLimit[x] = yb;
							}

							if (sectors[sctr->lineDef[i].adjacent - 1].ceilingheight < sctr->ceilingheight)
							{
								// Create a ceiling for the change in height.
								float tex = tex1 * (1 - t2) + texc2 * t2;
								int u = tex / z;
								int realCeilY = ceily1 * (1 - t) + ceily2 * t + 0.5;
								int ceilY = Clamp(yBottomLimit[x], yTopLimit[x], realCeilY); // +0.5 to remove jaggies.
								RenderLine(x, yt, ceilY, yTop, realCeilY, 0xa7, 0x37, 0xcd, distance, sctr->lightlevel, u, 0, 0, sctr->lineDef[i].ceilingTexture, current);
								yTopLimit[x] = ceilY;
							}
							else
							{
								yTopLimit[x] = yt;
							}
						}
						else
						{
							// Draw a normal wall.
							float tex = tex1 * (1 - t2) + texw2 * t2;
							int u = tex / z;
							RenderLine(x, yt, yb, yTop, yBottom, 0xcc, 0xc5, 0xce, distance, sctr->lightlevel, u, 0, 0, sctr->lineDef[i].wallTexture, current);
						}
					}
					// Load in next in next portal if adjacent sector found.
					if (sctr->lineDef[i].adjacent > -1 && end != MAX_SECTORS - 1)
					{
						end++;
						queue[end] = (struct portal){sctr->lineDef[i].adjacent, x1, x2};
						// Load rooms with objects to draw
						struct sector *aSctr = &sectors[sctr->lineDef[i].adjacent - 1];
						if (aSctr->objectNum > 0)
						{
							rooms[roomNum].x1 = x1;
							rooms[roomNum].x2 = x2;
							rooms[roomNum].sctr = aSctr;
							for (int k = 0; k < WIDTH; k++)
							{
								rooms[roomNum].yt[k] = yTopLimit[k];
								rooms[roomNum].yb[k] = yBottomLimit[k];
							}
							rooms[roomNum].objectNum = aSctr->objectNum > MAX_SECTORS ? MAX_SECTORS : aSctr->objectNum;
							for (int o = 0; o < aSctr->objectNum; o++)
							{
								rooms[roomNum].objects[o].x = aSctr->objectDef[o].x;
								rooms[roomNum].objects[o].y = aSctr->objectDef[o].y;
								rooms[roomNum].objects[o].texture = aSctr->objectDef[o].texture;
							}
							roomNum++;
						}
					}
				}
			}
		}
	} while (current != end);
	// Render objects in portals found BACKWARDS (for drawing over one another)
	for (int i = roomNum - 1; i >= 0; i--)
	{
		struct room *room = &rooms[i];
		if (room->objectNum > 0)
		{
			struct objInfo objs[room->objectNum], temp;
			for (int p = 0; p < room->objectNum; p++)
			{
				objs[p].d = lineDistance(room->objects[p].x - player.position.x,
										 room->objects[p].y - player.position.y);
				objs[p].loc = p;
			}
			for (int i = 0; i < room->objectNum; i++)
			{
				for (int j = i + 1; j < room->objectNum; j++)
				{
					if (objs[i].d < objs[j].d)
					{
						temp = objs[i];
						objs[i] = objs[j];
						objs[j] = temp;
					}
				}
			}
			for (int j = 0; j < room->objectNum; j++)
			{
				struct object *obj = &room->objects[objs[j].loc];
				RenderObject(obj, room->sctr, ph, room->x1, room->x2, room->yt, room->yb);
			}
		}
	}
	// Render objects in current room
	struct sector *sctr = &sectors[player.sector - 1];
	struct objInfo objs[sctr->objectNum], temp;
	for (int p = 0; p < sctr->objectNum; p++)
	{
		objs[p].d = lineDistance(sctr->objectDef[p].x - player.position.x,
								 sctr->objectDef[p].y - player.position.y);
		objs[p].loc = p;
	}
	for (int i = 0; i < sctr->objectNum; i++)
	{
		for (int j = i + 1; j < sctr->objectNum; j++)
		{
			if (objs[i].d < objs[j].d)
			{
				temp = objs[i];
				objs[i] = objs[j];
				objs[j] = temp;
			}
		}
	}
	for (int i = 0; i < sctr->objectNum; i++)
	{
		struct object *obj = &sctr->objectDef[objs[i].loc];
		RenderObject(obj, sctr, ph, 0, WIDTH - 1, yTopSLimit, yBottomSLimit);
	}
}

static void MovePlayer()
{
	int sec = player.sector - 1;
	for (int i = 0; i < sectors[sec].lineNum; i++)
	{
		float crossing = SideOfLine(player.position.x + player.velocity.x, player.position.y + player.velocity.y, sectors[sec].lineDef[i].x1, sectors[sec].lineDef[i].y1, sectors[sec].lineDef[i].x2, sectors[sec].lineDef[i].y2);
		int overlap = BoxesOverlap(player.position.x, player.position.y, player.position.x + player.velocity.x, player.position.y + player.velocity.y, sectors[sec].lineDef[i].x1, sectors[sec].lineDef[i].y1, sectors[sec].lineDef[i].x2, sectors[sec].lineDef[i].y2);
		if (sectors[sec].lineDef[i].adjacent > -1 && crossing < 0 && overlap)
		{
			int nextFloorHeight = sectors[sectors[sec].lineDef[i].adjacent - 1].floorheight;
			if (sectors[sec].floorheight < nextFloorHeight)
			{
				playerHeight = nextFloorHeight;
			}
			fallHeight = nextFloorHeight;
			player.sector = sectors[sec].lineDef[i].adjacent;
		}
	}

	player.position.x += player.velocity.x;
	player.position.y += player.velocity.y;
	player.velocity.x = 0;
	player.velocity.y = 0;
}

static void HandleCollision()
{
	for (int i = 0; i < numSectors; i++)
	{
		for (int j = 0; j < sectors[i].lineNum; j++)
		{
			int canStep = 0;
			if (sectors[i].lineDef[j].adjacent > -1)
			{
				struct sector *aSctr = &sectors[sectors[i].lineDef[j].adjacent - 1];
				if ((aSctr->floorheight - sectors[player.sector - 1].floorheight) <= 30 && (aSctr->ceilingheight - aSctr->floorheight) > 50)
				{
					canStep = 1;
				}
			}
			if (!canStep)
			{
				float movingX = player.velocity.x > 0 ? player.velocity.x + 0.1 : player.velocity.x - 0.1;
				float movingY = player.velocity.y > 0 ? player.velocity.y + 0.1 : player.velocity.y - 0.1;
				float crossing = SideOfLine(player.position.x + movingX, player.position.y + movingY, sectors[i].lineDef[j].x1, sectors[i].lineDef[j].y1, sectors[i].lineDef[j].x2, sectors[i].lineDef[j].y2);
				int overlap = BoxesOverlap(player.position.x, player.position.y, player.position.x + movingX, player.position.y + movingY, sectors[i].lineDef[j].x1, sectors[i].lineDef[j].y1, sectors[i].lineDef[j].x2, sectors[i].lineDef[j].y2);
				// Check if the player is going to move behind the given wall and within range of the wall.
				if (crossing < 0 && overlap)
				{
					// Project the movement vector along the collided wall.
					float *dir = VectorProjection(player.velocity.x, player.velocity.y, sectors[i].lineDef[j].x2 - sectors[i].lineDef[j].x1, sectors[i].lineDef[j].y2 - sectors[i].lineDef[j].y1);
					if ((player.velocity.x > 0 && *dir < 0) ||
						(player.velocity.x < 0 && *dir > 0) ||
						(player.velocity.y < 0 && *(dir + 1) > 0) ||
						(player.velocity.y < 0 && *(dir + 1) > 0))
					{
						player.velocity.x = 0;
						player.velocity.y = 0;
					}
					else
					{
						player.velocity.x = *dir;
						player.velocity.y = *(dir + 1);
					}
				}
			}
		}
	}
	MovePlayer();
}

static void HeadBob()
{
	if (playerView >= 40 || playerView <= 20)
	{
		if (playerView >= 40)
		{
			viewDown = 1;
		}
		else
		{
			viewDown = 0;
		}
		viewMovement = 0;
	}
	viewMovement = viewDown == 1 ? viewMovement - 0.2 : viewMovement + 0.2;
	playerView += viewMovement;
}

static void HandleFalling()
{
	if (playerHeight > fallHeight)
	{
		fallingVelocity += 4;
		playerHeight -= fallingVelocity;
		if (playerHeight < fallHeight)
		{
			fallingVelocity = 0;
		}
	}

	if (playerHeight < fallHeight)
	{
		playerHeight += ++fallingVelocity;
		if (playerHeight > fallHeight)
		{
			fallingVelocity = 0;
			playerHeight = fallHeight;
		}
	}
}

void GameLoop()
{
	int keysPressed[6] = {0, 0, 0, 0, 0, 0}; //Init key states to not pressed.
	int close = 0;
	time_t start, end;
	while (!close)
	{
		start = clock();
		ClearFrame();
		RenderWalls();
		PresentFrame();
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
				close = 1;
				break;
			case SDL_KEYUP:
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
				case SDLK_w:
					keysPressed[0] = event.type == SDL_KEYDOWN ? 1 : 0;
					break;
				case SDLK_s:
					keysPressed[1] = event.type == SDL_KEYDOWN ? 1 : 0;
					break;
				case SDLK_a:
					keysPressed[2] = event.type == SDL_KEYDOWN ? 1 : 0;
					break;
				case SDLK_d:
					keysPressed[3] = event.type == SDL_KEYDOWN ? 1 : 0;
					break;
				case SDLK_q:
					keysPressed[4] = event.type == SDL_KEYDOWN ? 1 : 0;
					break;
				case SDLK_e:
					keysPressed[5] = event.type == SDL_KEYDOWN ? 1 : 0;
					break;
				case SDLK_t:
					if (event.type == SDL_KEYDOWN)
					{
						showTextures = showTextures ? 0 : 1;
					}
					break;
				case SDLK_ESCAPE:
					close = 1;
					break;
				}
			}
		}
		player.velocity.x += keysPressed[0] ? MOVESPEED * cos(player.angle) : 0;
		player.velocity.y += keysPressed[0] ? MOVESPEED * sin(player.angle) : 0;
		player.velocity.x -= keysPressed[1] ? MOVESPEED * cos(player.angle) : 0;
		player.velocity.y -= keysPressed[1] ? MOVESPEED * sin(player.angle) : 0;
		player.velocity.x += keysPressed[2] ? MOVESPEED * sin(player.angle) : 0;
		player.velocity.y -= keysPressed[2] ? MOVESPEED * cos(player.angle) : 0;
		player.velocity.x -= keysPressed[3] ? MOVESPEED * sin(player.angle) : 0;
		player.velocity.y += keysPressed[3] ? MOVESPEED * cos(player.angle) : 0;

		if (player.velocity.x || player.velocity.y)
		{
			HandleCollision();
			HeadBob();
		}
		else
		{
			viewMovement = 0;
			playerView = 40;
		}

		HandleFalling();

		player.angle -= keysPressed[4] ? 0.08f : 0;
		player.angle += keysPressed[5] ? 0.08f : 0;

		end = clock();
		double diff = difftime(end, start);
		if (diff < 16667)
		{
			int wait = 16667 - diff;
			usleep(wait);
		}
	}
}