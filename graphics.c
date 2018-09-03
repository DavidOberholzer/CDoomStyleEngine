#include <SDL2/SDL.h>

#include "game.h"
#include "graphics.h"
#include "data_io.h"
#include "structures.h"
#include "worldmath.h"

// SDL Window and Surface.
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Surface *surface = NULL;

void InitGraphics()
{
	SDL_Init(SDL_INIT_EVERYTHING);

	window = SDL_CreateWindow("2.5d Engine David", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);

	if (window == NULL)
	{
		printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
		exit(1);
	}

	surface = SDL_GetWindowSurface(window);
	renderer = SDL_CreateSoftwareRenderer(surface);

	if (renderer == NULL)
	{
		printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
		exit(1);
	}

	SDL_ShowCursor(SDL_DISABLE);
}

void ClearFrame()
{
	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
	SDL_RenderClear(renderer);
}

void RenderLine(int x, int y1, int y2, int yt, int yb, int R, int G, int B, float distance, float sctrLight, int u, int roof, int ground, int textureIndex, int currentSector)
{
	float light_level = showTextures ? 0.4 * sctrLight : 1;
	if (roof == 1 || ground == 1)
	{
		if (currentSector != 0 || roof == 1 || showTextures == 0)
		{
			SDL_SetRenderDrawColor(renderer, R * light_level, G * light_level, B * light_level, 0x00);
			SDL_RenderDrawLine(renderer, x, y1, x, y2);
		}
		else
		{
			for (int y = y1; y <= y2; y++)
			{
				SDL_SetRenderDrawColor(renderer, light_level * R, light_level * G, light_level * B, 0x00);
				SDL_RenderDrawPoint(renderer, x, y);
			}
		}
	}
	else
	{
		light_level = showTextures ? (distance < 6 ? 1 : distance > 30 ? 0.2 : 6 / distance) * sctrLight : 1;
		if ((u < 0 || textureIndex == -1) || showTextures != 1)
		{
			SDL_SetRenderDrawColor(renderer, R * light_level, G * light_level, B * light_level, 0x00);
			SDL_RenderDrawLine(renderer, x, y1, x, y2);
		}
		else
		{
			struct texture *texture = &textures[textureIndex - 1];
			for (int y = yt; y <= yb; y++)
			{
				if (y >= y1 && y <= y2)
				{
					float dy = (y - yb) / (float)(yt - yb);
					int height = dy * (texture->height - 1);
					int index = (u + height * texture->width) * texture->components;
					SDL_SetRenderDrawColor(renderer, texture->pixels[index] * light_level, texture->pixels[index + 1] * light_level, texture->pixels[index + 2] * light_level, 0x00);
					SDL_RenderDrawPoint(renderer, x, y);
				}
			}
		}
	}
}

void PresentFrame()
{
	SDL_UpdateWindowSurface(window);
}

void TearDownGraphics()
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}