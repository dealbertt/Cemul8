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
    bool start;
    bool keepGoing;
    bool executeOnce;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *mainScreenTexture;
    SDL_Texture *instructionPanelTitle;
    SDL_FRect instructiontitleRect;
    SDL_Texture *controlsPanelTitle;
    SDL_FRect controlTitleRect;
    SDL_Texture *internalsTitlePanel;
    SDL_FRect internalTitleRect;
    TTF_Font *font;
    char filename[150];
}emulObjects;

Config *readConfiguration(const char *path);

#endif //CONFIG_H
