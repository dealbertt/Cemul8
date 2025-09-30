#ifndef AUDIO_H
#define AUDIO_H

#include <SDL3/SDL_audio.h>

void SDLCALL audioCallBack(void *userData, SDL_AudioStream *stream, int additionalAmount, int totalAmount);


int playAudio();
#endif //AUDIO_H
