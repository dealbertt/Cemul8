#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_error.h>
#include <SDL3_ttf/SDL_ttf.h>


#include "../include/chip8.h"
#include "../include/config.h"
#include "../include/functions.h"
#include "../include/overlay.h"
#include "../include/audio.h"

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
    
    SDL_AudioSpec spec;
    spec.freq = 44100;
    spec.format = SDL_AUDIO_S16;
    spec.channels = 1;
    
    uint8_t *wavBuffer;
    uint32_t wavLength;

    if(!SDL_LoadWAV("beep.wav", &spec, &wavBuffer, &wavLength)){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error trying to open wav file: %s\n", SDL_GetError());
        return -1;
    }

    SDL_AudioDeviceID device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if(!device){
        printf("error\n");
        return 0;
    }
    SDL_AudioStream *stream = SDL_CreateAudioStream(&spec, &spec);
    if(stream == NULL){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error opening audio device stream: %s\n", SDL_GetError());
        return -1;
    }

    SDL_BindAudioStream(device, stream);
    SDL_PutAudioStreamData(stream, wavBuffer, wavLength);

    SDL_ResumeAudioDevice(device);

    // Wait until the sound has finished playing
    while (SDL_GetAudioStreamQueued(stream) > 0) {
        SDL_Delay(100);
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
