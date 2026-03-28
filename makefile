INC  = $(addprefix -I, $(wildcard ./include/*))
LIB  = $(addprefix -L, $(wildcard ./lib/*)) -lmingw32 -lSDL2main -lSDL2
SRC  = $(wildcard ./src/*.cpp)
ARGS = -pthread -std=c++11

EXEC = ./build/chip8

all :
	g++ $(SRC) $(INC) $(LIB) -o $(EXEC)
