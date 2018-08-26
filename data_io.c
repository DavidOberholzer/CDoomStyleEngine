// External Includes
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "jpeglib.h"
// Local Includes
#include "data_io.h"
#include "worldmath.h"
#include "structures.h"
#include "graphics.h"

struct texture *textures = NULL;
int numTextures = 0;
struct sector *sectors = NULL;
int numSectors = 0;
struct player player;
float screenLightMap[HEIGHT];

// Custom Error Handler
struct my_error_mgr
{
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr *my_error_ptr;

void my_error_exit(j_common_ptr cinfo)
{
    my_error_ptr myerr = (my_error_ptr)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

void LoadMapFile(char *filename)
{
    char dir[256] = "maps/";
    strcat(dir, filename);
    FILE *data = fopen(dir, "rt");
    if (!data)
    {
        perror("Map did not load!\n");
        exit(1);
    }
    char Buf[256], word[256], tw[64], tc[64], tf[64], *ptr;
    struct line *lines = NULL, ln;
    struct object *objects = NULL, obj;
    struct xy position = {0, 0};
    int n, lineCount = 0, objectCount = 0, sector;
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
                sscanf(ptr += n, "%f%f%f%f%1s%1s%1s%1s%n", &ln.x1, &ln.y1, &ln.x2, &ln.y2, word, tw, tc, tf, &n);
                ln.adjacent = word[0] == 'x' ? -1 : atoi(word);
                ln.wallTexture = tw[0] == 'x' ? -1 : atoi(tw);
                ln.ceilingTexture = tc[0] == 'x' ? -1 : atoi(tc);
                ln.floorTexture = tf[0] == 'x' ? -1 : atoi(tf);
                lines = realloc(lines, ++lineCount * sizeof(*lines));
                lines[lineCount - 1] = ln;
                break;
            // Load in object
            case 'o':
                sscanf(ptr += n, "%f%f%d%64s%n", &position.x, &position.y, &sector, tw, &n);
                LoadObject(tw, position.x, position.y, sector, &obj);
                objects = realloc(objects, ++objectCount * sizeof(*objects));
                objects[objectCount - 1] = obj;
                break;
            // Load Sectors
            case 's':
                sectors = realloc(sectors, ++numSectors * sizeof(*sectors));
                struct sector *sctr = &sectors[numSectors - 1];
                sscanf(ptr += n, "%f%f%f%1s%1s%n", &sctr->floorheight, &sctr->ceilingheight, &sctr->lightlevel, tf, tc, &n);
                sctr->floorTexture = tf[0] == 'x' ? -1 : atoi(tf);
                sctr->ceilingTexture = tc[0] == 'x' ? -1 : atoi(tc);
                int l;
                int *lineLoc = NULL;
                for (l = 0; sscanf(ptr += n, "%32s%n", word, &n) == 1;)
                {
                    lineLoc = realloc(lineLoc, ++l * sizeof(*lineLoc));
                    lineLoc[l - 1] = atoi(word);
                }
                sctr->lineNum = l;
                sctr->lineDef = malloc(l * sizeof(*sctr->lineDef));
                float x1 = -1, x2 = -1, y1 = -1, y2 = -1;
                for (int m = 0; m < l; m++)
                {
                    sctr->lineDef[m] = lines[lineLoc[m] - 1];
                    float xMax = Max(sctr->lineDef[m].x1, sctr->lineDef[m].x2);
                    float xMin = Min(sctr->lineDef[m].x1, sctr->lineDef[m].x2);
                    float yMax = Max(sctr->lineDef[m].y1, sctr->lineDef[m].y2);
                    float yMin = Min(sctr->lineDef[m].y1, sctr->lineDef[m].y2);
                    if (x1 == -1 || xMin < x1)
                    {
                        x1 = xMin;
                    }
                    if (x2 == -1 || xMax > x2)
                    {
                        x2 = xMax;
                    }
                    if (y1 == -1 || yMin < y1)
                    {
                        y1 = yMin;
                    }
                    if (y2 == -1 || yMax > y2)
                    {
                        y2 = yMax;
                    }
                }
                free(lineLoc);
                sctr->t1.x = x1;
                sctr->t1.y = y1;
                sctr->t2.x = x2;
                sctr->t2.y = y1;
                sctr->t3.x = x2;
                sctr->t3.y = y2;
                sctr->t4.x = x1;
                sctr->t4.y = y2;
                int o = 0;
                int *objLoc = NULL;
                for (int i = 0; i < objectCount; i++)
                {
                    if (objects[i].sector == numSectors)
                    {
                        objLoc = realloc(objLoc, ++o * sizeof(*objLoc));
                        objLoc[o - 1] = i;
                    }
                }
                sctr->objectNum = o;
                sctr->objectDef = malloc(o * sizeof(*sctr->objectDef));
                for (int m = 0; m < o; m++)
                {
                    sctr->objectDef[m] = objects[objLoc[m]];
                }
                free(objLoc);
                break;
            // Load player position and starting sector
            case 'p':
                sscanf(ptr += n, "%f%f%f%d%n", &position.x, &position.y, &angle, &sector, &n);
                player = (struct player){{position.x, position.y},
                                         {0, 0},
                                         angle,
                                         sector};
                break;
            // Load in texture
            case 't':
                sscanf(ptr += n, "%64s%n", tw, &n);
                LoadTexture(tw);
                break;
            }
        }
    }
    free(lines);
    free(objects);
}

