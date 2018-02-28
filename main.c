#include <math.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define WIDTH 640
#define HEIGHT 480
#define PI 3.141592653589793238
#define VIEWANGLE 5 * PI / 12
#define PLAYERHEIGHT 200
#define MOVESPEED 0.1

// SDL Window and Surface.

static SDL_Window *window = NULL;
static SDL_Surface *surface = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;

// Vertex
struct xy
{
    float x, y;
};

// Render view window.
struct ViewWindow
{
    float x1, x2, yt1, yb1, yt2, yb2;
    signed char adjacent;
    int rendered;
} *portals = NULL;
int numPortals = 0;

// Sectors
static struct sector
{
    float floorheight, ceilingheight, lightlevel;
    struct line
    {
        float x1, y1, x2, y2;
        signed char adjacent;
    } * lineDef;
    unsigned lineNum;
} *sectors = NULL;
int numSectors = 0;

// Player location
static struct player
{
    struct xy position, velocity;
    float angle;
    unsigned sector;
} player;

static void LoadMapFile()
{
    FILE *data = fopen("maps/testmap1.txt", "rt");
    if (!data)
    {
        perror("Map did not load!");
        exit(1);
    }
    char Buf[256], word[256], *ptr;
    struct line *lines = NULL, ln;
    struct xy position = {0, 0};
    int n, lineCount = 0, sector;
    float angle;
    while (fgets(Buf, sizeof Buf, data))
    {
        ptr = Buf;
        if (sscanf(ptr, "%32s%n", word, &n) == 1)
        {
            switch (word[0])
            {
            // Load lines
            case 'l':
                sscanf(ptr += n, "%f%f%f%f%1s%n", &ln.x1, &ln.y1, &ln.x2, &ln.y2, word, &n);
                ln.adjacent = word[0] == 'x' ? -1 : atoi(word);
                lines = realloc(lines, ++lineCount * sizeof(*lines));
                lines[lineCount - 1] = ln;
                break;
            // Load Sectors
            case 's':
                sectors = realloc(sectors, ++numSectors * sizeof(*sectors));
                struct sector *sctr = &sectors[numSectors - 1];
                sscanf(ptr += n, "%f%f%f%n", &sctr->floorheight, &sctr->ceilingheight, &sctr->lightlevel, &n);
                int l;
                int *lineLoc = NULL;
                for (l = 0; sscanf(ptr += n, "%32s%n", word, &n) == 1;)
                {
                    lineLoc = realloc(lineLoc, ++l * sizeof(*lineLoc));
                    lineLoc[l - 1] = atoi(word);
                }
                sctr->lineNum = l;
                sctr->lineDef = malloc(l * sizeof(*sctr->lineDef));
                for (int m = 0; m < l; m++)
                {
                    sctr->lineDef[m] = lines[lineLoc[m] - 1];
                }
                free(lineLoc);
                break;
            // Load player position and starting sector
            case 'p':
                sscanf(ptr += n, "%f%f%f%d%n", &position.x, &position.y, &angle, &sector, &n);
                player = (struct player){{position.x, position.y},
                                         {0, 0},
                                         angle,
                                         sector};
                break;
            }
        }
    }
}

static void UnloadData()
{
    for (int a = 0; a < numSectors; a++)
    {
        free(sectors[a].lineDef);
    }
    free(sectors);
    sectors = NULL;
    numSectors = 0;
}

float DotProduct(float vx1, float vy1, float vx2, float vy2)
{
    return (vx1 * vx2 + vy1 * vy2);
}

float CrossProduct(float vx1, float vy1, float vx2, float vy2)
{
    return (vx1 * vy2 - vx2 * vy1);
}

float SideOfLine(float px, float py, float x1, float y1, float x2, float y2)
{
    return CrossProduct(x2 - x1, y2 - y1, px - x1, py - y1);
}

int Max(float x, float y)
{
    return x > y ? x : y;
}

int Min(float x, float y)
{
    return x < y ? x : y;
}

float Clamp(float num, float limit)
{
    return Max(Min(limit, num), -limit);
}

int RangesOverlap(float a0, float a1, float b0, float b1)
{
    return (Max(a0, a1) > Min(b0, b1) && Min(a0, a1) < Max(b0, b1)) ? 1 : 0;
}

int BoxesOverlap(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    return (RangesOverlap(x1, x2, x3, x4) > 1 && RangesOverlap(y1, y2, y3, y4)) ? 1 : 0;
}

