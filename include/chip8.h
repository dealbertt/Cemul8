#ifndef CHIP8_H
#define CHIP8_H
#define MEMORY 4096
#define CPU_FREQ 500
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define SCREEN_REFRESH_RATE 60 //for the moment
                               
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
typedef struct{
    uint16_t opcode; //the operation code of the instructiouint16_t n
    uint8_t memory[MEMORY]; //the total memory of the chip-8
    uint8_t V[16]; //all the general purpose registers

    uint16_t I; //special register I for memory addresses
    uint16_t pc; //Program counter

    uint8_t gpx[SCREEN_WIDTH * SCREEN_HEIGHT]; //pixels of the screen. Total pixels in the array: 2048
                                               //Keep in mind that this are the screens of the CHIP-8, but not of the actual screen that is showed, that is because the original resolution is way too small

    char instructionDescription[25];

    uint8_t delay_timer;
    uint8_t sound_timer;

    //STACK
    uint16_t stack[16];
    uint16_t sp; //stack pointer
                 //
    uint8_t keyPad[16];
}Chip8;


void initialize();

void initializeMemory();

void initRegisters();

void emulateCycle(); 

void simulateCpu();

unsigned short fetchOpcode();
void decode();

int loadProgram(const char *fileName);

int drawSprite(unsigned char x, unsigned char y, unsigned char nBytes, SDL_Window *window, SDL_Renderer *renderer);

int updateScreen();

int clearScreen();

int handleRealKeyboard();
unsigned char handleKeyPad();


int renderFrame();


#endif
