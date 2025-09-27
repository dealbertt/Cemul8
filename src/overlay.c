#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/overlay.h"
#include "../include/config.h"
typedef struct{
    SDL_Texture *keyTexture;
    SDL_Color color;
    SDL_FRect keyRect;
    uint8_t keyCode;
}keyPadTexture;

typedef struct{
    SDL_Texture *instTexture;
    uint16_t designatedOpcode;
    char *description;
}instructionTexture;
int xIncrement;

keyPadTexture keyTextures[16];

instructionTexture displayedInstructions[20];

int initPanelTitles(emulObjects *objects, int scalingFactor){
    objects->mainScreenTexture = SDL_CreateTexture(objects->renderer, 
            SDL_PIXELFORMAT_ARGB8888, 
            SDL_TEXTUREACCESS_TARGET, 
            SCREEN_WIDTH, SCREEN_HEIGHT);

    if(objects->mainScreenTexture == NULL){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error when creating mainScreenTexture: %s\n", SDL_GetError());
        return -1;
    }


    SDL_SetTextureScaleMode(objects->mainScreenTexture, SDL_SCALEMODE_NEAREST);

    char fontPath[40] = "fonts/FiraCodeNerdFont-Regular.ttf";
    objects->font = TTF_OpenFont(fontPath, 40);
    if(objects->font == NULL){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error trying to open the font: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Color color = {255, 255, 255, 255};


    char instructionTitle[15] = "INSTRUCTIONS";

    SDL_Surface *titleSurface = TTF_RenderText_Solid(objects->font, instructionTitle, strlen(instructionTitle), color);
    objects->instructionPanelTitle = SDL_CreateTextureFromSurface(objects->renderer, titleSurface);


    objects->instructiontitleRect.x = 0;
    objects->instructiontitleRect.y = 0;
    objects->instructiontitleRect.w = titleSurface->w;
    objects->instructiontitleRect.h = titleSurface->h;
    SDL_RenderTexture(objects->renderer, objects->instructionPanelTitle, NULL, &objects->instructiontitleRect);
    SDL_DestroySurface(titleSurface);

    char controlTitleString[15] = "CONTROLS";

    SDL_Surface *controlSurface = TTF_RenderText_Solid(objects->font, controlTitleString, strlen(controlTitleString), color);
    objects->controlsPanelTitle = SDL_CreateTextureFromSurface(objects->renderer, controlSurface);


    objects->controlTitleRect.x = (SCREEN_WIDTH * scalingFactor) / 2.0 - 100;
    objects->controlTitleRect.y = (SCREEN_HEIGHT * scalingFactor) / 2.0;
    objects->controlTitleRect.w = controlSurface->w;
    objects->controlTitleRect.h = controlSurface->h;
    SDL_RenderTexture(objects->renderer, objects->controlsPanelTitle, NULL, &objects->controlTitleRect);
    SDL_DestroySurface(controlSurface);

    char internalTitle[15] = "INTERNALS";
    SDL_Surface *internalSurface = TTF_RenderText_Solid(objects->font, internalTitle, strlen(internalTitle), color);
    objects->internalsTitlePanel = SDL_CreateTextureFromSurface(objects->renderer, internalSurface);

    objects->internalTitleRect.x = (SCREEN_WIDTH * scalingFactor) * 0.75;
    objects->internalTitleRect.y = 0;
    objects->internalTitleRect.w = internalSurface->w;
    objects->internalTitleRect.h = internalSurface->h;

    SDL_RenderTexture(objects->renderer, objects->internalsTitlePanel, NULL, &objects->internalTitleRect);
    SDL_DestroySurface(internalSurface);


    return 0;
}

int initControlPanel(emulObjects *objects, Chip8 *chip){
    SDL_Color color = {255, 255, 255, 255};
    char instructions[150] = "F1: STOP/RESUME EXECUTION | F6: EXECUTE ONE INSTRUCTION";
    TTF_SetFontSize(objects->font, 20);
    SDL_Surface *instructionSurface = TTF_RenderText_Solid(objects->font, instructions, strlen(instructions), color);

    objects->controlInstructions = SDL_CreateTextureFromSurface(objects->renderer, instructionSurface);

    objects->controlInstructionsRect.x = (SCREEN_WIDTH * 25) / 4.0;
    objects->controlInstructionsRect.y = objects->controlTitleRect.y + 50;
    objects->controlInstructionsRect.w = instructionSurface->w;
    objects->controlInstructionsRect.h = instructionSurface->h;
    SDL_RenderTexture(objects->renderer, objects->controlInstructions, NULL, &objects->controlInstructionsRect);
    int y = objects->controlTitleRect.y + 150;
    for(int row = 0; row < 4; row ++){
        int x = objects->controlTitleRect.x - 30;

        for(int col = 0; col < 4; col++){
            int keyIndex = row * 4 + col;
            char keyPadCode[5];
            SDL_Color color = {255, 255, 255, 255}; //white

            if(chip->keyPad[keyIndex] != 0){
                color.b = 0;
                color.g = 0;
            }
            snprintf(keyPadCode, sizeof(keyPadCode), "%01X", keyIndex);

            SDL_Surface *keySurface = TTF_RenderText_Solid(objects->font, keyPadCode, strlen(keyPadCode), color); 
            if(keySurface == NULL){
                printf("Null surface on initControlPanel!\n");
                exit(1);
            }
            keyTextures[keyIndex].keyTexture = SDL_CreateTextureFromSurface(objects->renderer, keySurface);

             keyTextures[keyIndex].keyRect.x = x; 
             keyTextures[keyIndex].keyRect.y = y;
             keyTextures[keyIndex].keyRect.w = keySurface->w; 
             keyTextures[keyIndex].keyRect.h = keySurface->h; 

            SDL_RenderTexture(objects->renderer, keyTextures[keyIndex].keyTexture, NULL, &keyTextures[keyIndex].keyRect);

            xIncrement = keySurface->w + 50;

            x += xIncrement;
            SDL_DestroySurface(keySurface);
        }
        y += 60;
    }
    return 0;
}

int renderInstructionPanel(emulObjects objects, Chip8 *chip, int scalingFactor){
    SDL_RenderTexture(objects.renderer, objects.instructionPanelTitle, NULL, &objects.instructiontitleRect);

    TTF_SetFontSize(objects.font, 25.0);
    SDL_Color color = {255, 255, 255, 255};

    uint16_t secondPc = chip->pc - 2;
    const uint16_t currentOpcode = (chip->memory[secondPc] << 8) | (chip->memory[secondPc + 1]);
    char *currentInstruction = getLongerInstruction(currentOpcode, secondPc);

    SDL_Surface *surface = TTF_RenderText_Solid(objects.font, currentInstruction, strlen(currentInstruction), color);
    SDL_Texture *currentInstructionTexture = SDL_CreateTextureFromSurface(objects.renderer, surface);

    free(currentInstruction);

    const SDL_FRect rect = {
        0
            , objects.instructiontitleRect.h 
            , surface->w 
            , 60};

    SDL_RenderTexture(objects.renderer, currentInstructionTexture, NULL, &rect);

    SDL_DestroySurface(surface);
    SDL_DestroyTexture(currentInstructionTexture);

    return 0; 
}

int renderControlPanel(emulObjects *objects, Chip8 *chip){
    //set the target back to the renderer
    SDL_RenderTexture(objects->renderer, objects->controlsPanelTitle, NULL, &objects->controlTitleRect);

    SDL_RenderTexture(objects->renderer, objects->controlInstructions, NULL, &objects->controlInstructionsRect);
    //Render the title of the control panel
    int y = objects->controlTitleRect.y + 50;

    TTF_SetFontSize(objects->font, 60); 
    for(int row = 0; row < 4; row ++){
        int x = objects->controlTitleRect.x - 50;

        for(int col = 0; col < 4; col++){
            int keyIndex = row * 4 + col;
            //char keyPadCode[5];
            SDL_Color color = {128, 128, 128, 255}; //white
            keyTextures[keyIndex].color = color;

            if(chip->keyPad[keyIndex] != 0){
                SDL_SetRenderDrawColor(objects->renderer, 255, 0, 0, 255);
            }else{
                SDL_SetRenderDrawColor(objects->renderer, 128, 128, 128, 255);
            }
            SDL_RenderFillRect(objects->renderer, &keyTextures[keyIndex].keyRect);


            SDL_RenderTexture(objects->renderer, keyTextures[keyIndex].keyTexture, NULL, &keyTextures[keyIndex].keyRect);
            x += xIncrement;

        }
        y += 60;
    }
    return 0; 
}

int renderInternalPanel(emulObjects objects, Chip8 *chip){

    SDL_RenderTexture(objects.renderer, objects.internalsTitlePanel, NULL, &objects.internalTitleRect);

    return 0;
}
char *getLongerInstruction(const uint16_t currentOpcode, const uint16_t secondPc){
    char *message = malloc(60); // enough space for PC + description

    if (!message) return NULL;

    const uint8_t xRegIndex = (currentOpcode & 0x0F00) >> 8;
    const uint8_t yRegIndex = (currentOpcode & 0x00F0) >> 4;
    const uint8_t nn = currentOpcode & 0x00FF;
    const uint16_t nnn = currentOpcode & 0x0FFF;

    switch (currentOpcode & 0xF000) {
        case 0x0000:
            switch (currentOpcode & 0x0FFF) {
                case 0x00E0:
                    snprintf(message, 60, "0x%03X: CLR SCRN", secondPc);
                    break;
                case 0x00EE:
                    snprintf(message, 60, "0x%03X: RET", secondPc);
                    break;
                default:
                    snprintf(message, 60, "0x%03X: MCHN CD", secondPc);
                    break;
            }
            break;

        case 0x1000:
            snprintf(message, 60, "0x%03X: JMP 0x%03X", secondPc, nnn);
            break;

        case 0x2000:
            snprintf(message, 60, "0x%03X: CALL 0x%03X", secondPc, nnn);
            break;

        case 0x3000:
            snprintf(message, 60, "0x%03X: SKP V[%d] == 0x%02X", secondPc, xRegIndex, nn);
            break;

        case 0x4000:
            snprintf(message, 60, "0x%03X: SKP V[%d] != 0x%02X", secondPc, xRegIndex, nn);
            break;

        case 0x5000:
            snprintf(message, 60, "0x%03X: SKP V[%d] == V[%d]", secondPc, xRegIndex, yRegIndex);
            break;

        case 0x6000:
            snprintf(message, 60, "0x%03X: STR 0x%02X -> V[%d]", secondPc, nn, xRegIndex);
            break;

        case 0x7000:
            snprintf(message, 60, "0x%03X: ADD 0x%02X -> V[%d]", secondPc, nn, xRegIndex);
            break;

        case 0x8000:
            switch (currentOpcode & 0x000F) {
                case 0x0:
                    snprintf(message, 60, "0x%03X: V[%d] = V[%d]", secondPc, xRegIndex, yRegIndex);
                    break;
                case 0x1:
                    snprintf(message, 60, "0x%03X: V[%d] |= V[%d]", secondPc, xRegIndex, yRegIndex);
                    break;
                case 0x2:
                    snprintf(message, 60, "0x%03X: V[%d] &= V[%d]", secondPc, xRegIndex, yRegIndex);
                    break;
                case 0x3:
                    snprintf(message, 60, "0x%03X: V[%d] ^= V[%d]", secondPc, xRegIndex, yRegIndex);
                    break;
                case 0x4:
                    snprintf(message, 60, "0x%03X: V[%d] += V[%d]", secondPc, xRegIndex, yRegIndex);
                    break;
                case 0x5:
                    snprintf(message, 60, "0x%03X: V[%d] -= V[%d]", secondPc, xRegIndex, yRegIndex);
                    break;
                case 0x6:
                    snprintf(message, 60, "0x%03X: V[%d] >>= 1", secondPc, xRegIndex);
                    break;
                case 0x7:
                    snprintf(message, 60, "0x%03X: V[%d] - V[%d]", secondPc, xRegIndex, yRegIndex);
                    break;
                case 0xE:
                    snprintf(message, 60, "0x%03X: V[%d] <<= 1", secondPc, xRegIndex);
                    break;
                default:
                    snprintf(message, 60, "0x%03X: UNKNOWN OPCODE", secondPc);
                    break;
            }
            break;

        case 0x9000:
            snprintf(message, 60, "0x%03X: SKP V[%d] != V[%d]", secondPc, xRegIndex, yRegIndex);
            break;

        case 0xA000:
            snprintf(message, 60, "0x%03X: I = 0x%03X", secondPc, nnn);
            break;

        case 0xB000:
            snprintf(message, 60, "0x%03X: JMP 0x%03X + V[0]", secondPc, nnn);
            break;

        case 0xC000:
            snprintf(message, 60, "0x%03X: V[%d] = RAND & 0x%02X", secondPc, xRegIndex, nn);
            break;

        case 0xD000:
            snprintf(message, 60, "0x%03X: DRW V[%d], V[%d]", secondPc, xRegIndex, yRegIndex);
            break;

        case 0xE000:
            snprintf(message, 60, "0x%03X: SKP V[%d]", secondPc, xRegIndex);
            break;

        case 0xF000:
            snprintf(message, 60, "0x%03X: FX?? V[%d]", secondPc, xRegIndex);
            break;

        default:
            snprintf(message, 60, "0x%03X: UNKNOWN OPCODE", secondPc);
            break;
    }

    return message;
}