void LoadObject(char *filename, float x, float y, int sector, struct object *object)
{
    char dir[256] = "objects/";
    strcat(dir, filename);
    object->x = x;
    object->y = y;
    object->sector = sector;
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    FILE *infile;
    JSAMPARRAY buffer;
    int buffer_height = 1;
    int row_stride;

    if ((infile = fopen(dir, "rb")) == NULL)
    {
        printf("Can't open %s\n", filename);
        exit(1);
    }

    // Override error routine
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp(jerr.setjmp_buffer))
    {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        printf("JPEG read error!");
        exit(1);
    }
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);

    (void)jpeg_read_header(&cinfo, TRUE);
    (void)jpeg_start_decompress(&cinfo);

    object->width = cinfo.output_width;
    object->height = cinfo.output_height;
    object->components = cinfo.output_components;

    row_stride = object->width * object->components;
    buffer = (JSAMPARRAY)malloc(sizeof(JSAMPROW) * buffer_height);
    buffer[0] = (JSAMPROW)malloc(sizeof(JSAMPLE) * row_stride);

    object->pixels = malloc(sizeof(unsigned char) * (object->width * object->height * object->components));
    long counter = 0;
    while (cinfo.output_scanline < cinfo.output_height)
    {
        (void)jpeg_read_scanlines(&cinfo, buffer, 1);
        memcpy(object->pixels + counter, buffer[0], row_stride);
        counter += row_stride;
    }

    (void)jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    free(buffer);
    printf("Successfully loaded object %s\n", filename);
}

void LoadTexture(char *filename)
{
    char dir[256] = "textures/";
    strcat(dir, filename);
    textures = realloc(textures, ++numTextures * sizeof(*textures));
    struct texture *texture = &textures[numTextures - 1];
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    FILE *infile;
    JSAMPARRAY buffer;
    int buffer_height = 1;
    int row_stride;

    if ((infile = fopen(dir, "rb")) == NULL)
    {
        printf("Can't open %s\n", filename);
        exit(1);
    }

    // Override error routine
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp(jerr.setjmp_buffer))
    {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        printf("JPEG read error!");
        exit(1);
    }
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);

    (void)jpeg_read_header(&cinfo, TRUE);
    (void)jpeg_start_decompress(&cinfo);

    texture->width = cinfo.output_width;
    texture->height = cinfo.output_height;
    texture->components = cinfo.output_components;

    row_stride = texture->width * texture->components;
    buffer = (JSAMPARRAY)malloc(sizeof(JSAMPROW) * buffer_height);
    buffer[0] = (JSAMPROW)malloc(sizeof(JSAMPLE) * row_stride);

    texture->pixels = malloc(sizeof(unsigned char) * (texture->width * texture->height * texture->components));
    long counter = 0;
    while (cinfo.output_scanline < cinfo.output_height)
    {
        (void)jpeg_read_scanlines(&cinfo, buffer, 1);
        memcpy(texture->pixels + counter, buffer[0], row_stride);
        counter += row_stride;
    }

    (void)jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    free(buffer);
    printf("Successfully loaded texture %s\n", filename);
}

void LoadScreenLightMap()
{
    for (int y = 0; y < HEIGHT; y++)
    {
        float lightlevel = 0;
        if (y > HEIGHT / 6)
        {
            float t = (y - HEIGHT / 6) / (float)(HEIGHT);
            lightlevel = t * 2.5;
        }

        screenLightMap[y] = lightlevel;
    }
}

void UnloadData()
{
    // Free up textures in memory
    free(textures);
    textures = NULL;
    numTextures = 0;
    // Free up lines and sectors in memory
    for (int a = 0; a < numSectors; a++)
    {
        free(sectors[a].lineDef);
    }
    free(sectors);
    sectors = NULL;
    numSectors = 0;
}