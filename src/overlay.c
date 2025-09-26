#include <stdio.h>
#include <stdlib.h>
#include "../include/overlay.h"


int renderInstructionPanel(emulObjects objects, Chip8 *chip, int scalingFactor){
    SDL_RenderTexture(objects.renderer, objects.instructionPanelTitle, NULL, &objects.instructiontitleRect);

    TTF_SetFontSize(objects.font, 25.0);
    SDL_Color color = {255, 255, 255, 255};

    uint16_t secondPc = chip->pc - 2;
    const uint16_t currentOpcode = (chip->memory[secondPc] << 8) | (chip->memory[secondPc + 1]);
    char *currentInstruction = getLongerInstruction(currentOpcode, secondPc);

    SDL_Surface *surface = TTF_RenderText_Solid(objects.font, currentInstruction, strlen(currentInstruction), color);
    SDL_Texture *currentInstructionTexture = SDL_CreateTextureFromSurface(objects.renderer, surface);

    SDL_DestroySurface(surface);
    free(currentInstruction);

    const SDL_FRect rect = {
        0
            , objects.instructiontitleRect.h 
            , (SCREEN_WIDTH * scalingFactor) / 4.0
            , 60};

    SDL_RenderTexture(objects.renderer, currentInstructionTexture, NULL, &rect);

    SDL_DestroyTexture(currentInstructionTexture);


    //Render the entire instruction panel

    //paint the border
                                                            
    return 0; 
}

int renderControlPanel(emulObjects objects, Chip8 *chip){
    //set the target back to the renderer
    SDL_RenderTexture(objects.renderer, objects.controlsPanelTitle, NULL, &objects.controlTitleRect);

    //Render the title of the control panel
    int y = objects.controlTitleRect.y + 50;

    TTF_SetFontSize(objects.font, 60); 
    for(int row = 0; row < 4; row ++){
        int x = objects.controlTitleRect.x - 50;

        for(int col = 0; col < 4; col++){
            int keyIndex = row * 4 + col;
            char keyPadCode[5];
            SDL_Color color = {255, 255, 255, 255}; //white

            if(chip->keyPad[keyIndex] != 0){
                color.b = 0;
                color.g = 0;
            }
            snprintf(keyPadCode, sizeof(keyPadCode), "%01X", keyIndex);

            SDL_Surface *keySurface = TTF_RenderText_Solid(objects.font, keyPadCode, strlen(keyPadCode), color); 
            SDL_Texture *keyTexture = SDL_CreateTextureFromSurface(objects.renderer, keySurface);

            const SDL_FRect keyRect = {
                    x
                    , y 
                    , keySurface->w 
                    , keySurface->h};

            SDL_RenderTexture(objects.renderer, keyTexture, NULL, &keyRect);
            x += keySurface->w + 50;

            SDL_DestroySurface(keySurface);
            SDL_DestroyTexture(keyTexture);
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
