#include "chip8.h"

unsigned char FontTable[] = {

    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F

};

Chip8::Chip8(){

    unsigned char i = 0x0;
    for(unsigned char pos = 0x050 ; pos <= 0x09F ; ++pos){
        RAM[pos] = FontTable[i];
        ++i;
    }

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0){
        fprintf(stderr , "Couldn't initialize SDL library : %s\n" , SDL_GetError());
    }

    this->off.r = 0x0 ; this->off.g = 0x0 ; this->off.b = 0x0;

    this->on.r = 0xFF ; this->on.g = 0xFF ; this->on.b = 0xFF;

    for(int i = 0 ; i < 64 ; ++i){
        for(int j = 0 ; j < 32 ; ++j){
            this->display[i][j] = false;
        }
    }

    atexit(SDL_Quit);

};

Chip8::~Chip8(){

    SDL_Quit();

};

int Chip8::initialize(){

    window = SDL_CreateWindow("Chip-8 Emulator" , SDL_WINDOWPOS_CENTERED , SDL_WINDOWPOS_CENTERED , 640 , 320 , 0);
    if(!window){
        fprintf(stderr , "Couldn't open SDL window... Error Message : %s\n" , SDL_GetError());
        return -1;
    }

    renderer = SDL_CreateRenderer(window , -1 , SDL_RENDERER_ACCELERATED);
    if(!renderer){
        fprintf(stderr , "Couldn't create SDL renderer... Error Message : %s\n" , SDL_GetError());
        return -1;
    }

    SDL_RenderSetScale(this->renderer , 10 , 10);

    this->loop();

    return 0;

};

int Chip8::load_rom(const char *rom_name){

    std::fstream rom(rom_name , std::ios_base::in);

    unsigned int RAM_pos = 0x200;

    if(rom.fail()){
        fprintf(stderr , "Couldn't open ROM file...\n");
        return -1;
    }
    
    while(!rom.eof()){
        this->RAM[RAM_pos] = (unsigned char)rom.get();
        ++RAM_pos;
    }
    rom.close();

    this->pc = 0x200;
    
    return 0;

};

int Chip8::clear_screen(){

    SDL_SetRenderDrawColor(this->renderer , 0x0 , 0x0 , 0x0 , 0xFF);
    SDL_RenderClear(this->renderer);
    SDL_RenderPresent(this->renderer);

    for(int i = 0 ; i < 64 ; ++i){
        for(int j = 0 ; j < 32 ; ++j){
            this->display[i][j] = false;
        }
    }

    return 0;

};

bool Chip8::is_pixel_on(const SDL_Color *color){
    // DEBUGGING
    fprintf(stdout , "Pixel Color : R = %x ; G = %x ; B = %x\n" , color->r , color->g , color->b);
    if((color->r == 0x0) && (color->g == 0x0) && (color->b == 0x0))
        return false;
    return true;

};

int Chip8::colorPixel(unsigned char x_coord , unsigned char y_coord , bool pixel_on){

    switch (pixel_on){
        case true :
            if(display[x_coord][y_coord]){
                
                SDL_SetRenderDrawColor(this->renderer , 0x0 , 0x0 , 0x0 , 0xFF);
                SDL_RenderDrawPoint(this->renderer , x_coord , y_coord);
                display[x_coord][y_coord] = false;
                reg[0xF] = 1;
                break;
            }
            else{

                SDL_SetRenderDrawColor(this->renderer , 0xFF , 0xFF , 0xFF , 0xFF);
                SDL_RenderDrawPoint(this->renderer , x_coord , y_coord);
                display[x_coord][y_coord] = true;
                break;
            }

        case false :
            break;
    }

    return 0;

};

// OBS : Pela implementação, os sprites "dão a volta" quando atingem a borda. Porém, a instrução original não permite isso...
// Necessário reescrever
int Chip8::draw(unsigned char reg_x_coord , unsigned char reg_y_coord , unsigned char rows){

    reg[0xF] = 0;
    bool sprite[rows][0x8];

    for(unsigned char i = 0x0 ; i < rows ; ++i){

        // DEBUGGING
        //fprintf(stdout , "SPRITE BYTE : %x\n" , RAM[index + i]);

        for(unsigned char j = 0x0 ; j < 0x8 ; ++j){

            // DEBUGGING
            ///fprintf(stdout , "reg[index + i] = %x\n" , reg[index + i]);

            unsigned char sprite_byte = RAM[index + i];

            // Sprites Invertidos? Big Endian e Little Endian? Investigar o pq
            //sprite[i][j] = ((sprite_byte & (0x1 << j)) >> j);
            sprite[i][j] = (sprite_byte & (0x80 >> j));

            // DEBUGGING
            //std::cout << sprite[i][j];
        }

        // DEBUGGING
        //std::cout << std::endl;
    }

    for(unsigned char i = 0x0 ; i < rows ; ++i){
        if(((reg[reg_y_coord] + i) & 0x1F) >= 32)
            break;
        for(unsigned char j = 0x0 ; j < 0x8 ; ++j){ 
            if(((reg[reg_x_coord] + j) & 0x3F) >= 64)
               break;

            colorPixel(((reg[reg_x_coord] + j) & 0x3F) , ((reg[reg_y_coord] + i) & 0x1F) , sprite[i][j]);
        }
    }
    SDL_RenderPresent(this->renderer);
    return 0;

};

