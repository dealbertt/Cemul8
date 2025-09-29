#ifndef CONFIG_H
#define CONFIG_H
#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

typedef struct{
    bool debugOutput;
    short scalingFactor;
}Config;

typedef struct{
    TTF_Font *font;
    char filename[150];
    SDL_Color color;

    //Booleans for debugging
    bool start;
    bool keepGoing;
    bool executeOnce;

    //The sdl_window and renderer for the app
    SDL_Window *window;
    SDL_Renderer *renderer;

    //The texture for the chip-8 screen
    SDL_Texture *mainScreenTexture;

    //Texture and rect for the instruction panel title (20 instruction from the pc)
    SDL_Texture *instructionPanelTitle;
    SDL_FRect instructiontitleRect;

    //Texture and rect for the control panel title
    SDL_Texture *controlsPanelTitle;
    SDL_FRect controlTitleRect;

    //Texture and rect for the internal panel title
    SDL_Texture *internalsTitlePanel;
    SDL_FRect internalTitleRect;

    //Texture and rect for the control instructions (all text) 
    SDL_Texture *controlInstructions;
    SDL_FRect controlInstructionsRect;

    SDL_Texture *timersTitle;
    SDL_FRect timersTitleRect;
}emulObjects;

Config *readConfiguration(const char *path);

#endif //CONFIG_H
