#include <stdio.h>
#include <string.h>
#include "../include/chip8.h"

//CHIP-8 specifications
unsigned short opcode; //the operation code of the instruction
unsigned char memory[MEMORY]; //the total memory of the chip-8
unsigned char V[16]; //all the general purpose registers

unsigned short I; //special register I for memory addresses
unsigned short pc; //Program counter


unsigned char gpx[SCREEN_WIDTH * SCREEN_WIDTH]; //pixels of the screen

unsigned char delay_timer;
unsigned char sound_timer;

//STACK
unsigned short stack[16];
unsigned short sp; //stack pointer

unsigned char keyPad[16];
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
char fileName[20];
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
}

void initRegisters(){
    for(int i = 0; i < 16; i++){
        V[i] = 0;
    }
    I = 0;
    pc = 0x200;
}
/*
void loadProgram(const char *fileName){
    FILE *ptr = fopen(fileName, "rb");
    int bufferSize = 100;
    for(int i = 0; i < bufferSize; i++){
        memory[i + 512] = buffer[i];
    } 
    
}
*/

void emulateCycle(){
    //FETCH
    opcode = memory[pc] << 8 | memory[pc + 1];

    //DECODE
    switch(opcode & 0xF000){ //You only want to look at the first digit because is the one that tells you the opcode, therefore the AND operation with the 0xF000 
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
            }

        case 0xA000:
            I = opcode & 0x0FFF; //We take 0x0FFF because we only care about the last 3 digits, which contain the address that we want to set the register I to
            pc += 2;
            break;

        case 0xB000: //Jump to address NNN + V0
            pc = (opcode & 0X0FFF) + V[0];
            break;

        case 0xC000:// random number with a mask of NN
            break;

        default:
            printf("Unkonwn opcode: 0x%X\n", opcode);
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

int setFileName(const char *argName){
    if(strstr(argName, ".ch8") == NULL || strstr(argName, ".c8") == NULL){
        printf("Please select a file with the extension .ch8 or .c8\n");
        return -1;
    }

    strcpy(fileName, argName);
    printf("FileName: %s\n", fileName);

    return 0;
}
