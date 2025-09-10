#ifndef CHIP8_H
#define CHIP8_H
#define MEMORY 4096
#define CPU_FREQ 500
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define SCREEN_REFRESH_RATE 60 //for the moment
                               
#include <stdbool.h>
#include <SDL3/SDL.h>

typedef struct{
    bool running;
    SDL_Window *window;
    SDL_Renderer *renderer;
    char filename[20];
}emulObjects;

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

int simulateOpcode(unsigned short simOpcode);
#endif
