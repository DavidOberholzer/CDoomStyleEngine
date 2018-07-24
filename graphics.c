#include <SDL2/SDL.h>

#include "graphics.h"
#include "data_io.h"
#include "structures.h"

// SDL Window and Surface.
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;

void InitGraphics()
{
	SDL_Init(SDL_INIT_EVERYTHING);

	window = SDL_CreateWindow("2.5d Engine David", 0, 0, WIDTH, HEIGHT, SDL_WINDOW_MAXIMIZED);

	if (window == NULL)
	{
		printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
		exit(1);
	}

	renderer = SDL_CreateRenderer(window, -1, 0);

	if (renderer == NULL)
	{
		printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
		exit(1);
	}

	// Get Texture from renderer.
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WIDTH, HEIGHT);

	if (texture == NULL)
	{
		printf("SDL_CreateTexture failed: %s\n", SDL_GetError());
		exit(1);
	}

	SDL_ShowCursor(SDL_DISABLE);
}

void ClearFrame()
{
	SDL_SetRenderTarget(renderer, texture);
	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
	SDL_RenderClear(renderer);
}

void RenderLine(int x, int y1, int y2, int R, int G, int B, float distance, int roof, int ground)
{
	float light_level = roof == 0 && ground == 0 ? distance < 4 ? 1 : distance > 20 ? 0.2 : 4 / distance : 0.4;
	SDL_SetRenderDrawColor(renderer, R * light_level, G * light_level, B * light_level, 0x00);
	SDL_RenderDrawLine(renderer, x, y1, x, y2);
}

void PresentFrame()
{
	SDL_SetRenderTarget(renderer, NULL);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void DrawTexture(int index)
{
	struct texture *texture = &textures[index];
	int length = texture->width * texture->height * texture->components;
	int width = 0;
	for (int i = 0; i < length; i += texture->components)
	{
		if (width == 400) {
			width = 0;
		}
		width++;
		int height = floor(i / (float)(texture->width * texture->components));
		SDL_SetRenderDrawColor(renderer, texture->pixels[i], texture->pixels[i + 1], texture->pixels[i + 2], 0x00);
		SDL_RenderDrawPoint(renderer, width, height);
	}
}

void TearDownGraphics()
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}