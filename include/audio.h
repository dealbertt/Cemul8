#ifndef AUDIO_H
#define AUDIO_H

#include "chip8.h"
#include <SDL3/SDL_audio.h>

typedef struct{
    SDL_AudioDeviceID device;
    SDL_AudioStream *stream;
    SDL_AudioSpec spec;

    uint8_t *wavBuffer;
    uint32_t wavLength;
}audioStruct;

int initializeAudio(Chip8 *chip);

void SDLCALL audioCallBack(void *userdata, SDL_AudioStream *stream, int additonalAmount, int totalAmoun8);
int playAudio(audioStruct *audio);
#endif //AUDIO_H
