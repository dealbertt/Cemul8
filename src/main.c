#include <SDL3/SDL_video.h>
#include <stdio.h>
#include <string.h>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>

#include "../include/chip8.h"


extern char fileName[20];

Config globalConfig = {.debugOutput = false, .running = false};

int main(int argc, char **argv){
    for(int i = 0; i < argc; i++){
        if(strcmp(argv[i], "-DEBUG_OUTPUT") == 0){
            printf("Verbose output enabled\n");
            globalConfig.debugOutput = true;
        }
    }
    if(argc > 1){
        setFileName(argv[1]);
    }else{
        printf("Please select a file to load in the emulator!\n");
        return -1;
    }

    if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)){
        SDL_LogError(SDL_LOG_PRIORITY_ERROR, "Error trying to initialize SDL: %s\n", SDL_GetError());
        return -1;
    }
    if(!SDL_CreateWindowAndRenderer("Chip-8", SCREEN_WIDTH * 25, SCREEN_HEIGHT * 25, SDL_WINDOW_RESIZABLE, &globalConfig.window, &globalConfig.renderer)){
        SDL_LogError(SDL_LOG_PRIORITY_ERROR, "Error on SDL_CreateWindowAndRenderer: %s\n", SDL_GetError());
        return -1;
    }
    loadProgram(fileName);
    simulateCpu();

    SDL_Quit();
    return 0;
}

