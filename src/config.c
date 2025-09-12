#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL_log.h>

#include "../include/config.h"

Config *readConfiguration(const char *path){
    FILE *ptr = fopen(path, "r");
    if(ptr == NULL){
        perror("Error while trying to open the config file");
        exit(1);
    }
    Config *config = (Config *)malloc(sizeof(Config));
    char line[100];
    while(fgets(line, sizeof(line), ptr)){
        if(line[0] == '#' || strlen(line) < 3) continue; 
        if(strstr(line, "DEBUG_OUTPUT")) sscanf(line, "DEBUG_OUTPUT=%d", (int *)&config->debugOutput);
        else if(strstr(line, "SCALING_FACTOR")) sscanf(line, "SCALING_FACTOR=%hu", &config->scalingFactor);
    }
    fclose(ptr);
    //std::cout << "Number of elements: " << config->numberElements << "\n";
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Debug output: %d\n", config->debugOutput);
    //std::cout << "Window Width: " << config->windowWidth << "\n";
    SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO, "Scaling factor: %d\n", config->scalingFactor);

    return config;
}
