#ifndef FUNCTIONS_H
#define FUNCTIONS_H

int setFileName(const char *argName);

int handleKeyboard();

unsigned char waitForPress();
unsigned char returnKeyEquivalent(const bool *pressed);
 
unsigned char generateRandomNN(int mask);


#endif
