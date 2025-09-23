#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_error.h>
#include <SDL3_ttf/SDL_ttf.h>


#include "../include/chip8.h"
#include "../include/config.h"
#include "../include/functions.h"


/*
TODO:
- Maybe separate the different rendering of the textures into different functions so that its not as messy as it is right now

- Also have to think about moving some code to other files, because the text rendering and stuff shouldnt be in the chip8.c file

- Also have to implement audio at some point (f*ck)
- Im sure there is a much better way of actually rendering all those instructions, but for now, it gets the job done, need to maybe create a small font to properly fit all 20 instructions

- Oh yeah and maybe fix the wrapping qirk


 */

Config *globalConfig = NULL;

emulObjects objects = {.start= false, .keepGoing = false, .executeOnce = false, .window = NULL, .renderer = NULL, .mainScreenTexture= NULL,  .instructionPanelTitle = NULL};



void quit(int signum);
int setFileName(const char *argName);

int main(int argc, char **argv){
    signal(SIGTERM, quit);
    signal(SIGQUIT, quit);
    signal(SIGINT, quit);

    globalConfig = readConfiguration("config/config.txt");
    if(globalConfig == NULL){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error while trying to load the config file\n");
        return -1;
    } 

    if(argc > 1){
        setFileName(argv[1]);
    }else{
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "No program has been provided for the emulator:");
        return -1;
    }

    if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error trying to initialize SDL: %s\n", SDL_GetError());
        return -1;
    }

    if(!SDL_CreateWindowAndRenderer("emul8", SCREEN_WIDTH * globalConfig->scalingFactor, SCREEN_HEIGHT * globalConfig->scalingFactor, SDL_WINDOW_RESIZABLE, &objects.window, &objects.renderer)){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error on SDL_CreateWindowAndRenderer: %s\n", SDL_GetError());
        return -1;
    }
    objects.mainScreenTexture = SDL_CreateTexture(objects.renderer, 
            SDL_PIXELFORMAT_ARGB8888, 
            SDL_TEXTUREACCESS_TARGET, 
            SCREEN_WIDTH, SCREEN_HEIGHT);

    if(objects.mainScreenTexture == NULL){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error when creating mainScreenTexture: %s\n", SDL_GetError());
        return -1;
    }


    SDL_SetTextureScaleMode(objects.mainScreenTexture, SDL_SCALEMODE_NEAREST);



    if(!TTF_Init()){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error with TTF_Init: %s\n", SDL_GetError());
        cleanup();
    }

    char fontPath[40] = "fonts/FiraCodeNerdFont-Regular.ttf";
    objects.font = TTF_OpenFont(fontPath, 40);
    if(objects.font == NULL){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error trying to open the font: %s\n", SDL_GetError());
        cleanup();
    }

    SDL_Color color = {255, 255, 255, 255};


    char title[15] = "INSTRUCTIONS";

    SDL_Surface *titleSurface = TTF_RenderText_Solid(objects.font, title, strlen(title), color);
    objects.instructionPanelTitle = SDL_CreateTextureFromSurface(objects.renderer, titleSurface);

    SDL_DestroySurface(titleSurface);

    objects.titleRect.x = 0;
    objects.titleRect.y = 0;
    objects.titleRect.w = (SCREEN_WIDTH * globalConfig->scalingFactor) / 4.0 + 0.5;
    objects.titleRect.h = 50;
    SDL_RenderTexture(objects.renderer, objects.instructionPanelTitle, NULL, &objects.titleRect);

                                                        
    initialize(); //initializes ll the chip-8 components
    if(loadProgram(objects.filename) == -1){
        cleanup();
    }
    SDL_Delay(1000);
    simulateCpu();

    TTF_CloseFont(objects.font);

    cleanup();
    return 0;
}

void quit(int signum){
    cleanup();
}

int setFileName(const char *argName){
     if(strstr(argName, ".ch8") == NULL && strstr(argName, ".c8") == NULL){
        printf("Please select a file with the extension .ch8 or .c8\n");
        return -1;
    }

    strcpy(objects.filename, argName);
    printf("FileName: %s\n", objects.filename);
    return 0;
}
