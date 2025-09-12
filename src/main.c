#include <SDL3/SDL_log.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

#include "../include/chip8.h"
#include "../include/config.h"

/*
 TODO:
 - Create Simulate/Test opcode function to test each individual opcode

 */

Config *globalConfig;

char fileName[20];
emulObjects objects = {.running = false, .window = NULL, .renderer = NULL};


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
            printf("Verbose output enabled\n");
            globalConfig->debugOutput = true;
        }
    }
    if(argc > 1){
        setFileName(argv[1]);
    }else{
        SDL_LogError(SDL_LOG_PRIORITY_ERROR, "No program has been provided for the emulator:");
        return -1;
    }

    if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)){
        SDL_LogError(SDL_LOG_PRIORITY_ERROR, "Error trying to initialize SDL: %s\n", SDL_GetError());
        return -1;
    }

    if(!SDL_CreateWindowAndRenderer("emul8", SCREEN_WIDTH * globalConfig->scalingFactor, SCREEN_HEIGHT * globalConfig->scalingFactor, SDL_WINDOW_RESIZABLE, &objects.window, &objects.renderer)){
        SDL_LogError(SDL_LOG_PRIORITY_ERROR, "Error on SDL_CreateWindowAndRenderer: %s\n", SDL_GetError());
        return -1;
    }
    

    //Snippet to test the screen
    /*
    SDL_RenderPresent(globalConfig.renderer);
    updateScreen();
    SDL_Delay(1000);
    clearScreen();
    SDL_Delay(1000);
    */
    initialize();
    loadProgram(objects.filename);
    simulateCpu();

    SDL_Quit();
    return 0;
}

void quit(int signum){
    (void)signum;
    exit(-1);
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
