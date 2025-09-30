#include <SDL3/SDL_stdinc.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_log.h>

#include "../include/audio.h"
#include "../include/chip8.h"

typedef struct{
    Chip8 *chip; 
    int phase;
}audioData;

int initializeAudio(Chip8 *chip){
    audioData *audio = malloc(sizeof(audioData));

    audio->chip = chip;
    audio->phase = 0; 

    SDL_AudioSpec spec;
    SDL_zero(spec);

    spec.freq = 44100;
    spec.format = SDL_AUDIO_U8;
    spec.channels = 1;

    SDL_AudioDeviceID device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    SDL_AudioStream *stream = SDL_CreateAudioStream(&spec, &spec);
    SDL_SetAudioStreamGetCallback(stream, audioCallBack, audio);
    SDL_BindAudioStream(device, stream);

    SDL_ResumeAudioDevice(device);
    return 0;
}
void SDLCALL audioCallBack(void *userdata, SDL_AudioStream *stream, int additonalAmount, int totalAmount){
    audioData *audio = (audioData *)userdata;

    int samples = additonalAmount;
    if(samples == 0) return;

    Uint8 *buffer = SDL_malloc(samples);
    if(!buffer) return;

    if(audio->chip->sound_timer > 0){
         for (int i = 0; i < samples; i++) {
            audio->phase++;
            int period = 44100 / 440; 
            buffer[i] = (audio->phase % period) < (period / 2) ? 0xFF : 0x00;
        }
    }else{
        SDL_memset(buffer, 0, samples);
    }

    SDL_PutAudioStreamData(stream, buffer, samples);
    SDL_free(buffer);
}

