all :

g++	./src/main.cpp ./src/chip8.cpp -I ./SDL2/include/SDL2 -L ./SDL2/lib -o chip8 -lmingw32 -lSDL2main -lSDL2
