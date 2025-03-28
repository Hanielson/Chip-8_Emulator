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

// Chip_8 Key (0x0 - 0xF) is used as index to access respective QWERTY key scancodes
SDL_Scancode ScancodeTable[] = {
    SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3,
    SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A,
    SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C,
    SDL_SCANCODE_4, SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V
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
    // Necessary to open file as binary -> if not explicit, the function opens it as text
    std::fstream rom(rom_name , std::ios_base::in | std::ios_base::binary);
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

int Chip8::colorPixel(unsigned char x_coord , unsigned char y_coord , bool pixel_on){
    if (pixel_on){
        if(display[x_coord][y_coord]){
            SDL_SetRenderDrawColor(this->renderer , 0x0 , 0x0 , 0x0 , 0xFF);
            SDL_RenderDrawPoint(this->renderer , x_coord , y_coord);
            display[x_coord][y_coord] = false;
            reg[0xF] = 1;
        }
        else{
            SDL_SetRenderDrawColor(this->renderer , 0xFF , 0xFF , 0xFF , 0xFF);
            SDL_RenderDrawPoint(this->renderer , x_coord , y_coord);
            display[x_coord][y_coord] = true;
        }
    }
    return 0;
};

// OBS : Pela implementação, os sprites "dão a volta" quando atingem a borda. Porém, a instrução original não permite isso...
// Necessário reescrever
int Chip8::draw(unsigned char reg_x_coord , unsigned char reg_y_coord , unsigned char rows){
    reg[0xF] = 0;
    bool sprite[rows][0x8];
    for(unsigned char i = 0x0 ; i < rows ; ++i){
        for(unsigned char j = 0x0 ; j < 0x8 ; ++j){
            unsigned char sprite_byte = RAM[index + i];
            sprite[i][j] = (sprite_byte & (0x80 >> j));
        }
    }
    for(unsigned char i = 0x0 ; i < rows ; ++i){
        if ((i > 0) && ((reg[reg_y_coord] + i) >= 32))
            break;
        for(unsigned char j = 0x0 ; j < 0x8 ; ++j){ 
            if ((j > 0) && ((reg[reg_x_coord] + j) >= 64))
               break;
            colorPixel(((reg[reg_x_coord] + j) & 0x3F) , ((reg[reg_y_coord] + i) & 0x1F) , sprite[i][j]);
        }
    }
    SDL_RenderPresent(this->renderer);
    return 0;
};

void Chip8::setTimers(){
    // 720 instructions per second -> decrement 60 times per second -> decrement every 12 instructions
    if(count == 0xC){
        if(delay_t == 0x0){
            delay_t = 0xFF;
        }
        else{
            delay_t -= 0x1;
        }
        if(sound_t == 0x0){
            sound_t = 0xFF;
        }
        else{
            sound_t -= 0x1;
        }
        count = 0x0;
        return;
    }
    else{
        count += 0x1;
        return;
    }
};

unsigned char Chip8::fontAddr(unsigned char font_char){
    switch(font_char){
        case 0x0 : return 0x50;
        case 0x1 : return 0x55;
        case 0x2 : return 0x5A;
        case 0x3 : return 0x5F;
        case 0x4 : return 0x64;
        case 0x5 : return 0x69;
        case 0x6 : return 0x6E;
        case 0x7 : return 0x73;
        case 0x8 : return 0x78;
        case 0x9 : return 0x7D;
        case 0xA : return 0x82;
        case 0xB : return 0x87;
        case 0xC : return 0x8C;
        case 0xD : return 0x91;
        case 0xE : return 0x96;
        case 0xF : return 0x9B;
    }
    return 0x0;
};

int Chip8::loop(){
    while(true){
        // TESTE : ESTAREI IMPLEMENTANDO UMA ÚNICA AVALIAÇÃO DE SDL_EVENT POR CICLO DE INSTRUÇÃO -> SUJEITO À MODIFICAÇÃO
        SDL_PollEvent(&chip_event);
        if(chip_event.type == SDL_QUIT){
            SDL_Quit();
            fprintf(stdout , "Closing application...\n");
            return 0;
        }
        if((pc >= (unsigned int)0x200) && (pc <= (unsigned int)0x1000)){
            // FETCH
            unsigned int instruction = 0x0;
            instruction = RAM[pc];
            instruction = (instruction << 8) | RAM[pc + 1];
            // DEBUGGING
            fprintf(stdout , "PC : %x ; Instruction : %x ; RAM[pc] : %x ; RAM[pc + 1] : %x\n" , pc , instruction , RAM[pc] , RAM[pc + 1]);
            pc += (unsigned int)0x2;
            // DECODE
            unsigned char opcode = (instruction & 0xF000) >> 12;
            unsigned char X      = (instruction & 0x0F00) >> 8;
            unsigned char Y      = (instruction & 0x00F0) >> 4;
            unsigned char N      = (instruction & 0x000F);
            unsigned char NN     = (instruction & 0x00FF);
            unsigned int NNN     = (instruction & 0x0FFF);
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
                        case (unsigned char)0x0 :
                            reg[X] = reg[Y];
                            break;
                        case (unsigned char)0x1 :
                            //reg[0xF] = 0;
                            reg[X] = (reg[X] | reg[Y]);
                            break;
                        case (unsigned char)0x2 :
                            //reg[0xF] = 0;
                            reg[X] = (reg[X] & reg[Y]);
                            break;
                        case (unsigned char)0x3 :
                            //reg[0xF] = 0;
                            reg[X] = (reg[X] ^ reg[Y]);
                            break;
                        case (unsigned char)0x4 : {
                            unsigned char reg_X_old_value = reg[X];
                            reg[X] += reg[Y];
                            // Add Overflow
                            if((0xFF - reg[Y]) < reg_X_old_value){
                                reg[0xF] = 0x1;
                            }
                            else{
                                reg[0xF] = 0x0;
                            }
                            break;
                        }
                        case (unsigned char)0x5 : {
                            unsigned char reg_X_old_value = reg[X];
                            reg[X] -= reg[Y];
                            if(reg_X_old_value >= reg[Y]){
                                reg[0xF] = 0x1;
                            }
                            else{
                                reg[0xF] = 0x0;
                            }
                            break;
                        }
                        case (unsigned char)0x6 : {
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
                        case (unsigned char)0x7 : {
                            unsigned char reg_X_old_value = reg[X];
                            reg[X] = (reg[Y] - reg[X]);
                            if(reg_X_old_value <= reg[Y]){
                                reg[0xF] = 1;
                            }
                            else{
                                reg[0xF] = 0;
                            }
                            break;
                        }
                        case (unsigned char)0xE : {
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
                // Jump to NNN + V0
                case (unsigned char)0xB :
                    this->stack.push(pc);
                    pc = (NNN + reg[0x0]);
                    break;
                // Generate Random Number, AND's it with NN and then store it at VX
                case (unsigned char)0xC : {
                    srand(time(NULL));
                    unsigned char random_value = (unsigned char)(rand() % 256);
                    reg[X] = (random_value & NN);
                    break;
                }   
                // Draw on Display
                case (unsigned char)0xD :
                    draw(X , Y , N);
                    break;
                // KEY INPUTS AFFECT NEXT INSTRUCTION EXECUTION
                case (unsigned char)0xE :
                    switch(NN){
                        // Jump Instruction if Key in VX is pressed
                        case (unsigned char)0x9E : {
                            if((reg[X] < 0x0) || (reg[X] > 0xF)){
                                fprintf(stderr , "Key stored at VX register has no correspondent...\n");
                                // Break from if
                                break;
                            }
                            const Uint8 *keyboard_state = SDL_GetKeyboardState(nullptr);
                            SDL_Scancode key_pressed = ScancodeTable[reg[X]];
                            if(keyboard_state[key_pressed]){
                                pc += 2;
                            }
                            break;
                        }
                        // Jump Instruction if Key in VX is NOT pressed
                        case (unsigned char)0xA1 : {
                            if((reg[X] < 0x0) || (reg[X] > 0xF)){
                                fprintf(stderr , "Key stored at VX register has no correspondent...\n");
                                // Break from if
                                break;
                            }
                            const Uint8 *keyboard_state = SDL_GetKeyboardState(nullptr);
                            SDL_Scancode key_pressed = ScancodeTable[reg[X]];
                            if(!keyboard_state[key_pressed]){
                                pc += 2;
                            }
                            break;
                        }
                    }
                    break;
                case (unsigned char)0xF :
                    switch(NN){
                        // Set VX to value of delay_timer
                        case (unsigned char)0x07 :
                            reg[X] = delay_t;
                            break;
                        // Waits for key input and stores it at VX
                        case (unsigned char)0x0A :{   
                            if(chip_event.type == SDL_KEYDOWN){
                                unsigned char key_pressed = 0x0;
                                for(; key_pressed <= 0xF ; key_pressed += 0x1){
                                    if(ScancodeTable[key_pressed] == chip_event.key.keysym.scancode){
                                        break;
                                    }
                                }
                                reg[X] = key_pressed;
                                // increment PC to compensate it's later decrement -> in the end, PC continues from where it was
                                pc += (unsigned int)0x2;
                            }
                            // Maintain the wait key status until a key is pressed
                            pc -= (unsigned int)0x2;
                            break;
                        }
                        // Set delay timer to value of VX
                        case (unsigned char)0x15 :
                            delay_t = reg[X];
                            break;
                        // Set sound timer to value of VX
                        case (unsigned char)0x18 :
                            sound_t = reg[X];
                            break;
                        // Index register will have VX value added to it
                        case (unsigned char)0x1E :
                            index += reg[X];
                            if(index >= 0x1000)
                                reg[0xF] = 1;
                            break;
                        // Sets Index to Adress of Font at VX
                        case (unsigned char)0x29 : {
                            unsigned char font_char = (reg[X] & 0x0F);
                            index = fontAddr(font_char);
                            break;
                        }
                        // Binary-Coded Decimal Conversion
                        case (unsigned char)0x33 : {
                            // Returns only the Hundreds Quantity, as there will be no floating-point precision
                            unsigned char hundreds = (reg[X] / 0x64);
                            // (reg[X] - (hundreds * 0x64)) removes the Hundreds Unit, then we perform a division by 10 (0xA), which returns only
                            // the tens unit, without floating point precision
                            unsigned char tens = ((reg[X] - (hundreds * 0x64))/0xA);
                            // similar algorithm goes for the units
                            unsigned char unit = (reg[X] - (hundreds * 0x64) - (tens * 0xA));
                            RAM[index] = hundreds;
                            RAM[(index + (unsigned int)0x1)] = tens;
                            RAM[(index + (unsigned int)0x2)] = unit;
                            break;
                        }
                        // Store values from V0 to VX in successive memory locations, starting from index
                        case (unsigned char)0x55 : {
                            unsigned char store_index = 0x0;
                            for(; store_index <= X ; store_index += 0x1){
                                //index += store_index;
                                RAM[index + ((unsigned int)store_index)] = reg[store_index];
                            }
                            break;
                        }
                        // Store value from memory, starting from "index" address, to registers V0 to VX
                        case (unsigned char)0x65 : {
                            unsigned char store_index = 0x0;
                            for(; store_index <= X ; store_index += 0x1){
                                //index += store_index;
                                reg[store_index] = RAM[index + ((unsigned int)store_index)];
                            }
                            break;
                        }
                    }
                    break;
                default :
                    fprintf(stderr , "Not an instruction\n");
                    break;
            }
        }
        else{     
            fprintf(stderr , "PC points to outside reserved program memory...\n");
            while(true){
                SDL_WaitEvent(&chip_event);
                if(chip_event.type == SDL_QUIT){
                    SDL_Quit();
                }
            }
            return -1;
        }
        // We take this time as the base for calculating the timers decrement
        this->setTimers();
        SDL_Delay(1000/720);
    };
    return 0;
};
