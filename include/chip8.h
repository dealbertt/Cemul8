#ifndef CHIP8_H
#define CHIP8_H
#define MEMORY 4096
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define SCREEN_REFRESH_RATE 60 //for the moment


void initialize();

void initializeMemory();

void initRegisters();

void emulateCycle();

void loadProgram(const char *fileName);

void setFileName(const char *argName);

#endif
