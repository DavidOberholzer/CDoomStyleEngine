#include <time.h>
#include <unistd.h>

#include "data_io.h"
#include "game.h"
#include "graphics.h"
#include "worldmath.h"

#define MOVESPEED 0.03

static float playerView = 40;
static float viewMovement = 0;
static int viewDown = 1;
static int playerHeight = 0;
static int fallHeight = 0;
static int fallingVelocity = 0;
int showTextures = 1;

static void RenderWalls()
{
	struct texture *texture = &textures[3];
	struct xy t1 = {0.3, 0.2}, t2 = {0.3, 0.8}, t3 = {0.8, 0.8}, t4 = {0.8, 0.2};
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
	int yStart = HEIGHT, yEnd = 0;
	struct screen_line sLines[4];
	for (int i = 0; i < 4; i++)
	{
		if (!lines[i].clipped)
		{
			float oz1, oz2;
			int ust1, ust2, zb1, zb2;
			oz1 = 1 / lines[i].x1;
			oz2 = 1 / lines[i].x2;
			ust1 = (500 * lines[i].y1 * oz1) + WIDTH / 2;
			ust2 = (500 * lines[i].y2 * oz2) + WIDTH / 2;
			zb1 = (50 * oz1) + HEIGHT / 2;
			zb2 = (50 * oz2) + HEIGHT / 2;
			sLines[i] = (struct screen_line){
				ust1, zb1, ust2, zb2, oz1, oz2,
				lines[i].u1 * oz1, lines[i].v1 * oz1, lines[i].u2 * oz2, lines[i].v2 * oz2};
			yStart = zb1 < yStart ? zb1 : yStart;
			yStart = zb2 < yStart ? zb2 : yStart;
			yEnd = zb1 > yEnd ? zb1 : yEnd;
			yEnd = zb2 > yEnd ? zb2 : yEnd;
		}
		else
		{
			sLines[i] = (struct screen_line){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
		}
	}
	yStart = Clamp(HEIGHT - 1, 0, yStart);
	yEnd = Clamp(HEIGHT - 1, 0, yEnd);
	for (int y = yStart; y <= yEnd; y++)
	{
		struct screen_line spanLines[2];
		int spansFound = 0;
		for (int j = 0; j < 4; j++)
		{
			int in = sLines[j].sy1 > sLines[j].sy2 ? (y <= sLines[j].sy1 && y >= sLines[j].sy2) : (y >= sLines[j].sy1 && y <= sLines[j].sy2);
			if (in)
			{
				spanLines[spansFound++] = sLines[j];
				if (spansFound == 2)
					break;
			}
		}
		int x1, x2;
		float oz, uoz1, uoz2, voz1, voz2;
		if (spanLines[0].sy2 > spanLines[0].sy1)
		{
			float t = (y - spanLines[0].sy1) / (float)(spanLines[0].sy2 - spanLines[0].sy1);
			x1 = spanLines[0].sx1 * (1 - t) + spanLines[0].sx2 * t;
			uoz1 = spanLines[0].uoz1 * (1 - t) + spanLines[0].uoz2 * t;
			voz1 = spanLines[0].voz1 * (1 - t) + spanLines[0].voz2 * t;
			oz = spanLines[0].oz1 * (1 - t) + spanLines[0].oz2 * t;
		}
		else
		{
			float t = (y - spanLines[0].sy2) / (float)(spanLines[0].sy1 - spanLines[0].sy2);
			x1 = spanLines[0].sx2 * (1 - t) + spanLines[0].sx1 * t;
			uoz1 = spanLines[0].uoz2 * (1 - t) + spanLines[0].uoz1 * t;
			voz1 = spanLines[0].voz2 * (1 - t) + spanLines[0].voz1 * t;
			oz = spanLines[0].oz2 * (1 - t) + spanLines[0].oz1 * t;
		}
		if (spanLines[1].sy2 > spanLines[1].sy1)
		{
			float t = (y - spanLines[1].sy1) / (float)(spanLines[1].sy2 - spanLines[1].sy1);
			uoz2 = spanLines[1].uoz1 * (1 - t) + spanLines[1].uoz2 * t;
			voz2 = spanLines[1].voz1 * (1 - t) + spanLines[1].voz2 * t;
			x2 = spanLines[1].sx1 * (1 - t) + spanLines[1].sx2 * t;
		}
		else
		{
			float t = (y - spanLines[1].sy2) / (float)(spanLines[1].sy1 - spanLines[1].sy2);
			uoz2 = spanLines[1].uoz2 * (1 - t) + spanLines[1].uoz1 * t;
			voz2 = spanLines[1].voz2 * (1 - t) + spanLines[1].voz1 * t;
			x2 = spanLines[1].sx2 * (1 - t) + spanLines[1].sx1 * t;
		}
		x1 = Clamp(WIDTH, 0, x1);
		x2 = Clamp(WIDTH, 0, x2);
		int xDir = x1 > x2;
		int startX = xDir ? x2 : x1,
			endX = xDir ? x1 : x2;
		float startU = xDir ? uoz2 : uoz1,
			  endU = xDir ? uoz1 : uoz2,
			  startV = xDir ? voz2 : voz1,
			  endV = xDir ? voz1 : voz2;
		for (int x = startX; x <= endX; x++)
		{
			float t = (x - startX) / (float)(endX - startX);
			int u = Clamp(texture->width - 1, 0, (startU * (1 - t) + endU * t) / oz);
			int v = Clamp(texture->height - 1, 0, (startV * (1 - t) + endV * t) / oz);
			int index = (u + v * texture->width) * texture->components;
			SDL_SetRenderDrawColor(renderer, texture->pixels[index], texture->pixels[index + 1], texture->pixels[index + 2], 0x00);
			SDL_RenderDrawPoint(renderer, x, y);
		}
	}
}

// static void RenderWalls()
// {
// 	int maxSectors = 16;
// 	int yTopLimit[WIDTH], yBottomLimit[WIDTH], renderedSectors[numSectors];
// 	for (int i = 0; i < WIDTH; i++)
// 	{
// 		yTopLimit[i] = 0;
// 		yBottomLimit[i] = HEIGHT - 1;
// 	}
// 	for (int i = 0; i < numSectors; i++)
// 	{
// 		renderedSectors[i] = 0;
// 	}
// 	struct portal
// 	{
// 		int sectorNo, x1, x2;
// 	} queue[maxSectors];

// 	// Whole screen is the initial portal.
// 	*queue = (struct portal){player.sector, 0, WIDTH - 1};
// 	int end = 0, current = -1;

// 	// Get current player height
// 	float ph = (int)playerView + playerHeight;
// 	do
// 	{
// 		current += 1;
// 		struct portal *currentPortal = queue + current;
// 		struct sector *sctr = &sectors[currentPortal->sectorNo - 1];
// 		for (int i = 0; i < sctr->lineNum; i++)
// 		{
// 			// First Vector rotation of the lines around the player.
// 			float rotx1, roty1, rotx2, roty2;
// 			rotx1 = (sctr->lineDef[i].x1 - player.position.x) * cos(player.angle) + (sctr->lineDef[i].y1 - player.position.y) * sin(player.angle);
// 			roty1 = (sctr->lineDef[i].y1 - player.position.y) * cos(player.angle) - (sctr->lineDef[i].x1 - player.position.x) * sin(player.angle);
// 			rotx2 = (sctr->lineDef[i].x2 - player.position.x) * cos(player.angle) + (sctr->lineDef[i].y2 - player.position.y) * sin(player.angle);
// 			roty2 = (sctr->lineDef[i].y2 - player.position.y) * cos(player.angle) - (sctr->lineDef[i].x2 - player.position.x) * sin(player.angle);

// 			// Clip line to the view cone.
// 			float *ptr;
// 			ptr = ClipViewCone(rotx1, roty1, rotx2, roty2, VIEWANGLE);

// 			if (*ptr > 0 && *(ptr + 2) > 0)
// 			{
// 				int s1, s2, us1, us2, tex1, texw2, texc2, texf2, zt1, zb1, zt2, zb2, stepy1, stepy2, ceily1, ceily2;
// 				float z1, z2;
// 				// Wall X transformation
// 				s1 = (500 * *(ptr + 1) / *ptr) + WIDTH / 2;
// 				s2 = (500 * *(ptr + 3) / *(ptr + 2)) + WIDTH / 2;
// 				us1 = (500 * roty1 / rotx1) + WIDTH / 2;
// 				us2 = (500 * roty2 / rotx2) + WIDTH / 2;
// 				z1 = 1 / rotx1;
// 				z2 = 1 / rotx2;
// 				tex1 = 0;
// 				texw2 = sctr->lineDef[i].wallTexture > -1 ? (textures[sctr->lineDef[i].wallTexture - 1].width - 1) * z2 : 0;
// 				texc2 = sctr->lineDef[i].ceilingTexture > -1 ? (textures[sctr->lineDef[i].ceilingTexture - 1].width - 1) * z2 : 0;
// 				texf2 = sctr->lineDef[i].floorTexture > -1 ? (textures[sctr->lineDef[i].floorTexture - 1].width - 1) * z2 : 0;

// 				zt1 = (-(sctr->ceilingheight - ph) / *ptr) + HEIGHT / 2;
// 				zb1 = ((ph - sctr->floorheight) / *ptr) + HEIGHT / 2;
// 				zt2 = (-(sctr->ceilingheight - ph) / *(ptr + 2)) + HEIGHT / 2;
// 				zb2 = ((ph - sctr->floorheight) / *(ptr + 2)) + HEIGHT / 2;
// 				if (sctr->lineDef[i].adjacent > -1)
// 				{
// 					stepy1 = (ph - sectors[sctr->lineDef[i].adjacent - 1].floorheight) / *ptr + HEIGHT / 2;
// 					stepy2 = (ph - sectors[sctr->lineDef[i].adjacent - 1].floorheight) / *(ptr + 2) + HEIGHT / 2;
// 					ceily1 = -(sectors[sctr->lineDef[i].adjacent - 1].ceilingheight - ph) / *ptr + HEIGHT / 2;
// 					ceily2 = -(sectors[sctr->lineDef[i].adjacent - 1].ceilingheight - ph) / *(ptr + 2) + HEIGHT / 2;
// 				}

// 				// Wall is facing the player only.
// 				if (s1 < s2)
// 				{
// 					int x1 = Max(currentPortal->x1, s1);
// 					int x2 = Min(currentPortal->x2, s2);
// 					for (int x = x1; x <= x2; x++)
// 					{
// 						float t = (x - s1) / (float)(s2 - s1);
// 						float t2 = (x - us1) / (float)(us2 - us1);
// 						float z = z1 * (1 - t2) + z2 * t2;
// 						int yTop = zt1 * (1 - t) + zt2 * t + 0.5;	// +0.5 to remove jaggies.
// 						int yBottom = zb1 * (1 - t) + zb2 * t + 0.5; // +0.5 to remove jaggies.
// 						int yt = Clamp(yBottomLimit[x], yTopLimit[x], yTop);
// 						int yb = Clamp(yBottomLimit[x], yTopLimit[x], yBottom);
// 						float dx = *ptr * (1 - t) + *(ptr + 2) * t;
// 						float dy = *(ptr + 1) * (1 - t) + *(ptr + 3) * t;
// 						float distance = dx * dx + dy * dy;
// 						// // Draw roofs and floors.
// 						if (sctr->ceilingTexture == -1) {
// 							RenderLine(x, yTopLimit[x], yt, yTop, yBottom, 0x78, 0x78, 0x78, distance, sctr->lightlevel, -1, 1, 0, -1, current);
// 						}
// 						if (sctr->floorTexture == -1) {
// 							RenderLine(x, yb, yBottomLimit[x], yTop, yBottom, 0x79, 0x79, 0x79, distance, sctr->lightlevel, -1, 0, 1, -1, current);
// 						}
// 						if (sctr->lineDef[i].adjacent > -1)
// 						{
// 							if (sectors[sctr->lineDef[i].adjacent - 1].floorheight > sctr->floorheight)
// 							{
// 								// Create a floor wall for the change in height.
// 								float tex = tex1 * (1 - t2) + texf2 * t2;
// 								int u = tex / z;
// 								int realStepY = stepy1 * (1 - t) + stepy2 * t + 0.5;
// 								int stepY = Clamp(yBottomLimit[x], yTopLimit[x], realStepY); // +0.5 to remove jaggies.
// 								RenderLine(x, stepY, yb, realStepY, yBottom, 0x37, 0xcd, 0xc1, distance, sctr->lightlevel, u, 0, 0, sctr->lineDef[i].floorTexture, current);
// 								yBottomLimit[x] = stepY;
// 							}
// 							else
// 							{
// 								yBottomLimit[x] = yb;
// 							}

// 							if (sectors[sctr->lineDef[i].adjacent - 1].ceilingheight < sctr->ceilingheight)
// 							{
// 								// Create a ceiling for the change in height.
// 								float tex = tex1 * (1 - t2) + texc2 * t2;
// 								int u = tex / z;
// 								int realCeilY = ceily1 * (1 - t) + ceily2 * t + 0.5;
// 								int ceilY = Clamp(yBottomLimit[x], yTopLimit[x], realCeilY); // +0.5 to remove jaggies.
// 								RenderLine(x, yt, ceilY, yTop, realCeilY, 0xa7, 0x37, 0xcd, distance, sctr->lightlevel, u, 0, 0, sctr->lineDef[i].ceilingTexture, current);
// 								yTopLimit[x] = ceilY;
// 							}
// 							else
// 							{
// 								yTopLimit[x] = yt;
// 							}
// 						}
// 						else
// 						{
// 							// Draw a normal wall.
// 							float tex = tex1 * (1 - t2) + texw2 * t2;
// 							int u = tex / z;
// 							RenderLine(x, yt, yb, yTop, yBottom, 0xcc, 0xc5, 0xce, distance, sctr->lightlevel, u, 0, 0, sctr->lineDef[i].wallTexture, current);
// 						}
// 					}
// 					// Load in next in next portal if adjacent sector found.
// 					if (sctr->lineDef[i].adjacent > -1 && end != maxSectors - 1)
// 					{
// 						end++;
// 						queue[end] = (struct portal){sctr->lineDef[i].adjacent, x1, x2};
// 					}
// 				}
// 			}
// 		}
// 		if (sctr->ceilingTexture > -1 || sctr->floorTexture > -1)
// 		{
// 			float rott1x, rott1y, rott2x, rott2y, rott3x, rott3y, rott4x, rott4y;
// 			rott1x = (sctr->t1.x - player.position.x) * cos(player.angle) + (sctr->t1.y - player.position.y) * sin(player.angle);
// 			rott1y = (sctr->t1.y - player.position.y) * cos(player.angle) - (sctr->t1.x - player.position.x) * sin(player.angle);
// 			rott2x = (sctr->t2.x - player.position.x) * cos(player.angle) + (sctr->t2.y - player.position.y) * sin(player.angle);
// 			rott2y = (sctr->t2.y - player.position.y) * cos(player.angle) - (sctr->t2.x - player.position.x) * sin(player.angle);
// 			rott3x = (sctr->t3.x - player.position.x) * cos(player.angle) + (sctr->t3.y - player.position.y) * sin(player.angle);
// 			rott3y = (sctr->t3.y - player.position.y) * cos(player.angle) - (sctr->t3.x - player.position.x) * sin(player.angle);
// 			rott4x = (sctr->t4.x - player.position.x) * cos(player.angle) + (sctr->t4.y - player.position.y) * sin(player.angle);
// 			rott4y = (sctr->t4.y - player.position.y) * cos(player.angle) - (sctr->t4.x - player.position.x) * sin(player.angle);

// 			int ust1, ust2, ust3, ust4, uv = 0, u2, v2, zb1, zb2, zb3, zb4;
// 			float zx1, zx2, zy1, zy2;
// 			ust1 = (500 * rott1y / rott1x) + WIDTH / 2;
// 			ust2 = (500 * rott2y / rott2x) + WIDTH / 2;
// 			ust3 = (500 * rott3y / rott3x) + WIDTH / 2;
// 			ust4 = (500 * rott4y / rott4x) + WIDTH / 2;
// 			zb1 = ((ph - sctr->floorheight) / rott1x) + HEIGHT / 2;
// 			zb2 = ((ph - sctr->floorheight) / rott2x) + HEIGHT / 2;
// 			zb3 = ((ph - sctr->floorheight) / rott3x) + HEIGHT / 2;
// 			zb4 = ((ph - sctr->floorheight) / rott4x) + HEIGHT / 2;
// 			if (ust1 >= 0 && ust1 < WIDTH && zb1 >= 0 && zb1 < HEIGHT)
// 			{
// 				SDL_SetRenderDrawColor(renderer, 0x18, 0xff, 0x15, 0x00);
// 				SDL_RenderDrawPoint(renderer, ust1, zb1);
// 			}
// 			// if (ust2 >= 0 && ust2 < WIDTH && zb2 >= 0 && zb2 < HEIGHT)
// 			// {
// 			// 	SDL_SetRenderDrawColor(renderer, 0x18, 0xff, 0x15, 0x00);
// 			// 	SDL_RenderDrawPoint(renderer, ust2, zb2);
// 			// }
// 			// if (ust3 >= 0 && ust3 < WIDTH && zb3 >= 0 && zb3 < HEIGHT)
// 			// {
// 			// 	SDL_SetRenderDrawColor(renderer, 0x18, 0xff, 0x15, 0x00);
// 			// 	SDL_RenderDrawPoint(renderer, ust3, zb3);
// 			// }
// 			// if (ust4 >= 0 && ust4 < WIDTH && zb4 >= 0 && zb4 < HEIGHT)
// 			// {
// 			// 	SDL_SetRenderDrawColor(renderer, 0x18, 0xff, 0x15, 0x00);
// 			// 	SDL_RenderDrawPoint(renderer, ust4, zb4);
// 			// }
// 			// zx1 = 1 / rott1x;
// 			// zx2 = 1 / rott3x;
// 			// z4 = 1 / rott4x;
// 			// u2 = textures[sctr->floorTexture - 1].width * z
// 		}
// 		// ++renderedSectors[currentPortal->sectorNo];
// 	} while (current != end);
// }

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