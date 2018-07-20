compile:
	gcc -o game "main.c" -lSDL2 -lm
run:
	./game
car:
	make compile
	make run