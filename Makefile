compile:
	gcc -o app "main.c" "data_io.c" "game.c" "graphics.c" "worldmath.c" -lSDL2 -lm -ljpeg
pgcompile:
	gcc -o app "main.c" "data_io.c" "game.c" "graphics.c" "worldmath.c" -lSDL2 -lm -ljpeg -pg -ggdb
run:
	./app
car:
	make compile
	make run
profile: pgcompile
	make run