#ifndef GRAPHICS_INCLUDED
#define GRAPHICS_INCLUDED

#include <SDL2/SDL.h>

#define WIDTH 640
#define HEIGHT 480
#define PI 3.141592653589793238
#define VIEWANGLE 5 * PI / 12
#define MOVESPEED 0.05

extern SDL_Window *window;
extern SDL_Renderer *renderer;
extern SDL_Texture *texture;

void InitGraphics();
void ClearFrame();
void RenderLine(int, int, int, int, int, int, int, int, float, int, int, int, int);
void PresentFrame();
void TearDownGraphics();

#endif