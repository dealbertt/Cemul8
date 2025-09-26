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
    bool start;
    bool keepGoing;
    bool executeOnce;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *mainScreenTexture;
    SDL_Texture *instructionPanelTitle;
    SDL_Texture *controlsPanelTitle;
    SDL_FRect instructiontitleRect;
    SDL_FRect controlTitleRect;
    TTF_Font *font;

    char filename[150];
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

int handleRealKeyboard();
unsigned char handleKeyPad();


void checkRegisters();
void checkStack();
void checkKeyPad();
void checkInternals();

int renderFrame();

SDL_Texture *display10Instructions();
SDL_Texture *renderControlPanel();
char *getLongerInstruction(const uint16_t currentOpcode, const uint16_t secondPc);

#endif
