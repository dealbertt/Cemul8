#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_surface.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "../include/chip8.h"
#include "../include/overlay.h"
#include "../include/functions.h"
#include "../include/audio.h"


Chip8 chip;
//CHIP-8 specifications
//I have no idea how to implement the pad, typeshit

//Flag for indicating when the screen needs to be updated
bool drawFlag = false;

uint8_t chip8_fontset[80] =
{ 
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

SDL_Scancode keyMap[16] = {
    SDL_SCANCODE_X, //0
    SDL_SCANCODE_1, //1
    SDL_SCANCODE_2, //2
    SDL_SCANCODE_3, //3
    SDL_SCANCODE_Q, //4
    SDL_SCANCODE_W, //5
    SDL_SCANCODE_E, //6
    SDL_SCANCODE_A, //7
    SDL_SCANCODE_S, //8
    SDL_SCANCODE_D, //9
    SDL_SCANCODE_Z, //A
    SDL_SCANCODE_C, //B
    SDL_SCANCODE_4, //C
    SDL_SCANCODE_R, //D
    SDL_SCANCODE_F, //E
    SDL_SCANCODE_V, //F
};

//0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
//0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
//0x200-0xFFF - Program ROM and work RAM


extern emulObjects objects;
extern Config *globalConfig;

void initialize(){

    chip.pc = 0x200; // Set program counter to 0x200
    chip.opcode = 0; // Reset op code
    chip.I = 0;      // Reset I
    chip.sp = 0;     // Reset stack pointer

    // Clear the display
    for (int i = 0; i < 2048; i++) {
        chip.gpx[i] = 0;
    }

    // Clear the stack, keypad, and V registers
    for (int i = 0; i < 16; i++) {
        chip.stack[i] = 0;
        chip.keyPad[i] = 0;
        chip.V[i] = 0;
    }

    // Clear chip.memory
    for (int i = 0; i < 4096; i++) {
        chip.memory[i] = 0;
    }

    // Load font set into chip.memory
    for (int i = 0; i < 80; i++) {
        chip.memory[i] = chip8_fontset[i];
    }

    // Reset timers
    chip.delay_timer = 0;
    chip.sound_timer = 0;
    
    initializeAudio(&chip);
}

int loadProgram(const char *fileName){
    FILE *ptr = fopen(fileName, "rb");
    if(ptr == NULL){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error while trying to open the loaded file!\n");
        return -1;
    }
    fseek(ptr, 0, SEEK_END); //Go to the end of the file

    long fileSize = ftell(ptr); //Use ftell to get the size of said file
    printf("Size of the file: %lu\n", fileSize);

    rewind(ptr); //Go back to the beggining of the file using rewind, first time i have ever heard of it

    if(fileSize > (MEMORY - 512)){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Program selected is too big to load on the emulator!\nPlease select a smaller program\n");
        fclose(ptr);
        return -1;
    }

    size_t bytesRead = fread(&chip.memory[512], 1, fileSize, ptr); //Return the total number of bytes read, which if everything goes fine, should be equal to the fileSize
    if(bytesRead != fileSize){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error trying to read the program\n");
        fclose(ptr);
        return -1;
    }

    fclose(ptr);
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Program loaded in chip.memory successfully!\n");
    return 0;
}

void simulateCpu(){
    initializeAllRendering(&objects, &chip);

    Uint64 frequency = SDL_GetPerformanceFrequency();
    Uint64 lastCycleTime = SDL_GetPerformanceCounter();
    Uint64 lastTimerTick = SDL_GetPerformanceCounter();

    const double cpuCycleMs = 2.0;                 // CPU cycle every 2 ms
    const double timerIntervalMs = 1000.0 / 60.0;  // 60 Hz timer

    objects.start = true;
    objects.keepGoing = true;

    while (objects.start) {
        Uint64 now = SDL_GetPerformanceCounter();
        double elapsedCycle = ((double)(now - lastCycleTime) / (double)frequency) * 1000;
        double elapsedTimer = ((double)(now - lastTimerTick) / (double)frequency) * 1000.0;

        // Handle CPU cycle
        if (objects.keepGoing && elapsedCycle >= cpuCycleMs) {
            emulateCycle();
            lastCycleTime = now;
        } else if (!objects.keepGoing && objects.executeOnce) {
            emulateCycle();
            objects.executeOnce = false;
            lastCycleTime = now;
        } else {
            // Prevent busy-waiting
            SDL_Delay(1);
        }

        // Handle timers
        if (elapsedTimer >= timerIntervalMs) {
            if (chip.delay_timer > 0) chip.delay_timer--;
            if (chip.sound_timer > 0) {
                chip.sound_timer--;
            }
            lastTimerTick = now;
        }
        // Handle input every loop iteration
        handleRealKeyboard();

       renderFrame();
    }
}



void emulateCycle(){
    chip.opcode = (chip.memory[chip.pc] << 8) | (chip.memory[chip.pc + 1]);

    //printf("Current instruction: %04X\n", chip.opcode);
    //printf("Future instruction to execute: %04X\n", (chip.memory[pc + 1]))

    //EXtract the nibbles of the chip.opcode
    uint8_t xRegIndex = (chip.opcode & 0x0F00) >> 8;
    uint8_t yRegIndex = (chip.opcode & 0x00F0) >> 4;

    uint8_t n = (chip.opcode & 0x000F); //low 4 bits
    uint8_t nn = (chip.opcode & 0x00FF); //low byte
    uint16_t nnn = (chip.opcode & 0x0FFF); //low 12 bits

    uint8_t x = chip.V[xRegIndex];
    uint8_t y = chip.V[yRegIndex]; 

    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Opcode: %04X\n", chip.opcode);

    //Increment before decode and execution to avoid code repetition
    chip.pc += 2;

    //DECODE
    switch(chip.opcode & 0xF000){        
        case 0x0000:
            switch(chip.opcode & 0x0FFF){
                case 0x00E0: //Clear the screen
                    for(int i = 0; i < 2048; i++){
                        chip.gpx[i] = 0x0;
                    }

                    drawFlag = true;
                    break;
                
                case 0x00EE: //Return from a subroutine
                    if(chip.sp >= 0){
                        chip.sp--; //Because we increased when pushing onto the stack to point to the next free slot, we now decrease to retreive the value that was pushed
                       chip.pc = chip.stack[chip.sp];
                    }else{
                        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: The stack pointer is less than 0!\nCurrent value of sp: %d\n", chip.sp);
                    }
                    break;
                default:
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unkonwn chip.opcode: 0x%04X\n", chip.opcode);
                    break;
            }

            break;
        case 0x1000: //jump to the address NNN
            chip.pc = nnn;
            break;

        case 0x2000: //calls subroutine at address NNN
            if(chip.sp < 16){
                chip.stack[chip.sp] = chip.pc;
                chip.pc = nnn;
                chip.sp++; 
            }else{
                SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: The stack pointer is greater than 15!\nCurrent value of sp: %d\n", chip.sp);
            }
            break;

        case 0x3000: //Skip the following instruction if the value of register VX equals NN
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, chip.V[xRegIndex]);
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of NN: %04X\n", nn);
            if(chip.V[xRegIndex] == nn){
                chip.pc += 2;
            }
            
            break;

        case 0x4000: //Skip the following instruction if the value of register VX is not equal to NN
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, chip.V[xRegIndex]);
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of NN: %04X\n", nn);
            if(chip.V[xRegIndex] != nn){
                chip.pc += 2;
            }
            
            break;

        case 0x5000: //Skip the following instruction if the value of register VX is equal to the value of register VY
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, chip.V[xRegIndex]);
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", yRegIndex, chip.V[yRegIndex]);
            if(chip.V[xRegIndex] == chip.V[yRegIndex]){
                 chip.pc += 2;
            }

            break;

        case 0x6000: //Store number NN in register VX
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, chip.V[xRegIndex]);
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of NN: %04X\n", nn);

            chip.V[xRegIndex] = nn;
            break;

        case 0x7000: //Add the value NN to register VX
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, chip.V[xRegIndex]);
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of NN: %04X\n", nn);

            chip.V[xRegIndex] += nn;
            break;

        case 0x8000:
            switch(chip.opcode & 0x000F){
                case 0x0000: //store the value of VY on VX
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, chip.V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", yRegIndex, chip.V[yRegIndex]);
                    chip.V[xRegIndex] = chip.V[yRegIndex];
                    break;

                case 0x0001: //Set VX to VX OR VY
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, chip.V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", yRegIndex, chip.V[yRegIndex]);
                    chip.V[0xF] = 0;
                    chip.V[xRegIndex] |= chip.V[yRegIndex];
                    break;

                case 0x0002: //Set VX to VX AND VY
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, chip.V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", yRegIndex, chip.V[yRegIndex]);
                    chip.V[0xF] = 0;
                    chip.V[xRegIndex] &= chip.V[yRegIndex];
                    break;

                case 0x0003: //Set VX to VX XOR VY
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, chip.V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", yRegIndex, chip.V[yRegIndex]);
                    chip.V[0xF] = 0;
                    chip.V[xRegIndex] ^= chip.V[yRegIndex];
                    break;


                case 0x0004: //Add the value of register VY to register VX
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, chip.V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", yRegIndex, chip.V[yRegIndex]);
                    if(chip.V[xRegIndex] + chip.V[yRegIndex] > 0xFF){
                        chip.V[0xF] = 1;
                    }else{
                        chip.V[0xF] = 0;
                    }
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[0xF]: %04X\n", chip.V[0xF]);

                    chip.V[xRegIndex] += chip.V[yRegIndex];
                    break;

                case 0x0005: //Subtract the value of register VY from register VX
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, chip.V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", yRegIndex, chip.V[yRegIndex]);
                    if(chip.V[yRegIndex] > chip.V[xRegIndex]){
                        chip.V[0xF] = 0;
                    }else{
                        chip.V[0xF] = 1;
                    }

                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of chip.V[0xF]: %04X\n", chip.V[0xF]);
                    chip.V[xRegIndex] -= chip.V[yRegIndex];
                    break;

                case 0x0006: //Store the value of register VY shifted right one bit in register VX
                    chip.V[0xF] = chip.V[xRegIndex] & 0x1;
                    chip.V[xRegIndex] = chip.V[yRegIndex];
                    chip.V[xRegIndex] >>= 1;


                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, chip.V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of chip.V[0xF]: %04X\n", chip.V[0xF]);
                    break;

                case 0x0007: //Set register VX to the value of VY minus VX
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, chip.V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", yRegIndex, chip.V[yRegIndex]);
                    if(chip.V[xRegIndex] > chip.V[yRegIndex]){
                        chip.V[0xF] = 0;
                    }else{
                        chip.V[0xF] = 1;
                    }
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of chip.V[0xF]: %04X\n", chip.V[0xF]);

                    chip.V[xRegIndex] = chip.V[yRegIndex] - chip.V[xRegIndex];
                    break;

                case 0x000E: //store the value of register vy shifted left one bit in register vx
                    chip.V[0xF] = chip.V[xRegIndex] >> 7; 
                    chip.V[xRegIndex] = chip.V[yRegIndex];
                    chip.V[xRegIndex] <<=  1;

                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, chip.V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of chip.V[0xF]: %04X\n", chip.V[0xF]);
                    break;
                default:
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unkonwn chip.opcode: 0x%04X\n", chip.opcode);
                    break;
            }
            break;
        case 0x9000://Skip the following instruction if the value of register VX is not equal to the value of register VY
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, chip.V[xRegIndex]);
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", yRegIndex, chip.V[yRegIndex]);
            if(chip.V[xRegIndex] != chip.V[yRegIndex]){
                 chip.pc += 2;
            }

            break;

        case 0xA000: //Store chip.memory address NNN in register I
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of I: %04X\n", chip.I);
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of NNN: %04X\n", nnn);

            chip.I = nnn; 
            break;

        case 0xB000: //Jump to address NNN + V0
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[0]: %04X\n", chip.V[0]);
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of NNN: %04X\n", nnn);

            chip.pc = nnn + chip.V[0];
            break;

        case 0xC000://Set VX Random number with a mask of NN
            chip.V[xRegIndex] = generateRandomNN(nn);
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n",xRegIndex, chip.V[xRegIndex]);
            break;

        case 0xD000://Draw a sprite at position VX, VY with N bytes of sprite data starting at the address stored in I 
            {
                uint8_t startX = x;
                uint8_t startY = y;
                uint8_t height = n;
                //static bool test = false;

                chip.V[0xF] = 0;
                for (int yline = 0; yline < height; yline++)
                {
                    uint8_t pixel = chip.memory[chip.I + yline];
                    for(int xline = 0; xline < 8; xline++)
                    {
                        bool pixelOn = (pixel >> (7 - xline)) & 1;
                        if(pixelOn)
                        {
                            int X = (startX + xline) % 64;
                            int Y = (startY + yline) % 32;
                            int idx = (Y * 64) + X;

                            //If even after wrapping, the coordinates are within bounds, we draw, otherwise we dont
                            if(X >= 0 && X < 64 && Y >= 0 && Y < 32){
                                if(chip.gpx[idx]){
                                    chip.V[0xF] = 1;
                                }

                                chip.gpx[idx] ^= 1;
                            }

                        }
                    }
                }
                /*
                if(x == 110 && y == 50){

                    printf("TEST STARTED\n");
                    test = true;
                }

                if((startX == 46 && startY == 18) || (startX == 28 && startY == 29) || (startX == 22 && startY == 2) || (startX == 34 && startY == 10)){
                    printf("WARNING! SPECIAL VALUE!\n");
                }
                printf("Drawing at X: %d and Y: %d\n", startX, startY);
                if(test){
                    printf("V[0]: %d\n", V[0]);
                    printf("V[1]: %d\n", V[1]);
                    printf("V[F]: %d\n", chip.V[0xF]);
                    printf("V[9]: %d\n", V[9]);
                }

                if(V[9] == 4){
                    printf("Test passed!");
                }

                if(x == 40 && y == 23 && test == true){
                    printf("TEST STOPPED\n");
                    test = false;
                }

                */
                drawFlag = true;
                break;
            }

        case 0xE000: 
            switch (chip.opcode & 0x000F) {
                case 0x000E: //Skip the following instruction if the key corresponding to the hex value currently stored in register VX is pressed
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n",xRegIndex, chip.V[xRegIndex]);
                    if(chip.keyPad[chip.V[xRegIndex]] != 0){
                         chip.pc += 2;
                    }                         

                    break;

                case 0x0001://Skip the following instruction if the key corresponding to the hex value currently stored in register VX is not pressed
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n",xRegIndex, chip.V[xRegIndex]);
                    if(chip.keyPad[chip.V[xRegIndex]] == 0){
                         chip.pc += 2;
                    }                         

                    break;

                default:
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unkonwn chip.opcode: 0x%04X\n", chip.opcode);
                    break;
            }
            break;

        case 0xF000:
            switch (chip.opcode & 0x00FF){
                case 0x0007://Store the current value of the  delay timer in register VX
                    chip.V[xRegIndex] = chip.delay_timer; 
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n",xRegIndex, chip.V[xRegIndex]);
                    break;

                case 0x000A://Wait for a keypress and store the result in register VX
                {
                    bool pressed = false;
                    for (int i = 0; i < 16; i++) {
                        if (chip.keyPad[i] == 0x1){
                            chip.V[xRegIndex] = i;      // Store key in Vx
                            pressed = true;
                            break;
                        }
                    }

                    if (!pressed) {
                        //pc stays the same, we stay on the same instruction
                        chip.pc -= 2;
                    }else{
                        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n",xRegIndex, chip.V[xRegIndex]);
                        return;
                    }
                    break;
                }

                case 0x0015://Set the delay timer to the value of register VX
                    chip.delay_timer = chip.V[xRegIndex]; 
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n",xRegIndex, chip.V[xRegIndex]);
                    break;

                case 0x0018://Set the sound timer to the value of register VX
                    chip.sound_timer = chip.V[xRegIndex]; 
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n",xRegIndex, chip.V[xRegIndex]);
                    break;

                case 0x001E: //Add the value stored in register VX to register I
                    if(chip.I + chip.V[xRegIndex] > 0xFFF){
                        chip.V[0xF] = 1;
                    }
                    else{
                        chip.V[0xF] = 0;
                    }
                    chip.I += chip.V[xRegIndex];
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n",xRegIndex, chip.V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of I: %04X\n", chip.I);
                    break;

                case 0x0029://Set I to the chip.memory address of the sprite data corresponding to the hexadecimal digit stored in register VX
                    chip.I = chip.V[xRegIndex] * 0x5;
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n",xRegIndex, chip.V[xRegIndex]);
                    break;

                case 0x0033://Store the binary-coded decimal equivalent of the value stored in register VX at addresses I, I + 1, and I + 2
                    chip.memory[chip.I] = chip.V[xRegIndex] / 100;
                    chip.memory[chip.I + 1] = (chip.V[xRegIndex] / 10) % 10;
                    chip.memory[chip.I + 2] = chip.V[xRegIndex]  % 10;

                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n",xRegIndex, chip.V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of chip.memory[I]: %04X\n", chip.memory[chip.I]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of chip.memory[I + 1]: %04X\n", chip.memory[chip.I + 1]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of chip.memory[I + 2]: %04X\n", chip.memory[chip.I + 2]);

                    break;

                    case 0x0055: //Store the values of registers V0 to VX inclusive in chip.memory starting at address I. I is set to I + X + 1 after operation²
                    for(int i = 0; i <= xRegIndex; i++){
                        chip.memory[chip.I + i] = chip.V[i];
                    }

                    chip.I += xRegIndex + 1;
                    break;

                case 0x0065: //Fill registers V0 to VX inclusive with the values stored in chip.memory starting at address I. I is set to I + X + 1 after operation²
                    for(int i = 0; i <= xRegIndex; i++){
                        chip.V[i] = chip.memory[chip.I + i];
                    }

                    chip.I += xRegIndex + 1;
                    break;
                default:
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unkonwn chip.opcode: 0x%04X\n", chip.opcode);
                    break;
            }
            break;

        default:
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unkonwn chip.opcode: 0x%04X\n", chip.opcode);
            break;
    } 

}

