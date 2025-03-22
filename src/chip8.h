#ifndef CHIP8_H
#define CHIP8_H

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <stack>
#include <ctime>
#include <SDL.h>

class Chip8{
    public:
        Chip8();
        ~Chip8();
        int initialize();
        int load_rom(const char *rom_name);
    private:
        // RAM : 0x050 até 0x09F -> Font Data ; 0x200 até 0x1000 -> Memória de Programa
        unsigned char RAM[4096];
        // STACK
        std::stack<unsigned int> stack;
        unsigned int index = 0x0;
        unsigned int pc = 0x0;
        unsigned char delay_t = 0xFF;
        unsigned char sound_t = 0xFF;
        unsigned char count = 0x0;
        unsigned char reg[16];
        bool display[64][32];
        SDL_Window *window;
        SDL_Renderer *renderer;
        SDL_Color on;
        SDL_Color off;
        SDL_Event chip_event;
        int clear_screen();
        bool is_pixel_on(const SDL_Color *color);
        int colorPixel(unsigned char x_coord , unsigned char y_coord , bool pixel_on);
        int draw(unsigned char x_coord , unsigned char y_coord , unsigned char rows);
        void setTimers();
        unsigned char fontAddr(unsigned char font_char);
        int loop();
};

#endif // ifndef CHIP8_H