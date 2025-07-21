#include <stdio.h>
#include "../include/chip8.h"
#include <SDL3/SDL.h>

int main(int argc, char **argv){
    if(VERBOSE_DEBUG_OUTPUT){
        printf("Verbose output enabled\n");
    }
    if(argc > 1){
        setFileName(argv[1]);
    }else{
        printf("Please select a file to load in the emulator!\n");
        return -1;
    }
    //some function to start the emulator
    return 0;
}
