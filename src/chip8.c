#include <SDL3/SDL_render.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_log.h>

#include "../include/chip8.h"
#include "../include/functions.h"


//CHIP-8 specifications
unsigned short opcode; //the operation code of the instruction
unsigned char memory[MEMORY]; //the total memory of the chip-8
unsigned char V[16]; //all the general purpose registers

unsigned short I; //special register I for memory addresses
unsigned short pc; //Program counter


unsigned char gpx[SCREEN_WIDTH * SCREEN_WIDTH]; //pixels of the screen. Total pixels in the array: 2048

unsigned char delay_timer;
unsigned char sound_timer;

//STACK
unsigned short stack[16];
unsigned short sp; //stack pointer

unsigned char keyPad[16];
//I have no idea how to implement the pad, typeshit

//Flag for indicating when the screen needs to be updated
bool drawFlag = false;

unsigned char chip8_fontset[80] =
{ 
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

//0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
//0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
//0x200-0xFFF - Program ROM and work RAM


extern Config globalConfig;
extern char fileName[20];

void initialize(){
    initializeMemory();
    initRegisters();
}

void initializeMemory(){
    opcode = 0;
    for(int i = 0; i < 80; i++){
        memory[i] = chip8_fontset[i];
    }

    for(int i = 80; i < MEMORY; i++){
        memory[i] = 0x00;
    }

    for(int i = 0; i < 16; i++){
        stack[i] = 0;
    }
    pc = 0x200;
}

void initRegisters(){
    for(int i = 0; i < 16; i++){
        V[i] = 0;
    }
    I = 0;
}

int loadProgram(const char *fileName){
    FILE *ptr = fopen(fileName, "rb");
    if(ptr == NULL){
        printf("Error while trying to open the loaded file!\n");
        return -1;
    }
    fseek(ptr, 0, SEEK_END); //Go to the end of the file

    long fileSize = ftell(ptr); //Use ftell to get the size of said file

    rewind(ptr); //Go back to the beggining of the file using rewind, first time i have ever heard of it

    if(fileSize > (MEMORY - 512)){
        printf("Program selected is too big to load on the emulator!\nPlease select a smaller program\n");
        fclose(ptr);
        return -1;
    }

    size_t bytesRead = fread(&memory[512], 1, fileSize, ptr); //Return the total number of bytes read, which if everything goes fine, should be equal to the fileSize
    if(bytesRead != fileSize){
        printf("Error trying to read the program\n");
        fclose(ptr);
        return -1;
    }

    fclose(ptr);
    printf("Program loaded in memory successfully!\n");
    return 0;
}

void simulateCpu(){
    globalConfig.running = true;

    Uint64 frequency = SDL_GetPerformanceFrequency(); 
    Uint64 lastCycleTime = SDL_GetPerformanceCounter();
    while(globalConfig.running){

        Uint64 now = SDL_GetPerformanceCounter();
        double elapsedTime = ((double)(now - lastCycleTime) / (double)frequency) * 1000;

        if(elapsedTime >= 2){
            emulateCycle();
            lastCycleTime = now;
        }else{
            SDL_Delay(1);
        }
        if(drawFlag){
            updateScreen();
        }

    }

}

unsigned short fetchOpcode(){
    return memory[pc] << 8 | memory[pc + 1];
}

void emulateCycle(){
    //FETCH
    opcode = fetchOpcode();

    //DECODE
    switch(opcode & 0xF000){ //You only want to look at the first digit because is the one that tells you the opcode, therefore the AND operation with the 0xF000 
        case 0x0000:
            switch(opcode & 0x0FFF){
                case 0x00E0:
                    //clearScreen()
                    pc += 2;
                    break;
                
                case 0x00EE:
                    pc += 2;
                    break;
            }
        case 0x1000: //jump to the address NNN
            pc = opcode & 0x0FFF;
            break;

        case 0x2000: //calls subroutine at address NNN
            stack[sp] = pc;
            sp++; //avoid overwriting the current stack
            pc = opcode & 0x0FFF;
            break;

        case 0x3000:
            if(V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)){
                pc += 4;
            }else{
                pc += 2;
            }
            break;

        case 0x4000:
            if(V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)){
                pc += 4;
            }else{
                pc += 2;
            }
            break;

        case 0x5000:
            if(V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4]){
                pc += 4;
            }else{
                pc += 2;
            }
            break;

        case 0x6000:
            V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            pc += 2;
            break;

        case 0x7000:
            V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            pc += 2;
            break;

        case 0x8000:
            switch(opcode & 0x000F){
                case 0x0000: //store the value of VY on VX
                    V[(opcode & 0X0F00) >> 8] = V[(opcode & 0X00F0) >> 4];
                    pc += 2;
                    break;

                case 0x0001: //Set VX to VX OR VY
                    V[(opcode & 0X0F00) >> 8] = (V[(opcode & 0X0F00) >> 8] | V[(opcode & 0X00F0) >> 4]);
                    pc += 2;
                    break;

                case 0x0002: //Set VX to VX AND VY
                    V[(opcode & 0X0F00) >> 8] = (V[(opcode & 0X0F00) >> 8] & V[(opcode & 0X00F0) >> 4]);
                    pc += 2;
                    break;

                case 0x0003: //Set VX to VX XOR VY
                    V[(opcode & 0X0F00) >> 8] = (V[(opcode & 0X0F00) >> 8] ^ V[(opcode & 0X00F0) >> 4]);
                    pc += 2;
                    break;


                case 0x0004: //Add the value of register VY to register VX
                    {

                        unsigned int addition = 0;
                        addition = V[(opcode & 0X0F00) >> 8] + V[(opcode & 0X00F0) >> 4];
                        V[(opcode & 0X0F00) >> 8] = addition;
                        if(addition > 255){
                            V[15] = 0x01; 
                        }else{
                            V[15] = 0x00; 
                        }
                        pc += 2;

                    }
                    break;

                case 0x0005: //Subtract the value of register VY from register VX
                    {

                        int substraction = V[(opcode & 0X0F00) >> 8] - V[(opcode & 0X00F0) >> 4];
                        V[(opcode & 0X0F00) >> 8] = substraction;
                        if(substraction < 0){
                            V[15] = 0x01; 
                        }else{
                            V[15] = 0x00; 
                        }
                        pc += 2;
                        break;
                    }

                case 0x0006: //Store the value of register VY shifted right one bit in register VX
                    V[(opcode & 0X0F00) >> 8] = (V[(opcode & 0X00F0) >> 4] >> 1);
                    V[15] = V[(opcode & 0X00F0) >> 4] & 0x01;
                    pc += 2;
                    break;

                case 0x0007: //Set register VX to the value of VY minus VX
                    {
                        int substraction = (V[(opcode & 0X00F0) >> 4] - V[(opcode & 0X0F00) >> 8]);
                        V[(opcode & 0X0F00) >> 8] = substraction;
                        V[15] = V[(opcode & 0X00F0) >> 4] & 0x01;
                        pc += 2;
                    }
                    break;

                case 0x000E: //Store the value of register VY shifted left one bit in register VX
                    V[(opcode & 0X0F00) >> 8] = (V[(opcode & 0X00F0) >> 4] << 1);
                    V[15] = (V[(opcode & 0X00F0) >> 4] & 0x80) >> 7; //Shift it to the bit 0 so it stores a 01 or 00, not 10000000...
                    pc += 2;
                    break;
            }
        case 0x9000:
            if(V[(opcode & 0X0F00) >> 8] != V[(opcode & 0X00F0) >> 4]){
                pc += 4;
            }else{
                pc += 2;
            }
            break;

        case 0xA000:
            I = opcode & 0x0FFF; 
            pc += 2;
            break;

        case 0xB000: //Jump to address NNN + V0
            pc = (opcode & 0X0FFF) + V[0];
            break;

        case 0xC000://Set VX Random number with a mask of NN
            generateRandomNN(opcode & 0x00FF);
            break;

        case 0xD000://Draw a sprite at position VX, VY with N bytes of sprite data starting at the address stored in I 
            {
                unsigned short x = V[(opcode & 0x0F00) >> 8];
                unsigned short y = V[(opcode & 0x00F0) >> 4];
                unsigned short height = opcode & 0x000F;
                unsigned short pixel;

                V[0xF] = 0;
                for (int yline = 0; yline < height; yline++)
                {
                    pixel = memory[I + yline];
                    for(int xline = 0; xline < 8; xline++)
                    {
                        if((pixel & (0x80 >> xline)) != 0)
                        {
                            if(gpx[(x + xline + ((y + yline) * 64))] == 1)
                                V[0xF] = 1;                                 

                            gpx[x + xline + ((y + yline) * 64)] ^= 1;
                        }
                    }
                }
                drawFlag = true;
                pc += 2;
            }
            //drawSprite(V[(opcode & 0x0F00) >> 8], V[(opcode & 0x00F0) >> 4], opcode & 0x000F, globalConfig.window, globalConfig.renderer);
            break;
        case 0xE000: 
            switch (opcode & 0x000F) {
                case 0x000E: //Skip the following instruction if the key corresponding to the hex value currently stored in register VX is pressed
                    if(handleKeyPad() == V[(opcode & 0x0F00) >> 8]){
                        pc += 4;
                    }else{
                        pc += 2;
                    }                         
                    break;

                case 0x0001://Skip the following instruction if the key corresponding to the hex value currently stored in register VX is not pressed
                    if(handleKeyPad() != V[(opcode & 0x0F00) >> 8]){
                        pc += 4;
                    }else{
                        pc += 2;
                    }                         
                    break;
            }
            break;

        case 0xF000:
            switch (opcode & 0x00FF){
                case 0x0007://Store the current value of the delay timer in register VX
                    V[(opcode & 0X0F00) >> 8] = delay_timer;
                    pc+= 2;
                    break;

                case 0x000A://Wait for a keypress and store the result in register VX
                {
                    unsigned char press = waitForPress();
                    if(press == 0xFF){
                        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Wrong key pressed, not included in the chip-8 keypad");
                    }else if(press == 0xF0){
                        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "An error happened while trying to get the SDL Keyboard state");
                    }else{
                        V[(opcode & 0x0F00) >> 8] = waitForPress();
                    }
                    pc+= 2;
                }
                    break;

                case 0x0015://Set the delay timer to the value of register VX
                    delay_timer = V[(opcode & 0X0F00) >> 8]; 
                    pc+= 2;
                    break;

                case 0x0018://Set the sound timer to the value of register VX
                    sound_timer = V[(opcode & 0X0F00) >> 8]; 
                    pc+= 2;
                    break;

                case 0x001E:
                    I += V[(opcode & 0X0F00) >> 8]; 
                    pc+= 2;
                    break;

                case 0x0029://Set I to the memory address of the sprite data corresponding to the hexadecimal digit stored in register VX
                    pc+= 2;
                    break;

                case 0x0033://Store the binary-coded decimal equivalent of the value stored in register VX at addresses I, I + 1, and I + 2
                    memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
                    memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
                    memory[I + 2] = V[(opcode & 0x0F00) >> 8]  % 10;
                    pc+= 2;
                    break;

                    case 0x0055: //Store the values of registers V0 to VX inclusive in memory starting at address I. I is set to I + X + 1 after operation²
                    for(int i = 0; i < (opcode & 0x0F00); i++){
                        memory[I] = V[i];
                    }
                    I = I + (opcode & 0x0F00) + 1;
                    pc+= 2;
                    break;

                case 0x0065: //Fill registers V0 to VX inclusive with the values stored in memory starting at address I. I is set to I + X + 1 after operation²
                    for(int i = 0; i < (opcode & 0x0F00); i++){
                        V[i] = memory[I];
                    }
                    I = I + (opcode & 0x0F00) + 1;
                    pc+= 2;
                    break;
            }
            break;
        default:
            printf("Unkonwn opcode: 0x%X\n", opcode);
            pc += 2;
    } 

    if(delay_timer > 0){
        delay_timer--;
    }

    if(sound_timer > 0){
        if(sound_timer == 1){
            printf("Beep Time!\n");
            sound_timer--;
        }
    }
}

