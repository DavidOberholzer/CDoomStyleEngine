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
#define PLAYERHEIGHT 20
#define MOVESPEED 0.05

// SDL Window and Surface.

static SDL_Window *window = NULL;
static SDL_Surface *surface = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;

static int printed = 0;

// Vertex
struct xy
{
    float x, y;
};

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
    FILE *data = fopen("maps/theoverlook.txt", "rt");
    if (!data)
    {
        perror("Map did not load!\n");
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

float Max(float x, float y)
{
    return x >= y ? x : y;
}

float Min(float x, float y)
{
    return x <= y ? x : y;
}

int Clamp(int top, int bottom, float num)
{
    return Max(Min(top, num), bottom);
}

int RangesOverlap(float a0, float a1, float b0, float b1)
{
    return ((Max(a0, a1) >= Min(b0, b1)) && (Min(a0, a1) <= Max(b0, b1)));
}

int BoxesOverlap(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    return (RangesOverlap(x1, x2, x3, x4) && RangesOverlap(y1, y2, y3, y4));
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

static void RenderLine(int x, int y1, int y2, int R, int G, int B, float distance)
{
    float light_level = distance < 4 ? 1 : distance > 10 ? 0.4 : 4 / distance;
    SDL_SetRenderDrawColor(renderer, R * light_level, G * light_level, B * light_level, 0x00);
    SDL_RenderDrawLine(renderer, x, y1, x, y2);
}

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
    float ph = PLAYERHEIGHT + sectors[player.sector - 1].floorheight;
    do
    {
        current += 1;
        struct portal *currentPortal = queue + current;
        struct sector *sctr = &sectors[currentPortal->sectorNo - 1];
        // if (renderedSectors[currentPortal->sectorNo] & 0x21)
        //     continue;
        // ++renderedSectors[currentPortal->sectorNo];
        for (int i = 0; i < sctr->lineNum; i++)
        {
            // First Vector rotation of the lines around the player.
            float rotx1, roty1, rotx2, roty2;
            rotx1 = (sctr->lineDef[i].x1 - player.position.x) * cos(player.angle) + (sctr->lineDef[i].y1 - player.position.y) * sin(player.angle);
            roty1 = (sctr->lineDef[i].y1 - player.position.y) * cos(player.angle) - (sctr->lineDef[i].x1 - player.position.x) * sin(player.angle);
            rotx2 = (sctr->lineDef[i].x2 - player.position.x) * cos(player.angle) + (sctr->lineDef[i].y2 - player.position.y) * sin(player.angle);
            roty2 = (sctr->lineDef[i].y2 - player.position.y) * cos(player.angle) - (sctr->lineDef[i].x2 - player.position.x) * sin(player.angle);

            float points[4] = {
                sctr->lineDef[i].x1 - player.position.x,
                sctr->lineDef[i].y1 - player.position.y,
                sctr->lineDef[i].x2 - player.position.x,
                sctr->lineDef[i].y2 - player.position.y};

            // Clip line to the view cone.
            float *ptr;
            ptr = ClipViewCone(rotx1, roty1, rotx2, roty2, VIEWANGLE);

            if (*ptr > 0 && *(ptr + 2) > 0)
            {
                int s1, s2, zt1, zb1, zt2, zb2, stepy1, stepy2, ceily1, ceily2;
                // Wall X transformation
                s1 = (500 * *(ptr + 1) / *ptr) + WIDTH / 2;
                s2 = (500 * *(ptr + 3) / *(ptr + 2)) + WIDTH / 2;
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
                        int yt = Clamp(yBottomLimit[x], yTopLimit[x], zt1 * (1 - t) + zt2 * t);
                        int yb = Clamp(yBottomLimit[x], yTopLimit[x], zb1 * (1 - t) + zb2 * t);
                        float dx = *ptr * (1 - t) + *(ptr + 2) * t;
                        float dy = *(ptr + 1) * (1 - t) + *(ptr + 3) * t;
                        float distance = fabs(dx) + fabs(dy);
                        // Draw roofs and floors.
                        RenderLine(x, yTopLimit[x], yt, 0x79 * sctr->lightlevel, 0x91 * sctr->lightlevel, 0xa9 * sctr->lightlevel, distance);
                        RenderLine(x, yb, yBottomLimit[x], 0x9a * sctr->lightlevel, 0x79 * sctr->lightlevel, 0xa9 * sctr->lightlevel, distance);

                        if (sctr->lineDef[i].adjacent > -1)
                        {
                            if (sectors[sctr->lineDef[i].adjacent - 1].floorheight > sctr->floorheight)
                            {
                                // Create a floor wall for the change in height.
                                int stepY = Clamp(yBottomLimit[x], yTopLimit[x], stepy1 * (1 - t) + stepy2 * t);
                                RenderLine(x, stepY, yb, 0x37 * sctr->lightlevel, 0xcd * sctr->lightlevel, 0xc1 * sctr->lightlevel, distance);
                                yBottomLimit[x] = stepY;
                            }
                            else
                            {
                                yBottomLimit[x] = yb;
                            }

                            if (sectors[sctr->lineDef[i].adjacent - 1].ceilingheight < sctr->ceilingheight)
                            {
                                // Create a ceiling for the change in height.
                                int ceilY = Clamp(yBottomLimit[x], yTopLimit[x], ceily1 * (1 - t) + ceily2 * t);
                                RenderLine(x, yt, ceilY, 0xa7 * sctr->lightlevel, 0x37 * sctr->lightlevel, 0xcd * sctr->lightlevel, distance);
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
                            RenderLine(x, yt, yb, 0xcc * sctr->lightlevel, 0xc5 * sctr->lightlevel, 0xce * sctr->lightlevel, distance);
                        }
                    }
                    // Load in next in next portal if adjacent sector found.
                    if (sctr->lineDef[i].adjacent > -1 && end != maxSectors - 1)
                    {
                        if (i == 4)
                        {
                            printf("%d\n", current);
                        }
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

int main()
{
    int keysPressed[6], close = 0;
    LoadMapFile();
    SDL_Init(SDL_INIT_EVERYTHING);
    window = SDL_CreateWindow("2.5d Engine David", 0, 0, WIDTH, HEIGHT, SDL_WINDOW_MAXIMIZED);
    surface = SDL_GetWindowSurface(window);
    renderer = SDL_CreateRenderer(window, -1, 0);
    // Get Texture from surface
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WIDTH, HEIGHT);
    SDL_ShowCursor(SDL_DISABLE);
    while (!close)
    {
        SDL_SetRenderTarget(renderer, texture);
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(renderer);
        RenderWalls();
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
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
        }

        player.angle -= keysPressed[4] ? 0.03f : 0;
        player.angle += keysPressed[5] ? 0.03f : 0;

        SDL_Delay(10);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    UnloadData();
    return 0;
}