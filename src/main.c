#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_pixels.h>

#include "../include/chip8.h"
#include "../include/config.h"
#include "../include/functions.h"

/*
 TODO:
 - Check the mapping of the real keyboard to the chip8 keypad, i dont think its that well implmented
 */

Config *globalConfig;

emulObjects objects = {.start= false, .keepGoing = false, .window = NULL, .renderer = NULL, .texture = NULL};



void quit(int signum);
int setFileName(const char *argName);

int main(int argc, char **argv){
    signal(SIGTERM, quit);
    signal(SIGQUIT, quit);
    signal(SIGKILL, quit);
    signal(SIGINT, quit);

    globalConfig = readConfiguration("config/config.txt");
    for(int i = 0; i < argc; i++){
        if(strcmp(argv[i], "-DEBUG_OUTPUT") == 0){
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Verbose output enabled\n");
            globalConfig->debugOutput = true;
        }
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
    objects.texture = SDL_CreateTexture(objects.renderer, 
            SDL_PIXELFORMAT_ARGB8888, 
            SDL_TEXTUREACCESS_STREAMING, 
            SCREEN_WIDTH, SCREEN_HEIGHT);
    if(objects.texture == NULL){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error when creating SDL_Texture\n");
        return -1;
    }

    SDL_SetTextureScaleMode(objects.texture, SDL_SCALEMODE_NEAREST);


    //Snippet to test the screen
    /*
    SDL_RenderPresent(globalConfig.renderer);
    updateScreen();
    SDL_Delay(1000);
    clearScreen();
    SDL_Delay(1000);
    */

    initialize();
    if(loadProgram(objects.filename) == -1){
        cleanup();
    }
    SDL_Delay(1000);
    simulateCpu();

    cleanup();
    return 0;
}

void quit(int signum){
    cleanup();
}

int setFileName(const char *argName){

    strcpy(objects.filename, argName);
    printf("FileName: %s\n", objects.filename);
    return 0;
}
