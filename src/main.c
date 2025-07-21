#include "../include/chip8.h"
#include <SDL3/SDL.h>

int main(int argc, char **argv){
    if(argc > 1){
        setFileName(argv[1]);
    }
    return 0;
}
