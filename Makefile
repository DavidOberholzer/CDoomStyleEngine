compile:
	gcc -o game "main.c" "data_io.c" "worldmath.c" -lSDL2 -lm
run:
	./game
car:
	make compile
	make run