int Chip8::loop(){

    while(true){

        if((pc >= (unsigned int)0x200) && (pc <= (unsigned int)0x1000)){

            // FETCH
            unsigned int instruction = 0x0;

            instruction = RAM[pc];
            instruction = (instruction << 8) | RAM[pc + 1];

            pc += (unsigned int)0x2;

            // DECODE
            unsigned char opcode = (instruction & 0xF000) >> 12;

            unsigned char X = (instruction & 0x0F00) >> 8;

            unsigned char Y = (instruction & 0x00F0) >> 4;

            unsigned char N = (instruction & 0x000F);

            unsigned char NN = (instruction & 0x00FF);

            unsigned int NNN = (instruction & 0x0FFF);

            switch(opcode){
                case (unsigned char)0x0 :
                    switch(instruction){
                        // Clear Screen
                        case 0x00E0 :
                            clear_screen();
                            break;
                        
                        // Return Execution from Subroutine
                        case 0x00EE :
                            pc = this->stack.top();
                            this->stack.pop();
                            break;

                        default :
                            break;
                    };
                    break;
                
                // Jump to NNN
                case (unsigned char)0x1 :
                    pc = NNN;
                    break;

                // Execute Subroutine
                case (unsigned char)0x2 :
                    this->stack.push(pc);
                    pc = NNN;
                    break;

                // Skip Instruction if reg[X] == NN
                case (unsigned char)0x3 :
                    if(reg[X] == NN)
                        pc += 2;
                    break;

                // Skip Instruction if reg[x] != NN
                case (unsigned char)0x4 :
                    if(reg[X] != NN)
                        pc += 2;
                    break;

                // Skip Instruction if reg[x] == reg[Y]
                case (unsigned char)0x5 :
                    if(reg[X] == reg[Y])
                        pc += 2;
                    break;

                // Set VX to NN
                case (unsigned char)0x6 :
                    reg[X] = NN;
                    break;

                // Add NN to VX
                case (unsigned char)0x7 :
                    reg[X] += NN;
                    break;

                case (unsigned char)0x8 :
                    switch(N){
                        case 0x0 :
                            reg[X] = reg[Y];
                            break;
                        
                        case 0x1 :
                            reg[X] = (reg[X] | reg[Y]);
                            break;

                        case 0x2 :
                            reg[X] = (reg[X] & reg[Y]);
                            break;
                        
                        case 0x3 :
                            reg[X] = (reg[X] ^ reg[Y]);
                            break;

                        case 0x4 :
                            // Add Overflow
                            if((0xFF - reg[Y]) < reg[X]){
                                reg[0xF] = 1;
                            }
                            else{
                                reg[0xF] = 0;
                            }
                            reg[X] += reg[Y];
                            break;

                        case 0x5 :
                            reg[0xF] = 0;
                            if(reg[X] > reg[Y])
                                reg[0xF] = 1;
                            reg[X] -= reg[Y];
                            break;

                        case 0x6 : {
                            bool out_bit = (reg[X] & 0x1);
                            reg[X] = (reg[X] >> 1);
                            if(out_bit){
                                reg[0xF] = 1;
                            }
                            else{
                                reg[0xF] = 0;
                            }
                            break;
                        }

                        case 0x7 :
                            reg[0xF] = 0;
                            if(reg[X] < reg[Y])
                                reg[0xF] = 1;
                            reg[X] = (reg[Y] - reg[X]);
                            break;

                        case 0xE : {
                            bool out_bit = (reg[X] & 0x80);
                            reg[X] = (reg[X] << 1);
                            if(out_bit){
                                reg[0xF] = 1;
                            }
                            else{
                                reg[0xF] = 0;
                            }
                            break;
                        }
                    }
                    break;

                // Skip Instruction if reg[x] != reg[Y]
                case (unsigned char)0x9 :
                    if(reg[X] != reg[Y])
                        pc += 2;
                    break;

                // Change index value to NNN
                case (unsigned char)0xA :
                    index = NNN;
                    break;

                case (unsigned char)0xB :
                    this->stack.push(pc);
                    pc = (NNN + reg[0x0]);
                    break;

                // Draw on Display
                case (unsigned char)0xD :
                    draw(X , Y , N);
                    break;

                default :
                    fprintf(stderr , "Not an instruction\n");
                    break;
            }
        }
        else{

            fprintf(stderr , "PC points to outside reserved program memory...\n");
            return -1;

        }

        SDL_Delay(1000/700);

    };

    return 0;

};
