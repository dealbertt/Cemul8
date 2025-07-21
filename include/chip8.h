#ifndef CHIP8_H
#define CHIP8_H
#define MEMORY 4096
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define SCREEN_REFRESH_RATE 60 //for the moment
                               
#if DEBUG_OUTPUT
#define VERBOSE_DEBUG_OUTPUT 1
#else
#define VERBOSE_DEBUG_OUTPUT 0
#endif

                               


void initialize();

void initializeMemory();

void initRegisters();

void emulateCycle();

int loadProgram(const char *fileName);

int setFileName(const char *argName);

#endif
