#define SDL_MAIN_HANDLE
#include "chip8.h"
#include <stdio.h>

int main(int argc , char **argv){

    if(argc != 2){
        fprintf(stderr , "Select ROM file to open...\n");
        return -1;
    }

    char *rom_name = argv[1];

    Chip8 console;

    console.load_rom(rom_name);

    console.initialize();

    return 0;
    
};