float *VectorProjection(float x1, float y1, float x2, float y2)
{
    static float ret[2];
    ret[0] = x2 * DotProduct(x1, y1, x2, y2) / (x2 * x2 + y2 * y2);
    ret[1] = y2 * DotProduct(x1, y1, x2, y2) / (x2 * x2 + y2 * y2);
    return ret;
}

float *IntersectionPoint(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    static float ret[3];
    float d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    if (d == 0)
    {
        ret[0] = 0;
        ret[1] = 0;
        ret[2] = 0;
    }
    else
    {
        ret[0] = (CrossProduct(x1, y1, x2, y2) * (x3 - x4) - (x1 - x2) * CrossProduct(x3, y3, x4, y4)) / d;
        ret[1] = (CrossProduct(x1, y1, x2, y2) * (y3 - y4) - (y1 - y2) * CrossProduct(x3, y3, x4, y4)) / d;
        ret[2] = 1;
    }
    return ret;
}

float *ClipViewCone(float x1, float y1, float x2, float y2, float angle)
{
    // Top cone line
    float x3 = 1;
    float tangle = tan(angle);
    float y3 = x3 * -tangle;
    float *p1;
    static float ret[4];

    p1 = IntersectionPoint(x1, y1, x2, y2, 0.0, 0.0, x3, y3);

    if (*(p1 + 2) == 1)
    {
        if (*p1 > 0)
        {
            if (!((x1 < -(y1 / tangle)) && (x2 < -(y2 / tangle))))
            {
                if (x1 < -(y1 / tangle))
                {
                    x1 = *p1;
                    y1 = *(p1 + 1);
                }
                if (x2 < -(y2 / tangle))
                {
                    x2 = *p1;
                    y2 = *(p1 + 1);
                }
            }
            else
            {
                ret[0] = -1;
                ret[1] = -1;
                ret[2] = -1;
                ret[3] = -1;
                return ret;
            }
        }
    }

    // Bottom cone line
    y3 = x3 * tangle;
    float *p2;

    p2 = IntersectionPoint(x1, y1, x2, y2, 0, 0, x3, y3);
    if (*(p2 + 2) == 1)
    {
        if (*p2 > 0)
        {
            if (!((x1 < (y1 / tangle)) && (x2 < (y2 / tangle))))
            {
                if (x1 < (y1 / tangle))
                {
                    x1 = *p2;
                    y1 = *(p2 + 1);
                }
                if (x2 < (y2 / tangle))
                {
                    x2 = *p2;
                    y2 = *(p2 + 1);
                }
            }
            else
            {
                ret[0] = -1;
                ret[1] = -1;
                ret[2] = -1;
                ret[3] = -1;
                return ret;
            }
        }
    }

    ret[0] = x1;
    ret[1] = y1;
    ret[2] = x2;
    ret[3] = y2;
    return ret;
}

static void RenderLine(float x, float y1, float y2, int R, int G, int B)
{
    SDL_SetRenderDrawColor(renderer, R, G, B, 0x00);
    SDL_RenderDrawLine(renderer, (int)x, (int)y1, (int)x, (int)y2);
}