//Because this function needs to work with memory and the Index register, is going to be placed at the chip8.c file for the moment
int drawSprite(unsigned char x, unsigned char y, unsigned char nBytes, SDL_Window *window, SDL_Renderer *renderer){
    for(int row = 0; row  < nBytes; row++){
        //unsigned char mask = 0;
        for(int i = 7; i >= 0; i++){
            if((memory[I + row] >> i) & 0x01){
                SDL_FRect rect = {x, y, 25, 25};
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &rect);
            }else{
                printf("Do not paint pixel!\n");
            }
        }
    }  
    return 0;
}

int updateScreen(){
    for(int i = 0; i < SCREEN_WIDTH; i++){
        for(int j = 0; j < SCREEN_HEIGHT; j++){
            if(!gpx[i * j]){
                //The reason it says scalated is because the screen is 25 x the original resolution, otherwise it would be way too small
                drawScalatedPixel(i, j, globalConfig.renderer);
            }
        }
    }
    //Update the screen once all the pixels (SDL_FRect) have been set
    SDL_RenderPresent(globalConfig.renderer);
    return 0;
}


int clearScreen(){
    for(int x = 0; x < SCREEN_WIDTH; x++){
        for(int y = 0; y < SCREEN_HEIGHT; y++){
            gpx[x * y] = 0;
        }
    }
    SDL_FRect screen = {0, 0, SCREEN_WIDTH * 25, SCREEN_HEIGHT * 25};
    SDL_SetRenderDrawColor(globalConfig.renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(globalConfig.renderer, &screen);
    SDL_RenderPresent(globalConfig.renderer);
    return 0;
}
