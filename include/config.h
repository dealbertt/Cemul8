#ifndef CONFIG_H
#define CONFIG_H
#include <stdbool.h>

typedef struct{
    bool debugOutput;
    short scalingFactor;
}Config;

Config *readConfiguration(const char *path);

#endif //CONFIG_H