static int DrawLine(float x1, float x2, float yt1, float yt2, float yb1, float yb2, int R, int G, int B)
{
    int drawn = 0;
    float cx1 = 0.0, cx2 = 0.0;
    for (int x = 0; x < numPortals; x++)
    {
        if (portals[x].rendered == 1)
        {
            cx1 = Max(x1, portals[x].x1);
            cx2 = Min(x2, portals[x].x2);
        }
    }
    if (cx1 < cx2)
    {
        if ((yt1 == yt2) && (yb1 == yb2))
        {
            for (int j = cx1; j <= cx2; j++)
            {
                float cyt = 0.0, cyb = 0.0;
                for (int x = 0; x < numPortals; x++)
                {
                    if (portals[x].rendered == 1)
                    {
                        float *p = IntersectionPoint(j, yt1, j, yb1, portals[x].x1, portals[x].yt1, portals[x].x2, portals[x].yt2);
                        cyt = Max(*(p + 1), yt1);
                        p = IntersectionPoint(j, yt1, j, yb1, portals[x].x1, portals[x].yb1, portals[x].x2, portals[x].yb2);
                        cyb = Min(*(p + 1), yb1);
                    }
                }
                if (cyt < cyb)
                {
                    RenderLine(j, cyt, cyb, R, G, B);
                    if (!drawn)
                    {
                        drawn = 1;
                    }
                }
            }
        }
        else
        {
            float mt, mb;
            float bt, bb;
            if (yt1 == yt2)
            {
                mb = (yb2 - yb1) / (cx2 - cx1);
                bb = yb2 - mb * cx2;
                for (int j = cx1; j <= cx2; j++)
                {
                    float cyt = 0.0, cyb = 0.0;
                    for (int x = 0; x < numPortals; x++)
                    {
                        if (portals[x].rendered == 1)
                        {
                            float *p = IntersectionPoint(j, yt1, j, mb * j + bb, portals[x].x1, portals[x].yt1, portals[x].x2, portals[x].yt2);
                            cyt = Max(*(p + 1), yt1);
                            p = IntersectionPoint(j, yt1, j, mb * j + bb, portals[x].x1, portals[x].yb1, portals[x].x2, portals[x].yb2);
                            cyb = Min(*(p + 1), mb * j + bb);
                        }
                    }
                    if (cyt < cyb)
                    {
                        RenderLine(j, cyt, cyb, R, G, B);
                        if (!drawn)
                        {
                            drawn = 1;
                        }
                    }
                }
            }
            else if (yb1 == yb2)
            {
                mt = (yt2 - yt1) / (cx2 - cx1);
                bt = yt2 - mt * cx2;
                for (int j = cx1; j <= cx2; j++)
                {
                    float cyt = 0.0, cyb = 0.0;
                    for (int x = 0; x < numPortals; x++)
                    {
                        if (portals[x].rendered == 1)
                        {
                            float *p = IntersectionPoint(j, mt * j + bt, j, yb1, portals[x].x1, portals[x].yt1, portals[x].x2, portals[x].yt2);
                            cyt = Max(*(p + 1), mt * j + bt);
                            p = IntersectionPoint(j, mt * j + bt, j, yb1, portals[x].x1, portals[x].yb1, portals[x].x2, portals[x].yb2);
                            cyb = Min(*(p + 1), yb1);
                        }
                    }
                    if (cyt < cyb)
                    {
                        RenderLine(j, cyt, cyb, R, G, B);
                        if (!drawn)
                        {
                            drawn = 1;
                        }
                    }
                }
            }
            else
            {
                mt = (yt2 - yt1) / (cx2 - cx1);
                mb = (yb2 - yb1) / (cx2 - cx1);
                bt = yt2 - mt * cx2;
                bb = yb2 - mb * cx2;
                for (int j = cx1; j <= cx2; j++)
                {
                    float cyt = 0.0, cyb = 0.0;
                    for (int x = 0; x < numPortals; x++)
                    {
                        if (portals[x].rendered == 1)
                        {
                            float *p = IntersectionPoint(j, mt * j + bt, j, mb * j + bb, portals[x].x1, portals[x].yt1, portals[x].x2, portals[x].yt2);
                            cyt = Max(*(p + 1), mt * j + bt);
                            p = IntersectionPoint(j, mt * j + bt, j, mb * j + bb, portals[x].x1, portals[x].yb1, portals[x].x2, portals[x].yb2);
                            cyb = Min(*(p + 1), mb * j + bb);
                        }
                    }
                    if (cyt < cyb)
                    {
                        RenderLine(j, cyt, cyb, R, G, B);
                        if (!drawn)
                        {
                            drawn = 1;
                        }
                    }
                }
            }
        }
    }
    return drawn;
}

