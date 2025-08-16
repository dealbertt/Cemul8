#ifndef FUNCTIONS_H
#define FUNCTIONS_H

int setFileName(const char *argName);

int handleRealKeyboard();
unsigned char handleKeyPad();

unsigned char waitForPress();
unsigned char returnKeyEquivalent(const bool *pressed);
unsigned char checkHexValue();
 
unsigned char generateRandomNN(int mask);


#endif
