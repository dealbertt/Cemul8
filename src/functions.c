#include <SDL3/SDL_render.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_log.h>

#include "../include/functions.h"

char fileName[20];
extern unsigned char memory[4096]; //the total memory of the chip-8

int setFileName(const char *argName){
    if(strstr(argName, ".ch8") == NULL && strstr(argName, ".c8") == NULL){
        printf("Please select a file with the extension .ch8 or .c8\n");
        return -1;
    }

    strcpy(fileName, argName);
    printf("FileName: %s\n", fileName);
    return 0;
}

int handleRealKeyboard(){
    SDL_Event event;
    SDL_PollEvent(&event);

    const bool *pressed = SDL_GetKeyboardState(NULL);
    if(event.type == SDL_EVENT_QUIT){
        SDL_Quit();
        return 0xF0;
    }

    if(event.type == SDL_EVENT_KEY_DOWN){
        returnKeyEquivalent(pressed);
    }
    return 0;
}


unsigned char handleKeyPad(){
    SDL_Event event;
    SDL_PollEvent(&event);

    const bool *pressed = SDL_GetKeyboardState(NULL);
    if(event.type == SDL_EVENT_QUIT){
        SDL_Quit();
        return 0xF0;
    }

    if(event.type == SDL_EVENT_KEY_DOWN){
        return returnKeyEquivalent(pressed);
    }
    return 0;

}
unsigned char waitForPress(){
    SDL_Event event;
    SDL_WaitEvent(&event);
    const bool *pressed = SDL_GetKeyboardState(NULL);
    if(event.type == SDL_EVENT_QUIT){
        SDL_Quit();
        return 0xF0;
    }
    if(event.type == SDL_EVENT_KEY_DOWN){
        return returnKeyEquivalent(pressed);
    }
    return 0xFF;
}

unsigned char returnKeyEquivalent(const bool *pressed){
    if(pressed[SDL_SCANCODE_1]){
        return 0x1;
    }else if(pressed[SDL_SCANCODE_2]){
        return 0x2;
    }else if(pressed[SDL_SCANCODE_3]){
        return 0x3;
    }else if(pressed[SDL_SCANCODE_4]){
        return 0xC;
    }else if(pressed[SDL_SCANCODE_Q]){
        return 0x4;
    }else if(pressed[SDL_SCANCODE_W]){
        return 0x5;
    }else if(pressed[SDL_SCANCODE_E]){
        return 0x6;
    }else if(pressed[SDL_SCANCODE_R]){
        return 0xD;
    }else if(pressed[SDL_SCANCODE_A]){
        return 0x7;
    }else if(pressed[SDL_SCANCODE_S]){
        return 0x8;
    }else if(pressed[SDL_SCANCODE_D]){
        return 0x9;
    }else if(pressed[SDL_SCANCODE_F]){
        return 0xE;
    }else if(pressed[SDL_SCANCODE_Z]){
        return 0xA;
    }else if(pressed[SDL_SCANCODE_X]){
        return 0x0;
    }else if(pressed[SDL_SCANCODE_C]){
        return 0xB;
    }else if(pressed[SDL_SCANCODE_V]){
        return 0xF;
    }
    return 0xFF;
}

unsigned char generateRandomNN(int mask){
    int randomNumber = rand() % (255 + 1 - 0) + 0;
    return randomNumber & mask;
}

int drawScalatedPixel(int x, int y, SDL_Renderer *renderer){
    SDL_FRect drawRect = {x * 25, y * 25, 25, 25};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); //All white
    SDL_RenderFillRect(renderer , &drawRect); 
    return 0;
}
