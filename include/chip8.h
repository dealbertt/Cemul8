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
    bool debugOutput;
    SDL_Window *window;
    SDL_Renderer *renderer;
}Config;

void initialize();

void initializeMemory();

void initRegisters();

void emulateCycle(); //Emulates

unsigned short fetchOpcode();

int loadProgram(const char *fileName);

int setFileName(const char *argName);
#endif
