compile:
	gcc -o app "main.c" "data_io.c" "game.c" "graphics.c" "worldmath.c" -lSDL2 -lm
pgcompile:
	gcc -o app "main.c" "data_io.c" "game.c" "graphics.c" "worldmath.c" -lSDL2 -lm -pg -ggdb
run:
	./app
car:
	make compile
	make run
profile: pgcompile
	make run