// CHIP-8 Keypad to Keyboard Mapping:
// CHIP-8: 1 2 3 C   -> Keyboard: 1 2 3 4
// CHIP-8: 4 5 6 D   -> Keyboard: Q W E R
// CHIP-8: 7 8 9 E   -> Keyboard: A S D F
// CHIP-8: A 0 B F   -> Keyboard: Z X C V
//

int handleRealKeyboard(){
    SDL_Event event;
    while(SDL_PollEvent(&event)){

        if(event.type == SDL_EVENT_QUIT){
            objects.start = false;
            return -1;
        }

        if(event.type == SDL_EVENT_KEY_DOWN){
            for(int i = 0; i < 16; i++){
                if(keyMap[i] == event.key.scancode){
                    chip.keyPad[i] = 0x1;
                }
            }

            if(event.key.scancode == SDL_SCANCODE_ESCAPE)
                objects.start= false;

            if(event.key.scancode == SDL_SCANCODE_F1)
                objects.keepGoing = !objects.keepGoing;

            if(event.key.scancode == SDL_SCANCODE_F2){
            }

            if(event.key.scancode == SDL_SCANCODE_F3){
            }

            if(event.key.scancode == SDL_SCANCODE_F4){
            }

            if(event.key.scancode == SDL_SCANCODE_F5){
                objects.keepGoing = !objects.keepGoing;
            }

            if(event.key.scancode == SDL_SCANCODE_F6){
                objects.executeOnce = true;
            }

        }

        if(event.type == SDL_EVENT_KEY_UP){
            for(int i = 0; i < 16; i++){
                if(keyMap[i] == event.key.scancode){
                    chip.keyPad[i] = 0x0;
                }
            }

        }
    }
    return 0;
}


