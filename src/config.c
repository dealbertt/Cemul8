#include <stdio.h>

#include <stdlib.h>
#include <SDL3/SDL_log.h>

#include "../include/config.h"

Config *readConfiguration(const char *path, emulObjects *objects){
    FILE *ptr = fopen(path, "r");
    if(ptr == NULL){
        perror("Error while trying to open the config file");
        return NULL;
    }
    Config *config = malloc(sizeof(Config));
    char line[100];
    while(fgets(line, sizeof(line), ptr)){
        if(line[0] == '#' || strlen(line) < 3) continue; 
        if(strstr(line, "DEBUG_OUTPUT")) sscanf(line, "DEBUG_OUTPUT=%d", (int *)&config->debugOutput);
        else if(strstr(line, "COLOR")) sscanf(line, "COLOR=%hu", &config->color);
    }
    fclose(ptr);
    if(config->debugOutput == 1){
        SDL_SetLogPriorities(SDL_LOG_PRIORITY_DEBUG);
    }else{
        SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
    }
     
    switch(config->color){
        case 0:
            objects->color.r = 255;
            objects->color.b = 255;
            objects->color.g = 255;
            objects->color.a = 255;
            break;
        case 1:
            objects->color.r = 128;
            objects->color.b = 128;
            objects->color.g = 128;
            objects->color.a = 255;
            break;
        case 2:
            objects->color.r = 50;
            objects->color.b = 50;
            objects->color.g = 255;
            objects->color.a = 255;
            break;
        case 3:
            objects->color.r = 50;
            objects->color.b = 255;
            objects->color.g = 50;
            objects->color.a = 255;
            break;
        case 4:
            objects->color.r = 255;
            objects->color.b = 50;
            objects->color.g = 50;
            objects->color.a = 255;
            break;

        default:
            objects->color.r = 255;
            objects->color.b = 255;
            objects->color.g = 255;
            objects->color.a = 255;
            break;

    }
     
    //std::cout << "Number of elements: " << config->numberElements << "\n";
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Debug output: %d\n", config->debugOutput);
    //std::cout << "Window Width: " << config->windowWidth << "\n";
    SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO, "Scaling factor: %d\n", config->scalingFactor);

    config->scalingFactor = 25;
    return config;
}
