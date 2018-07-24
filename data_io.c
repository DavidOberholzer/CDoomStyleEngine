// External Includes
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "jpeglib.h"
// Local Includes
#include "data_io.h"
#include "structures.h"

struct texture *textures = NULL;
int numTextures = 0;
struct sector *sectors = NULL;
int numSectors = 0;
struct player player;

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

void LoadMapFile()
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

void LoadTexture(char *filename)
{
    textures = realloc(textures, ++numTextures * sizeof(*textures));
    struct texture *texture = &textures[numTextures - 1];
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    FILE *infile;
    JSAMPARRAY buffer;
    int buffer_height = 1;
    int row_stride;

    if ((infile = fopen(filename, "rb")) == NULL)
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