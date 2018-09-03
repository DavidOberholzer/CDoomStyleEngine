#include <time.h>
#include <unistd.h>

#include "data_io.h"
#include "game.h"
#include "graphics.h"
#include "worldmath.h"

#define MOVESPEED 0.05
#define FOV 300

static float playerView = 400;
static float viewMovement = 0;
static int viewDown = 1;
static int playerHeight = 0;
static int fallHeight = 0;
static int fallingVelocity = 0;
int showTextures = 1;

static void RenderFlat(struct xy t1, struct xy t2, struct xy t3, struct xy t4, int tex, float viewHeight, float sctrLightLevel, struct portal *currentPortal, int yt[WIDTH], int yb[WIDTH], int reverse)
{
	struct texture *texture = &textures[tex - 1];
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
	uvs[1] = (struct xy){texture->width - 1, 0};
	uvs[2] = (struct xy){texture->width - 1, texture->height - 1};
	uvs[3] = (struct xy){0, texture->height - 1};
	for (int i = 0; i < 4; i++)
	{
		int i2 = (i + 1) < 4 ? i + 1 : 0;
		lines[i] = GetPolyLine(
			points[i].x, points[i].y, points[i2].x, points[i2].y,
			uvs[i].x, uvs[i].y, uvs[i2].x, uvs[i2].y, VIEWANGLE);
	}
	int yStart = HEIGHT - 1, yEnd = 0, startLine = -1;
	struct screen_line sLines[4];
	for (int i = 0; i < 4; i++)
	{
		if (!lines[i].clipped)
		{
			float oz1, oz2;
			int ust1, ust2, zb1, zb2, yless, ymore;
			oz1 = 1 / lines[i].x1;
			oz2 = 1 / lines[i].x2;
			ust1 = (FOV * lines[i].y1 * oz1) + WIDTH / 2;
			ust2 = (FOV * lines[i].y2 * oz2) + WIDTH / 2;
			zb1 = (viewHeight * oz1) + HEIGHT / 2;
			zb2 = (viewHeight * oz2) + HEIGHT / 2;
			sLines[i] = (struct screen_line){
				ust1, zb1, ust2, zb2, oz1, oz2,
				lines[i].u1 * oz1, lines[i].v1 * oz1, lines[i].u2 * oz2, lines[i].v2 * oz2};

			yless = zb2 < yStart;
			ymore = zb2 > yEnd;

			if (yless && abs(zb1 - zb2) > 0)
			{
				yStart = zb2;
				if (!reverse)
					startLine = i;
			}
			if (ymore && abs(zb1 - zb2) > 0)
			{
				yEnd = zb2;
				if (reverse)
					startLine = i;
			}
		}
		else
		{
			sLines[i] = (struct screen_line){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
		}
	}
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
				if (y >= 0 && y < HEIGHT)
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
									int index = (u + v * texture->width) * texture->components;
									SDL_SetRenderDrawColor(renderer, texture->pixels[index] * lightlevel, texture->pixels[index + 1] * lightlevel, texture->pixels[index + 2] * lightlevel, 0x00);
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

static void SegmentAndDrawFlats(struct sector *sctr, int ph, struct portal *currentPortal, int yTopLimit[WIDTH], int yBottomLimit[WIDTH])
{
	float segmentSize = 0.75;
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
	if (sctr->floorTexture > 0)
	{
		for (int i = 0; i < blockCount; i++)
		{
			RenderFlat(blocks[i].t1, blocks[i].t2, blocks[i].t3, blocks[i].t4, sctr->floorTexture, ph - sctr->floorheight, sctr->lightlevel, currentPortal, yTopLimit, yBottomLimit, 0);
		}
		// RenderFlat(sctr->t1, sctr->t2, sctr->t3, sctr->t4, sctr->floorTexture, ph - sctr->floorheight, sctr->lightlevel, currentPortal, yTopLimit, yBottomLimit, 0);
	}
	if (sctr->ceilingTexture > 0)
	{
		for (int i = 0; i < blockCount; i++)
		{
			RenderFlat(blocks[i].t1, blocks[i].t2, blocks[i].t3, blocks[i].t4, sctr->ceilingTexture, -(sctr->ceilingheight - ph), sctr->lightlevel, currentPortal, yTopLimit, yBottomLimit, 1);
		}
		// RenderFlat(sctr->t1, sctr->t2, sctr->t3, sctr->t4, sctr->ceilingTexture, -(sctr->ceilingheight - ph), sctr->lightlevel, currentPortal, yTopLimit, yBottomLimit, 1);
	}
	free(blocks);
}

static void RenderWalls()
{
	int maxSectors = 16;
	int yTopLimit[WIDTH], yBottomLimit[WIDTH], yTopTemp[WIDTH], yBottomTemp[WIDTH], renderedSectors[numSectors];
	for (int i = 0; i < WIDTH; i++)
	{
		yTopLimit[i] = 0;
		yTopTemp[i] = 0;
		yBottomLimit[i] = HEIGHT - 1;
		yBottomTemp[i] = HEIGHT - 1;
	}
	for (int i = 0; i < numSectors; i++)
	{
		renderedSectors[i] = 0;
	}
	struct portal queue[maxSectors];

	// Whole screen is the initial portal.
	*queue = (struct portal){player.sector, 0, WIDTH - 1};
	int end = 0, current = -1;

	// Get current player height
	float ph = (int)playerView + playerHeight;
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
				s1 = (FOV * *(ptr + 1) / *ptr) + WIDTH / 2;
				s2 = (FOV * *(ptr + 3) / *(ptr + 2)) + WIDTH / 2;
				us1 = (FOV * roty1 / rotx1) + WIDTH / 2;
				us2 = (FOV * roty2 / rotx2) + WIDTH / 2;
				z1 = 1 / rotx1;
				z2 = 1 / rotx2;
				tex1 = 0;
				texw2 = sctr->lineDef[i].wallTexture > -1 ? (textures[sctr->lineDef[i].wallTexture - 1].width - 1) * z2 : 0;
				texc2 = sctr->lineDef[i].ceilingTexture > -1 ? (textures[sctr->lineDef[i].ceilingTexture - 1].width - 1) * z2 : 0;
				texf2 = sctr->lineDef[i].floorTexture > -1 ? (textures[sctr->lineDef[i].floorTexture - 1].width - 1) * z2 : 0;

				zt1 = (-(sctr->ceilingheight - ph) / *ptr) + HEIGHT / 2;
				zb1 = ((ph - sctr->floorheight) / *ptr) + HEIGHT / 2;
				zt2 = (-(sctr->ceilingheight - ph) / *(ptr + 2)) + HEIGHT / 2;
				zb2 = ((ph - sctr->floorheight) / *(ptr + 2)) + HEIGHT / 2;
				if (sctr->lineDef[i].adjacent > -1)
				{
					stepy1 = (ph - sectors[sctr->lineDef[i].adjacent - 1].floorheight) / *ptr + HEIGHT / 2;
					stepy2 = (ph - sectors[sctr->lineDef[i].adjacent - 1].floorheight) / *(ptr + 2) + HEIGHT / 2;
					ceily1 = -(sectors[sctr->lineDef[i].adjacent - 1].ceilingheight - ph) / *ptr + HEIGHT / 2;
					ceily2 = -(sectors[sctr->lineDef[i].adjacent - 1].ceilingheight - ph) / *(ptr + 2) + HEIGHT / 2;
				}

				// Wall is facing the player only.
				if (s1 < s2)
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
						int yt = Clamp(yBottomTemp[x], yTopTemp[x], yTop);
						int yb = Clamp(yBottomTemp[x], yTopTemp[x], yBottom);
						float dx = *ptr * (1 - t) + *(ptr + 2) * t;
						float dy = *(ptr + 1) * (1 - t) + *(ptr + 3) * t;
						float distance = dx * dx + dy * dy;
						// // Draw roofs and floors.
						if (sctr->ceilingTexture == -1 || !showTextures)
						{
							RenderLine(x, yTopTemp[x], yt, yTop, yBottom, 0x78, 0x78, 0x78, distance, sctr->lightlevel, -1, 1, 0, -1, current);
						}
						if (sctr->floorTexture == -1 || !showTextures)
						{
							RenderLine(x, yb, yBottomTemp[x], yTop, yBottom, 0x79, 0x79, 0x79, distance, sctr->lightlevel, -1, 0, 1, -1, current);
						}
						if (sctr->lineDef[i].adjacent > -1)
						{
							if (sectors[sctr->lineDef[i].adjacent - 1].floorheight > sctr->floorheight)
							{
								// Create a floor wall for the change in height.
								float tex = tex1 * (1 - t2) + texf2 * t2;
								int u = tex / z;
								int realStepY = stepy1 * (1 - t) + stepy2 * t + 0.5;
								int stepY = Clamp(yBottomTemp[x], yTopTemp[x], realStepY); // +0.5 to remove jaggies.
								RenderLine(x, stepY, yb, realStepY, yBottom, 0x37, 0xcd, 0xc1, distance, sctr->lightlevel, u, 0, 0, sctr->lineDef[i].floorTexture, current);
								yBottomTemp[x] = stepY;
							}
							else
							{
								yBottomTemp[x] = yb;
							}

							if (sectors[sctr->lineDef[i].adjacent - 1].ceilingheight < sctr->ceilingheight)
							{
								// Create a ceiling for the change in height.
								float tex = tex1 * (1 - t2) + texc2 * t2;
								int u = tex / z;
								int realCeilY = ceily1 * (1 - t) + ceily2 * t + 0.5;
								int ceilY = Clamp(yBottomTemp[x], yTopTemp[x], realCeilY); // +0.5 to remove jaggies.
								RenderLine(x, yt, ceilY, yTop, realCeilY, 0xa7, 0x37, 0xcd, distance, sctr->lightlevel, u, 0, 0, sctr->lineDef[i].ceilingTexture, current);
								yTopTemp[x] = ceilY;
							}
							else
							{
								yTopTemp[x] = yt;
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
					if (sctr->lineDef[i].adjacent > -1 && end != maxSectors - 1)
					{
						end++;
						queue[end] = (struct portal){sctr->lineDef[i].adjacent, x1, x2};
					}
				}
			}
		}
		for (int i = 0; i < sctr->objectNum; i++)
		{
			struct object obj = sctr->objectDef[i];
			float rotx, roty, lightlevel;
			rotx = (obj.x - player.position.x) * cos(player.angle) + (obj.y - player.position.y) * sin(player.angle);
			roty = (obj.y - player.position.y) * cos(player.angle) - (obj.x - player.position.x) * sin(player.angle);
			if (rotx > 0)
			{
				float oz;
				int sx, sy, x1, x2, fy, tx, ty;
				sx = (FOV * roty / rotx) + WIDTH / 2;
				sy = ((ph - sctr->floorheight) / rotx) + HEIGHT / 2;
				oz = 1 / rotx;
				lightlevel = ((oz) > 1 ? 1 : oz) * sctr->lightlevel;
				tx = obj.width * oz / 2;
				ty = obj.height * oz;
				fy = sy - ty;
				x1 = sx - tx;
				x2 = sx + tx;
				for (int y = fy; y <= sy; y++)
				{
					float t1 = (y - fy) / (float)(sy - fy);
					int v = 0 * (1 - t1) + obj.height * t1;
					for (int x = x1; x <= x2; x++)
					{
						if (y > yTopLimit[x] && y < yBottomLimit[x] && x > 0 && x < WIDTH && x > currentPortal->x1 && x < currentPortal->x2)
						{
							float t2 = (x - x1) / (float)(x2 - x1);
							int u = 0 * (1 - t2) + obj.width * t2;
							int index = (u + v * obj.width) * obj.components;
							int r = obj.pixels[index], g = obj.pixels[index + 1], b = obj.pixels[index + 2];
							if (r != 0xff && g != 0x00 && b != 0xaf)
							{
								SDL_SetRenderDrawColor(renderer, obj.pixels[index] * lightlevel, obj.pixels[index + 1] * lightlevel, obj.pixels[index + 2] * lightlevel, 0x00);
								SDL_RenderDrawPoint(renderer, x, y);
							}
						}
					}
				}
			}
		}
		for (int i = 0; i < WIDTH; i++)
		{
			yTopLimit[i] = yTopTemp[i];
			yBottomLimit[i] = yBottomTemp[i];
		}
	} while (current != end);
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
				if ((sectors[sectors[i].lineDef[j].adjacent - 1].floorheight - sectors[player.sector - 1].floorheight) <= 30)
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
					player.velocity.x = *dir;
					player.velocity.y = *(dir + 1);
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

		player.angle -= keysPressed[4] ? 0.04f : 0;
		player.angle += keysPressed[5] ? 0.04f : 0;

		end = clock();
		double diff = difftime(end, start);
		if (diff < 16667)
		{
			int wait = 16667 - diff;
			usleep(wait);
		}
	}
}