//function in charge or rendering the entire screen (overlay + CHIP-8 screen)
int renderFrame(){
    //Clear the screen
    SDL_SetRenderDrawColor(objects.renderer, 0, 0, 0, 255); 
    SDL_RenderClear(objects.renderer);
    SDL_SetRenderDrawColor(objects.renderer, 255, 255, 255, 255); // white border

    //Rendering the chip-8 screen
    //update the CHIP-8 screen if needed
    if (drawFlag) {
        uint32_t pixels[2048];
        for (int i = 0; i < 2048; i++) {
            pixels[i] = chip.gpx[i] ? 0xFFFFFFFF : 0xFF000000;
        }

        SDL_UpdateTexture(objects.mainScreenTexture, NULL, pixels, 64 * sizeof(Uint32));
        drawFlag = false;
    }

    SDL_FRect mainWindowRect = {
             (SCREEN_WIDTH * globalConfig->scalingFactor) / 4.0
            , 0
            , (SCREEN_WIDTH * globalConfig->scalingFactor) / 2.0
            , (SCREEN_HEIGHT * globalConfig->scalingFactor) / 2.0};

    SDL_RenderTexture(objects.renderer, objects.mainScreenTexture, NULL, &mainWindowRect);
    SDL_SetRenderDrawColor(objects.renderer, 255, 255, 255, 255); // white border
    SDL_RenderRect(objects.renderer, &mainWindowRect);

    //The entire instruction panel
    renderInstructionPanel(&objects, &chip, globalConfig->scalingFactor);

    //The keypad Control panel
    renderControlPanel(&objects, &chip);

    renderInternalPanel(&objects, &chip);

    renderTimerPanel(&objects, &chip);

    renderIndexPanel(&objects, &chip);
    //Border for the instruction Panel
    //Border for the chip8 screen

    //Border of the title
    //SDL_SetRenderDrawColor(objects.renderer, 255, 255, 255, 255); // white border
    //SDL_RenderRect(objects.renderer, &objects.instructiontitleRect);
    //SDL_SetRenderDrawColor(objects.renderer, 0, 0, 0, 255); 

    //finally update the screen :)
    SDL_RenderPresent(objects.renderer);
    return 0;
}




