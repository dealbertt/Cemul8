#ifndef FUNCTIONS_H
#define FUNCTIONS_H


#include <SDL3/SDL_render.h>


int handleRealKeyboard();
unsigned char handleKeyPad();

unsigned char waitForPress();
unsigned char returnKeyEquivalent(const bool *pressed);
unsigned char checkHexValue();
 
unsigned char generateRandomNN(int mask);

int drawScalatedPixel(int x, int y, SDL_Renderer *renderer);

#endif
