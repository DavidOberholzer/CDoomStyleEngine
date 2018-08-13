#include "data_io.h"
#include "game.h"
#include "graphics.h"
#include "worldmath.h"

static float playerView = 40;
static float viewMovement = 0;
static int viewDown = 1;
static int playerHeight = 0;
static int fallHeight = 0;
static int fallingVelocity = 0;

static void RenderWalls()
{
	int maxSectors = 16;
	int yTopLimit[WIDTH], yBottomLimit[WIDTH], renderedSectors[numSectors];
	for (int i = 0; i < WIDTH; i++)
	{
		yTopLimit[i] = 0;
		yBottomLimit[i] = HEIGHT - 1;
	}
	for (int i = 0; i < numSectors; i++)
	{
		renderedSectors[i] = 0;
	}
	struct portal
	{
		int sectorNo, x1, x2;
	} queue[maxSectors];

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
				int s1, s2, us1, us2, tex1, tex2, zt1, zb1, zt2, zb2, stepy1, stepy2, ceily1, ceily2;
				float z1, z2;
				// Wall X transformation
				s1 = (500 * *(ptr + 1) / *ptr) + WIDTH / 2;
				s2 = (500 * *(ptr + 3) / *(ptr + 2)) + WIDTH / 2;
				us1 = (500 * roty1 / rotx1) + WIDTH / 2;
				us2 = (500 * roty2 / rotx2) + WIDTH / 2;
				z1 = 1 / rotx1;
				z2 = 1 / rotx2;
				tex1 = 0 * z1;
				tex2 = 400 * z2;
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
						float tex = tex1 * (1 - t2) + tex2 * t2;
						int u = tex / z;
						int yTop = zt1 * (1 - t) + zt2 * t + 0.5;	// +0.5 to remove jaggies.
						int yBottom = zb1 * (1 - t) + zb2 * t + 0.5; // +0.5 to remove jaggies.
						int yt = Clamp(yBottomLimit[x], yTopLimit[x], yTop);
						int yb = Clamp(yBottomLimit[x], yTopLimit[x], yBottom);
						float dx = *ptr * (1 - t) + *(ptr + 2) * t;
						float dy = *(ptr + 1) * (1 - t) + *(ptr + 3) * t;
						float distance = dx * dx + dy * dy;
						// // Draw roofs and floors.
						RenderLine(x, yTopLimit[x], yt, yTop, yBottom, 0x78 * sctr->lightlevel, 0x78 * sctr->lightlevel, 0x78 * sctr->lightlevel, distance, -1, 1, 0, -1, current);
						RenderLine(x, yb, yBottomLimit[x], yTop, yBottom, 0x79 * sctr->lightlevel, 0x79 * sctr->lightlevel, 0x79 * sctr->lightlevel, distance, -1, 0, 1, -1, current);

						if (sctr->lineDef[i].adjacent > -1)
						{
							if (sectors[sctr->lineDef[i].adjacent - 1].floorheight > sctr->floorheight)
							{
								// Create a floor wall for the change in height.
								int realStepY = stepy1 * (1 - t) + stepy2 * t + 0.5;
								int stepY = Clamp(yBottomLimit[x], yTopLimit[x], realStepY); // +0.5 to remove jaggies.
								RenderLine(x, stepY, yb, realStepY, yBottom, 0x37, 0xcd, 0xc1, distance, u, 0, 0, sctr->lineDef[i].floorTexture, current);
								yBottomLimit[x] = stepY;
							}
							else
							{
								yBottomLimit[x] = yb;
							}

							if (sectors[sctr->lineDef[i].adjacent - 1].ceilingheight < sctr->ceilingheight)
							{
								// Create a ceiling for the change in height.
								int realCeilY = ceily1 * (1 - t) + ceily2 * t + 0.5;
								int ceilY = Clamp(yBottomLimit[x], yTopLimit[x], realCeilY); // +0.5 to remove jaggies.
								RenderLine(x, yt, ceilY, yTop, realCeilY, 0xa7, 0x37, 0xcd, distance, u, 0, 0, sctr->lineDef[i].ceilingTexture, current);
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
							RenderLine(x, yt, yb, yTop, yBottom, 0xcc, 0xc5, 0xce, distance, u, 0, 0, sctr->lineDef[i].wallTexture, current);
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
		// ++renderedSectors[currentPortal->sectorNo];
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
				float crossing = SideOfLine(player.position.x + player.velocity.x, player.position.y + player.velocity.y, sectors[i].lineDef[j].x1, sectors[i].lineDef[j].y1, sectors[i].lineDef[j].x2, sectors[i].lineDef[j].y2);
				int overlap = BoxesOverlap(player.position.x, player.position.y, player.position.x + player.velocity.x, player.position.y + player.velocity.y, sectors[i].lineDef[j].x1, sectors[i].lineDef[j].y1, sectors[i].lineDef[j].x2, sectors[i].lineDef[j].y2);
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
	while (!close)
	{
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

		player.angle -= keysPressed[4] ? 0.03f : 0;
		player.angle += keysPressed[5] ? 0.03f : 0;

		SDL_Delay(10);
	}
}