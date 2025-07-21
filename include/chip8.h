#ifndef CHIP8_H
#define CHIP8_H
#define MEMORY 4096
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define SCREEN_REFRESH_RATE 60 //for the moment
                               
#include <stdbool.h>
typedef struct{
    bool debugOutput;
}Config;
void initialize();

void initializeMemory();

void initRegisters();

void emulateCycle();

int loadProgram(const char *fileName);

int setFileName(const char *argName);

#endif
