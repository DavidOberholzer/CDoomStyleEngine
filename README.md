# CDOOMStyleEngine

## Running it!

Make sure you have a gcc compiler and SDL2 libaries installed on your system.

You can compile main.c yourself or just run the make command in linux `make compile` then `make run` to run it!

## Controls

W - Forwards\
S - Backwards\
A - Strafe Left\
D - Strafe Right\
Q - Rotate Anti-clockwise\
E - Rotate Clockwise\
T - Toggle textures (for information)

## Map File Structure

A map file is simply a text file with the contents of each line and sector, along with the player position and all the texture used in the given map.
Note that `index` means the count of the item that appears, ie. if there 3 line declarations in the map file their indexes will be 1, 2 and 3 respectively. 

The structure for each piece is as follows:

LINE
----     
`line _x1_ _y1_ _x2_ _y2_ _adjacentsectorvalue_ _wallTextureindex_ _ceilingTextureindex_ _stepTextureindex_`

**x1, y1:** Start point\
**x2, y2:** End point\
**adjacentsectorindex:** The index of the sector connected to this line, thus this wall will act as a portal to this adjacent sector.\
**wallTextureindex:** The index of the texture to be used for the middle wall section (used only if not a portal for now).\
**ceilingTextureindex:** The index of the texture to be used for the roof wall section (used only if a portal).\
**stepTextureindex:** The index of the texture to be used for the step wall section (used only if a portal).\

SECTOR
------
`sector _floorheight_ _ceilingheight_ _lightlevel_ _sectorlines_`

**floorheight:** The height of the sector floor.\
**ceilingheight:** The height of the sector ceiling.\
**lightlevel:** The light level of the sector (0-1).\
**sectorlines:** A list of line indexes, seperated by spaces, that belong to this sector. ie. `2 3 4 5 6`.\

PLAYER
------
`player _x1_ _y1_ _angle_ _startingsector_`

**x1, y1:** Starting coordinates.\
**angle:** Starting camera angle in relation to the world.\
**startingsector:** The sector the player will start in.\

TEXTURE
-------
`texture _filename_`

**filename:** The name of the texture file stored in the `/textures` directory.\

## Textures

Textures are currently loaded in using `jpeglib`. Just used it for ease for now. 


## Todos && Notes

* Improve speed with textures present (seems slow to draw each pixel rather than lines.)
* Improve head bob reverting to standard height.
* Mouse support.