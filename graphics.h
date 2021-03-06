#ifndef GRAPHICS_INCLUDED
#define GRAPHICS_INCLUDED

#include <SDL2/SDL.h>

#define WIDTH 640
#define WIDTHD2 WIDTH / 2
#define HEIGHT 480
#define HEIGHTD2 HEIGHT / 2
#define HUD HEIGHT / 6
#define PI 3.141592653589793238
#define VIEWANGLE 5 * PI / 12
#define MAX_SECTORS 52

extern SDL_Window *window;
extern SDL_Renderer *renderer;
extern SDL_Surface *surface;

void InitGraphics();
void ClearFrame();
void RenderLine(int, int, int, int, int, int, int, int, float, float, int, int, int, int, int);
void PresentFrame();
void TearDownGraphics();

#endif