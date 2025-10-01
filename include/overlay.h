#ifndef OVERLAY_H
#define OVERLAY_H

#include "chip8.h"
#include "config.h"

int preRenderInstructions(const emulObjects *objects, const Chip8 *chip);

//Rendering functions

int renderInstructionPanel(const emulObjects *objects, const Chip8 *chip, const int scalingFactor);


int renderControlPanel(const emulObjects *objects, const Chip8 *chip);

int renderInternalPanel(const emulObjects *objects, const Chip8 *chip);

int renderTimerPanel(const emulObjects *objects, const Chip8 *chip);

//Initializing the different titles
int initializeAllRendering(emulObjects *objects, Chip8 *chip);

int initControlPanel(emulObjects *objects, Chip8 *chip);

int initPanelTitles(emulObjects *objects, const int scalingFactor);

int initRegisterPanel(const emulObjects *objects, const Chip8 *chip);
int initRegisterTextures(SDL_Renderer *renderer, TTF_Font *font, SDL_Color color);

int initTimerPanel(emulObjects *objects, Chip8 *chip);

int initIndexPanel(emulObjects *objects, Chip8 *chip);

int renderIndexPanel(emulObjects *objects, Chip8 *chip);
char *getLongerInstruction(const uint16_t currentOpcode, const uint16_t secondPc);

void freeOverlayStructs();
#endif
