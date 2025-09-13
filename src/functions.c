#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_log.h>

#include "../include/functions.h"
#include "../include/config.h"

extern char fileName[20];
extern unsigned char memory[4096]; //the total memory of the chip-8
extern Config *globalConfig;


unsigned char generateRandomNN(int mask){
    int randomNumber = rand() % (255 + 1 - 0) + 0;
    return randomNumber & mask;
}

int drawScalatedPixel(int x, int y, SDL_Renderer *renderer, SDL_Color color){
    SDL_FRect drawRect = {x * globalConfig->scalingFactor, y * globalConfig->scalingFactor, globalConfig->scalingFactor, globalConfig->scalingFactor};
    SDL_SetRenderDrawColor(renderer, color.r ,color.g, color.b, color.a); //All white
    SDL_RenderFillRect(renderer , &drawRect); 
    return 0;
}
void cleanup(){
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Quitting emul8...\n");
    SDL_Quit();
    exit(0);
}
