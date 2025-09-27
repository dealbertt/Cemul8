#ifndef OVERLAY_H
#define OVERLAY_H

#include "chip8.h"
#include "config.h"

int renderInstructionPanel(emulObjects objects, Chip8 *chip, int scalingFactor);
int renderControlPanel(emulObjects *objects, Chip8 *chip);
int renderInternalPanel(emulObjects *objects, Chip8 *chip);
char *getLongerInstruction(const uint16_t currentOpcode, const uint16_t secondPc);

int initControlPanel(emulObjects *objects, Chip8 *chip);
int initPanelTitles(emulObjects *objects, int scalingFactor);

int initRegisterPanel(emulObjects *objects, Chip8 *chip);
int initRegisterTextures(SDL_Renderer *renderer, TTF_Font *font);
int updateRegisterValue();

#endif
