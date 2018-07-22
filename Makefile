compile:
	gcc -o app "main.c" "data_io.c" "game.c" "graphics.c" "worldmath.c" -lSDL2 -lm
run:
	./app
car:
	make compile
	make run