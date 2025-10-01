#include <stdio.h>
#include <string.h>
#include <signal.h>



#include "../include/chip8.h"
#include "../include/config.h"
#include "../include/functions.h"
#include "../include/overlay.h"

/*
TODO:
- Reset button

- Also have to implement audio at some point (f*ck)

- cleanup and code improvements
 */
Config *globalConfig = NULL;

emulObjects objects = {.start= false, .keepGoing = false, .executeOnce = false, .window = NULL, .renderer = NULL, .mainScreenTexture= NULL,  .instructionPanelTitle = NULL, .controlsPanelTitle = NULL, .internalsTitlePanel = NULL, .color = {255, 255, 255, 255}};

SDL_AudioStream *stream = NULL;

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

    if(!SDL_CreateWindowAndRenderer("Cemul8", SCREEN_WIDTH * globalConfig->scalingFactor, SCREEN_HEIGHT * globalConfig->scalingFactor, SDL_WINDOW_RESIZABLE, &objects.window, &objects.renderer)){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error on SDL_CreateWindowAndRenderer: %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetWindowPosition(objects.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    if(!TTF_Init()){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error with TTF_Init: %s\n", SDL_GetError());
        cleanup();
    }


    if(initPanelTitles(&objects, globalConfig->scalingFactor) == 1){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error with initPanelTitles");
        cleanup();
    }
                                                        
    initialize(); //initializes all the chip-8 components
    if(loadProgram(objects.filename) == -1){
        cleanup();
    }
    
    SDL_Delay(1000);
    simulateCpu();

    TTF_CloseFont(objects.font);
    SDL_DestroyTexture(objects.mainScreenTexture);
    SDL_DestroyTexture(objects.instructionPanelTitle);
    SDL_DestroyTexture(objects.controlsPanelTitle);
    SDL_DestroyTexture(objects.internalsTitlePanel);

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
