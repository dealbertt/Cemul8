#include <SDL3/SDL_error.h>
#include <SDL3/SDL_log.h>
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
    SDL_FRect instRect;
    uint16_t designatedOpcode;
}instructionTexture;

typedef struct{
    SDL_Texture *indexTexture;
    SDL_Texture *valueTexture;

    SDL_FRect indexRect;
    SDL_FRect valueRect;

    uint8_t previousValue;
}registerTexture;


SDL_Texture *hexCharTextures[256];

int xIncrement;

keyPadTexture keyTextures[16];

instructionTexture preRenderedInstructions[MEMORY - 0x200 + 1];

registerTexture registerArray[16];

int initializeAllRendering(emulObjects *objects, Chip8 *chip){
    initControlPanel(objects, chip);
    initRegisterPanel(objects, chip);
    preRenderInstructions(objects, chip);
    return 0;
}

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
        SDL_DestroyTexture(objects->mainScreenTexture);
        return -1;
    }

    SDL_Color color = {255, 255, 255, 255};


    char instructionTitle[15] = "INSTRUCTIONS";

    SDL_Surface *titleSurface = TTF_RenderText_Solid(objects->font, instructionTitle, strlen(instructionTitle), color);
    if(titleSurface == NULL){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error trying create titleSurface: %s\n", SDL_GetError());

        SDL_DestroyTexture(objects->mainScreenTexture);
        TTF_CloseFont(objects->font);
        return -1;

    }

    objects->instructionPanelTitle = SDL_CreateTextureFromSurface(objects->renderer, titleSurface);
    if(objects->instructionPanelTitle == NULL){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error trying create instructionPanelTitle: %s\n", SDL_GetError());

        SDL_DestroyTexture(objects->mainScreenTexture);
        TTF_CloseFont(objects->font);
        SDL_DestroySurface(titleSurface);
        return -1;
    }


    objects->instructiontitleRect.x = 0;
    objects->instructiontitleRect.y = 0;
    objects->instructiontitleRect.w = titleSurface->w;
    objects->instructiontitleRect.h = titleSurface->h;
    SDL_RenderTexture(objects->renderer, objects->instructionPanelTitle, NULL, &objects->instructiontitleRect);
    SDL_DestroySurface(titleSurface);

    char controlTitleString[15] = "CONTROLS";

    SDL_Surface *controlSurface = TTF_RenderText_Solid(objects->font, controlTitleString, strlen(controlTitleString), color);
    if(controlSurface == NULL){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error trying create controlSurface: %s\n", SDL_GetError());

        SDL_DestroyTexture(objects->mainScreenTexture);
        TTF_CloseFont(objects->font);
        SDL_DestroySurface(titleSurface);
        return -1;
    }

    objects->controlsPanelTitle = SDL_CreateTextureFromSurface(objects->renderer, controlSurface);
    if(objects->controlsPanelTitle == NULL){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error trying create controlsPanelTitle: %s\n", SDL_GetError());

        SDL_DestroyTexture(objects->mainScreenTexture);
        TTF_CloseFont(objects->font);
        SDL_DestroySurface(titleSurface);
        SDL_DestroySurface(controlSurface);
        return -1;
    }


    objects->controlTitleRect.x = (SCREEN_WIDTH * scalingFactor) / 2.0 - 100;
    objects->controlTitleRect.y = (SCREEN_HEIGHT * scalingFactor) / 2.0;
    objects->controlTitleRect.w = controlSurface->w;
    objects->controlTitleRect.h = controlSurface->h;
    SDL_RenderTexture(objects->renderer, objects->controlsPanelTitle, NULL, &objects->controlTitleRect);
    SDL_DestroySurface(controlSurface);

    char internalTitle[15] = "REGISTERS";
    SDL_Surface *internalSurface = TTF_RenderText_Solid(objects->font, internalTitle, strlen(internalTitle), color);
    if(internalSurface == NULL){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error trying create internalSurface: %s\n", SDL_GetError());

        SDL_DestroyTexture(objects->mainScreenTexture);
        TTF_CloseFont(objects->font);
        SDL_DestroySurface(titleSurface);
        SDL_DestroySurface(controlSurface);
        return -1;
    }
    objects->internalsTitlePanel = SDL_CreateTextureFromSurface(objects->renderer, internalSurface);
    if(objects->internalsTitlePanel == NULL){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error trying create internalsTitlePanel: %s\n", SDL_GetError());

        SDL_DestroyTexture(objects->mainScreenTexture);
        TTF_CloseFont(objects->font);
        SDL_DestroySurface(titleSurface);
        SDL_DestroySurface(controlSurface);
        SDL_DestroySurface(internalSurface);
        return -1;
    }

    objects->internalTitleRect.x = (SCREEN_WIDTH * scalingFactor) * 0.75;
    objects->internalTitleRect.y = 0;
    objects->internalTitleRect.w = internalSurface->w;
    objects->internalTitleRect.h = internalSurface->h;

    SDL_RenderTexture(objects->renderer, objects->internalsTitlePanel, NULL, &objects->internalTitleRect);
    SDL_DestroySurface(internalSurface);
    return 0;
}

