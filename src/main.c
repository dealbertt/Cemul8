#include <SDL3/SDL_timer.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

#include "../include/chip8.h"
#include "../include/functions.h"

extern char fileName[20];

Config globalConfig = {.debugOutput = false, .running = false};

void quit(int signum);

int main(int argc, char **argv){
    signal(SIGTERM, quit);
    signal(SIGQUIT, quit);
    signal(SIGKILL, quit);
    signal(SIGINT, quit);

    for(int i = 0; i < argc; i++){
        if(strcmp(argv[i], "-DEBUG_OUTPUT") == 0){
            printf("Verbose output enabled\n");
            globalConfig.debugOutput = true;
        }
    }
    if(argc > 1){
        setFileName(argv[1]);
    }else{

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
    SDL_RenderPresent(globalConfig.renderer);
    updateScreen();
    SDL_Delay(1000);
    //loadProgram(fileName);
    //simulateCpu();

    SDL_Quit();
    return 0;
}

void quit(int signum){
    (void)signum;
    exit(-1);
}
