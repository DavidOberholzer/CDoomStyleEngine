# CDOOMStyleEngine v0.1.0

![alt text](https://raw.githubusercontent.com/davidoberholzer/cdoomstyleengine/master/screenshots/screen1.png)
![alt text](https://raw.githubusercontent.com/davidoberholzer/cdoomstyleengine/master/screenshots/screen2.png)
![alt text](https://raw.githubusercontent.com/davidoberholzer/cdoomstyleengine/master/screenshots/screen3.png)
![alt text](https://raw.githubusercontent.com/davidoberholzer/cdoomstyleengine/master/screenshots/screen4.png)

## What is this?

If you have happened to stumble upon this repo, this is my attempt to create a DOOM/Build rendering engine from scratch without any knowledge of computer graphics (or C for that matter). I just wanted to see if I could do it :). 
There are a lot improvements to make that I don't get much time to look at, but hopefully it will get better when I can take a look.

All the textures are my own and I made them with the program `Aseprite`.

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
T - Toggle textures (for information)\
H - Toggle HUD

## Map File Structure

A map file is simply a text file with the contents of each line and sector, along with the player position and all the texture used in the given map.
Note that `index` means the count of the item that appears, ie. if there 3 line declarations in the map file their indexes will be 1, 2 and 3 respectively. 

The structure for each piece is as follows:

TEXTURE
-------
`texture _filename_`

**filename:** The name of the texture file stored in the `/textures` directory.

LINE
----     
`line _x1_ _y1_ _x2_ _y2_ _adjacentsectorvalue_ _wallTextureindex_ _ceilingTextureindex_ _stepTextureindex_`

**x1, y1:** Start point\
**x2, y2:** End point\
**adjacentsectorindex:** The index of the sector connected to this line, thus this wall will act as a portal to this adjacent sector.\
**wallTextureindex:** The index of the texture to be used for the middle wall section (used only if not a portal for now).\
**ceilingTextureindex:** The index of the texture to be used for the roof wall section (used only if a portal).\
**stepTextureindex:** The index of the texture to be used for the step wall section (used only if a portal).

OBJECT
------

`object _x_ _y_ _sector_ _textureindex_`

**x, y** Position\
**sector** Sector the object is in.
**textureindex** 

SECTOR
------
`sector _floorheight_ _ceilingheight_ _lightlevel_ _floorTextureindex_ _ceilingTextureindex_ _sectorlines_`

**floorheight:** The height of the sector floor.\
**ceilingheight:** The height of the sector ceiling.\
**lightlevel:** The light level of the sector (0-1).\
**floorTextureindex:** The index of the floor texture to be used.\
**ceilingTextureindex:** The index of the ceiling texture to be used.\
**sectorlines:** A list of line indexes, seperated by spaces, that belong to this sector. ie. `2 3 4 5 6`.

PLAYER
------
`player _x1_ _y1_ _angle_ _startingsector_`

**x1, y1:** Starting coordinates.\
**angle:** Starting camera angle in relation to the world.\
**startingsector:** The sector the player will start in.

## Textures

Textures are in PCX format. The reason is because it is simple and easy to decode, thus spending a small amount of time on that portion of work.

## Todos && Notes

* Improve floor and ceiling texture mapping speed. Quick implementation added for now to get working.
* Mouse support.
* Collision with objects.
* Improve software rendering with SDL.
* Jumping maybe?
* Weird RANDOM standstill when sometimes entering a different sector.