static void RenderWalls()
{
    // Get current sector
    float ph = PLAYERHEIGHT + sectors[player.sector - 1].floorheight;
    portals = malloc(32 * sizeof(*portals));
    numPortals++;
    portals[numPortals - 1] = (struct ViewWindow){0,
                                                  WIDTH,
                                                  0,
                                                  HEIGHT,
                                                  0,
                                                  HEIGHT,
                                                  player.sector,
                                                  0};
    int allDone = 0;
    while (!allDone && numPortals < 32)
    {
        allDone = 1;
        int tempNum = numPortals;
        for (int x = 0; x < numPortals; x++)
        {
            if (portals[x].rendered == 0)
            {
                // Mark the portal as rendered.

                portals[x].rendered = 1;
                struct sector *sctr = &sectors[portals[x].adjacent - 1];
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
                        float s1, s2, ph, zt1, zb1, zt2, zb2;
                        // Wall X transformation
                        s1 = (500 * *(ptr + 1) / *ptr) + WIDTH / 2;
                        s2 = (500 * *(ptr + 3) / *(ptr + 2)) + WIDTH / 2;
                        zt1 = (-(sctr->ceilingheight - ph) / *ptr) + HEIGHT / 2;
                        zb1 = ((ph - sctr->floorheight) / *ptr) + HEIGHT / 2;
                        zt2 = (-(sctr->ceilingheight - ph) / *(ptr + 2)) + HEIGHT / 2;
                        zb2 = ((ph - sctr->floorheight) / *(ptr + 2)) + HEIGHT / 2;

                        // Wall is facing the player only.
                        if (s1 < s2)
                        {
                            int drawn = 0;
                            if (sctr->lineDef[i].adjacent > -1)
                            {
                                float stepy1 = zb1, stepy2 = zb2, ceily1 = zt1, ceily2 = zt2;
                                if ((sectors[sctr->lineDef[i].adjacent - 1].floorheight - sctr->floorheight) > 0)
                                {
                                    // Create a floor wall for the change in height.
                                    stepy1 = (ph - sectors[sctr->lineDef[i].adjacent - 1].floorheight) / *ptr + HEIGHT / 2;
                                    stepy2 = (ph - sectors[sctr->lineDef[i].adjacent - 1].floorheight) / *(ptr + 2) + HEIGHT / 2;
                                    DrawLine(s1, s2, stepy1, stepy2, zb1, zb2, 0x37, 0xcd, 0xc1);
                                }
                                if ((sectors[sctr->lineDef[i].adjacent - 1].ceilingheight - sctr->ceilingheight) < 0)
                                {
                                    // Create a ceiling for the change in height.
                                    ceily1 = -(sectors[sctr->lineDef[i].adjacent - 1].ceilingheight - ph) / *ptr + HEIGHT / 2;
                                    ceily2 = -(sectors[sctr->lineDef[i].adjacent - 1].ceilingheight - ph) / *(ptr + 2) + HEIGHT / 2;
                                    DrawLine(s1, s2, zt1, zt2, ceily1, ceily2, 0xa7, 0x37, 0xcd);
                                }
                                // Draw the center portal for now
                                drawn = DrawLine(s1, s2, ceily1, ceily2, stepy1, stepy2, 0xDD, 0x00, 0x00);
                                // Save to list of portals to render if drawn.
                                if (drawn)
                                {
                                    portals[++tempNum - 1] = (struct ViewWindow){s1, s2, ceily1, stepy1, ceily2, stepy2, sctr->lineDef[i].adjacent, 0};
                                    allDone = 0;
                                }
                            }
                            else
                            {
                                // Draw a normal wall.
                                drawn = DrawLine(s1, s2, zt1, zt2, zb1, zb2, 0xcc, 0xc5, 0xce);
                            }
                            // Draw roofs and floors.
                            if (drawn)
                            {
                                DrawLine(s1, s2, 0, 0, zt1, zt2, 0x79, 0x91, 0xa9);
                                DrawLine(s1, s2, zb1, zb2, HEIGHT, HEIGHT, 0x9a, 0x79, 0xa9);
                            }
                        }
                    }
                }
            }
        }
        // Only add the number of addtional portals after drawing lines
        numPortals = tempNum;
    }
    // Clear portals
    free(portals);
    numPortals = 0;
}

int main()
{
    int keysPressed[4], close = 0;
    LoadMapFile();
    SDL_Init(SDL_INIT_EVERYTHING);
    window = SDL_CreateWindow("2.5d Engine David", 0, 0, WIDTH, HEIGHT, SDL_WINDOW_FULLSCREEN);
    surface = SDL_GetWindowSurface(window);
    renderer = SDL_CreateRenderer(window, -1, 0);
    // Get Texture from surface
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WIDTH, HEIGHT);
    SDL_ShowCursor(SDL_DISABLE);
    while (close == 0)
    {
        SDL_SetRenderTarget(renderer, texture);
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(renderer);
        RenderWalls();
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        SDL_Event event;
        if (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                close = 1;
            }
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
                close = 1;
                break;
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

        int x, y;
        SDL_GetRelativeMouseState(&x, &y);
        player.angle = x * 0.003f;
        printf("%f\n", player.angle);

        SDL_Delay(10);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    UnloadData();
    return 0;
}