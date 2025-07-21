#include <stdio.h>
#include "../include/chip8.h"
#include <SDL3/SDL.h>
#include <string.h>

Config globalConfig = {.debugOutput = false};


extern char fileName[20];
int main(int argc, char **argv){
    for(int i = 0; i < argc; i++){
        if(strcmp(argv[i], "-DEBUG_OUTPUT") == 0){
            printf("Verbose output enabled\n");
            globalConfig.debugOutput = true;
        }
    }
    if(argc > 1){
        setFileName(argv[1]);
    }else{
        printf("Please select a file to load in the emulator!\n");
        return -1;
    }
    loadProgram(fileName);
    return 0;
}