int initControlPanel(emulObjects *objects, Chip8 *chip){
    //Render the instructions for the control panel
    SDL_Color color = {255, 255, 255, 255};
    char instructions[150] = "F1: STOP/RESUME EXECUTION | F6: EXECUTE ONE INSTRUCTION";
    TTF_SetFontSize(objects->font, 25);
    SDL_Surface *instructionSurface = TTF_RenderText_Solid(objects->font, instructions, strlen(instructions), color);
    if(instructionSurface == NULL){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error trying create internalsTitlePanel: %s\n", SDL_GetError());
        return -1;
    }

    objects->controlInstructions = SDL_CreateTextureFromSurface(objects->renderer, instructionSurface);

    objects->controlInstructionsRect.x = (SCREEN_WIDTH * 25) / 4.0;
    objects->controlInstructionsRect.y = objects->controlTitleRect.y + 50;
    objects->controlInstructionsRect.w = instructionSurface->w;
    objects->controlInstructionsRect.h = instructionSurface->h;
    SDL_RenderTexture(objects->renderer, objects->controlInstructions, NULL, &objects->controlInstructionsRect);
    SDL_DestroySurface(instructionSurface);
    //---------------------------
    //
    //Render the keypad for the first time 
    
    TTF_SetFontSize(objects->font, 40);
    int y = objects->controlTitleRect.y + 100;
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
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error creating keySurface: %s\n", SDL_GetError());
                return -1;
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

int initRegisterPanel(emulObjects *objects, Chip8 *chip){
    TTF_SetFontSize(objects->font, 25);

    SDL_Color color = {255, 255, 255, 255};

    int y = objects->internalTitleRect.h + 20;

    initRegisterTextures(objects->renderer, objects->font);
    for(int row = 0; row < 4; row++){
        int x = objects->internalTitleRect.x;
        for(int col = 0; col < 4; col++){
            //Render the index Textures
            int regIndex = row * 4 + col;
            char indexStr[10];
            snprintf(indexStr, sizeof(indexStr), "V%01X: ", regIndex);
            SDL_Surface *indexSurface = TTF_RenderText_Solid(objects->font, indexStr, strlen(indexStr), color);

            registerArray[regIndex].indexTexture = SDL_CreateTextureFromSurface(objects->renderer, indexSurface);
            registerArray[regIndex].indexRect.x = x; 
            registerArray[regIndex].indexRect.y = y; 
            registerArray[regIndex].indexRect.w = indexSurface->w; 
            registerArray[regIndex].indexRect.h = indexSurface->h; 


            SDL_RenderTexture(objects->renderer, registerArray[regIndex].indexTexture, NULL, &registerArray[regIndex].indexRect);
            //---------------------
            registerArray[regIndex].valueRect.x = registerArray[regIndex].indexRect.x + 50;
            registerArray[regIndex].valueRect.y = registerArray[regIndex].indexRect.y;
            registerArray[regIndex].valueRect.w = 30;
            registerArray[regIndex].valueRect.h = 30;
            registerArray[regIndex].previousValue = -1;



            //Render the value Textures
            SDL_DestroySurface(indexSurface);

            x += 100;
        }
        y += 50;
    }
    return 0;
}

int initRegisterTextures(SDL_Renderer *renderer, TTF_Font *font){
    SDL_Color color = {255, 255, 255, 255};
    TTF_SetFontSize(font, 25);

    for(int i = 0; i < 256; i++){
        char hex[4];
        snprintf(hex, sizeof(hex), "%02X", i);
        
        SDL_Surface *hexSurface = TTF_RenderText_Solid(font, hex, strlen(hex), color);
        if(hexSurface == NULL){
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error trying to create hexSurface: %s", SDL_GetError()); 
                return -1;
        }

        hexCharTextures[i] = SDL_CreateTextureFromSurface(renderer, hexSurface);
        if(hexCharTextures[i] == NULL){
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error trying to create hexCharTextures[%d]: %s", i, SDL_GetError()); 
                return -1;
        }
    }
    return 0;
}

int renderInternalPanel(emulObjects *objects, Chip8 *chip){

    SDL_RenderTexture(objects->renderer, objects->internalsTitlePanel, NULL, &objects->internalTitleRect);
    for(int row = 0; row < 4; row++){
        for(int col = 0; col < 4; col++){
            int regIndex = row * 4 + col;

            SDL_RenderTexture(objects->renderer, registerArray[regIndex].indexTexture, NULL, &registerArray[regIndex].indexRect);

            if(chip->V[regIndex] != registerArray[regIndex].previousValue){
                // i have to update the value
                SDL_RenderTexture(objects->renderer,
                  hexCharTextures[chip->V[regIndex]],
                  NULL,
                  &registerArray[regIndex].valueRect);
            }
        }
    }
    return 0;
}

int renderInstructionPanel(emulObjects *objects, Chip8 *chip, int scalingFactor){
    SDL_RenderTexture(objects->renderer, objects->instructionPanelTitle, NULL, &objects->instructiontitleRect);
    int y = objects->instructiontitleRect.h;
    for(int i = 0; i < 40; i += 2){
        preRenderedInstructions[((chip->pc - 0x200) / 2) + i].instRect.y = y;
        SDL_RenderTexture(objects->renderer, preRenderedInstructions[((chip->pc - 0x200) / 2) + i].instTexture, NULL, &preRenderedInstructions[((chip->pc - 0x200) / 2 ) + i].instRect);
        y += 40;
    }
    return 0; 
}

int preRenderInstructions(emulObjects *objects, Chip8 *chip){
    SDL_Color color = {255, 255, 255, 255};
    TTF_SetFontSize(objects->font, 25.0);
    //chip->pc = 0x200
    for(uint16_t pc = 0x200; pc < MEMORY - 1; pc += 2){
        const uint16_t opcode = (chip->memory[pc] << 8) | (chip->memory[pc + 1]);
        char *instruction = getLongerInstruction(opcode, pc); 

        SDL_Surface *surface = TTF_RenderText_Solid(objects->font, instruction, strlen(instruction), color);
        preRenderedInstructions[(pc - 0x200) / 2].instTexture = SDL_CreateTextureFromSurface(objects->renderer, surface);      
        preRenderedInstructions[(pc - 0x200) / 2].instRect.w = surface->w;
        preRenderedInstructions[(pc - 0x200) / 2].instRect.h = surface->h;
        
        SDL_DestroySurface(surface);
        free(instruction);
    }
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
