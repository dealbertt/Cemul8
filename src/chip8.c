#include <SDL3/SDL_error.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_surface.h>
#include <SDL3_ttf/SDL_ttf.h>
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

#include "../include/chip8.h"
#include "../include/config.h"
#include "../include/functions.h"


//CHIP-8 specifications
uint16_t opcode; //the operation code of the instruction
uint8_t memory[MEMORY]; //the total memory of the chip-8
uint8_t V[16]; //all the general purpose registers

uint16_t I; //special register I for memory addresses
uint16_t pc; //Program counter


uint8_t gpx[SCREEN_WIDTH * SCREEN_HEIGHT]; //pixels of the screen. Total pixels in the array: 2048
                                                //Keep in mind that this are the screens of the CHIP-8, but not of the actual screen that is showed, that is because the original resolution is way too small

char instructionDescription[25];

uint8_t delay_timer;
uint8_t sound_timer;

//STACK
uint16_t stack[16];
uint16_t sp; //stack pointer
             //
uint8_t keyPad[16];
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
    pc = 0x200; // Set program counter to 0x200
    opcode = 0; // Reset op code
    I = 0;      // Reset I
    sp = 0;     // Reset stack pointer

    // Clear the display
    for (int i = 0; i < 2048; i++) {
        gpx[i] = 0;
    }

    // Clear the stack, keypad, and V registers
    for (int i = 0; i < 16; i++) {
        stack[i] = 0;
        keyPad[i] = 0;
        V[i] = 0;
    }

    // Clear memory
    for (int i = 0; i < 4096; i++) {
        memory[i] = 0;
    }

    // Load font set into memory
    for (int i = 0; i < 80; i++) {
        memory[i] = chip8_fontset[i];
    }

    // Reset timers
    delay_timer = 0;
    sound_timer = 0;

}

int loadProgram(const char *fileName){
    FILE *ptr = fopen(fileName, "rb");
    if(ptr == NULL){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error while trying to open the loaded file!\n");
        return -1;
    }
    fseek(ptr, 0, SEEK_END); //Go to the end of the file

    long fileSize = ftell(ptr); //Use ftell to get the size of said file

    rewind(ptr); //Go back to the beggining of the file using rewind, first time i have ever heard of it

    if(fileSize > (MEMORY - 512)){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Program selected is too big to load on the emulator!\nPlease select a smaller program\n");
        fclose(ptr);
        return -1;
    }

    size_t bytesRead = fread(&memory[512], 1, fileSize, ptr); //Return the total number of bytes read, which if everything goes fine, should be equal to the fileSize
    if(bytesRead != fileSize){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error trying to read the program\n");
        fclose(ptr);
        return -1;
    }

    fclose(ptr);
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Program loaded in memory successfully!\n");
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: First few bytes of program: %02X %02X %02X %02X\n", memory[0x200], memory[0x201], memory[0x202], memory[0x203]);
    return 0;
}

void simulateCpu(){
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

        // Handle input every loop iteration
        handleRealKeyboard();

        // Handle timers
        if (elapsedTimer >= timerIntervalMs) {
            if (delay_timer > 0) delay_timer--;
            if (sound_timer > 0) {
                sound_timer--;
                if (sound_timer == 0) {
                    printf("Beep!\n");
                }
            }
            lastTimerTick = now;
        }
        renderFrame();
    }
}



void emulateCycle(){
    opcode = (memory[pc] << 8) | (memory[pc + 1]);

    //printf("Current instruction: %04X\n", opcode);
    //printf("Future instruction to execute: %04X\n", (memory[pc + 1]))
    //empty the string

    //Resetting the instructionDescription
    memset(instructionDescription, 0, sizeof(instructionDescription));

    //EXtract the nibbles of the opcode
    uint8_t xRegIndex = (opcode & 0x0F00) >> 8;
    uint8_t yRegIndex = (opcode & 0x00F0) >> 4;

    uint8_t n = (opcode & 0x000F); //low 4 bits
    uint8_t nn = (opcode & 0x00FF); //low byte
    uint16_t nnn = (opcode & 0x0FFF); //low 12 bits

    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Opcode: %04X\n", opcode);

    //Increment before decode and execution to avoid code repetition
    pc += 2;

    //DECODE
    switch(opcode & 0xF000){        
        case 0x0000:
            switch(opcode & 0x0FFF){
                case 0x00E0: //Clear the screen
                             //
                    snprintf(instructionDescription, sizeof(instructionDescription), "CLEAR SCREEN");
                    for(int i = 0; i < 2048; i++){
                        gpx[i] = 0x0;
                    }

                    drawFlag = true;
                    break;
                
                case 0x00EE: //Return from a subroutine
                    snprintf(instructionDescription, sizeof(instructionDescription), "RETURN");
                    if(sp >= 0){
                        sp--; //Because we increased when pushing onto the stack to point to the next free slot, we now decrease to retreive the value that was pushed
                        pc = stack[sp];
                    }else{
                        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: The stack pointer is less than 0!\nCurrent value of sp: %d\n", sp);
                    }
                    break;
                default:
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unkonwn opcode: 0x%04X\n", opcode);
                    break;
            }

            break;
        case 0x1000: //jump to the address NNN
            snprintf(instructionDescription, sizeof(instructionDescription), "JUMP 0x%03X", nnn);
            pc = nnn;
            break;

        case 0x2000: //calls subroutine at address NNN
            snprintf(instructionDescription, sizeof(instructionDescription), "CALL 0x%03X", nnn);
            if(sp < 16){
                stack[sp] = pc;
                pc = nnn;
                sp++; 
            }else{
                SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: The stack pointer is greater than 15!\nCurrent value of sp: %d\n", sp);
            }
            break;

        case 0x3000: //Skip the following instruction if the value of register VX equals NN
            snprintf(instructionDescription, sizeof(instructionDescription), "SKP V[%d] == 0x%02X", xRegIndex, nn);

            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, V[xRegIndex]);
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of NN: %04X\n", nn);
            if(V[xRegIndex] == nn){
                pc += 2;
            }
            
            break;

        case 0x4000: //Skip the following instruction if the value of register VX is not equal to NN
            snprintf(instructionDescription, sizeof(instructionDescription), "SKP V[%d] != 0x%02X", xRegIndex, nn);

            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, V[xRegIndex]);
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of NN: %04X\n", nn);
            if(V[xRegIndex] != nn){
                pc += 2;
            }
            
            break;

        case 0x5000: //Skip the following instruction if the value of register VX is equal to the value of register VY
            snprintf(instructionDescription, sizeof(instructionDescription), "SKP V[%d] != V[%d]", xRegIndex, yRegIndex);

            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, V[xRegIndex]);
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", yRegIndex, V[yRegIndex]);
            if(V[xRegIndex] == V[yRegIndex]){
                pc += 2;
            }

            break;

        case 0x6000: //Store number NN in register VX
            snprintf(instructionDescription, sizeof(instructionDescription), "STR 0x%02X -> V[%d]", nn, xRegIndex);

            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, V[xRegIndex]);
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of NN: %04X\n", nn);

            V[xRegIndex] = nn;
            break;

        case 0x7000: //Add the value NN to register VX
            snprintf(instructionDescription, sizeof(instructionDescription), "ADD 0x%02X -> V[%d]", nn, xRegIndex);

            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, V[xRegIndex]);
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of NN: %04X\n", nn);

            V[xRegIndex] += nn;
            break;

        case 0x8000:
            switch(opcode & 0x000F){
                case 0x0000: //store the value of VY on VX
                    snprintf(instructionDescription, sizeof(instructionDescription), "ADD V[%d] -> V[%d]", yRegIndex, xRegIndex);

                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", yRegIndex, V[yRegIndex]);
                    V[xRegIndex] = V[yRegIndex];
                    break;

                case 0x0001: //Set VX to VX OR VY
                    snprintf(instructionDescription, sizeof(instructionDescription), "V[%d] OR V[%d]", xRegIndex, yRegIndex);

                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", yRegIndex, V[yRegIndex]);
                    V[0xF] = 0;
                    V[xRegIndex] |= V[yRegIndex];
                    break;

                case 0x0002: //Set VX to VX AND VY
                    snprintf(instructionDescription, sizeof(instructionDescription), "V[%d] AND V[%d]", xRegIndex, yRegIndex);

                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", yRegIndex, V[yRegIndex]);
                    V[0xF] = 0;
                    V[xRegIndex] &= V[yRegIndex];
                    break;

                case 0x0003: //Set VX to VX XOR VY
                    snprintf(instructionDescription, sizeof(instructionDescription), "V[%d] XOR V[%d]", xRegIndex, yRegIndex);

                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", yRegIndex, V[yRegIndex]);
                    V[0xF] = 0;
                    V[xRegIndex] ^= V[yRegIndex];
                    break;


                case 0x0004: //Add the value of register VY to register VX
                    snprintf(instructionDescription, sizeof(instructionDescription), "V[%d] += V[%d]", xRegIndex, yRegIndex);

                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", yRegIndex, V[yRegIndex]);
                    if(V[xRegIndex] + V[yRegIndex] > 0xFF){
                        V[0xF] = 1;
                    }else{
                        V[0xF] = 0;
                    }
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[0xF]: %04X\n", V[0xF]);

                    V[xRegIndex] += V[yRegIndex];
                    break;

                case 0x0005: //Subtract the value of register VY from register VX
                    snprintf(instructionDescription, sizeof(instructionDescription), "V[%d] -= V[%d]", xRegIndex, yRegIndex);

                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", yRegIndex, V[yRegIndex]);
                    if(V[yRegIndex] > V[xRegIndex]){
                        V[0xF] = 0;
                    }else{
                        V[0xF] = 1;
                    }

                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[0xF]: %04X\n", V[0xF]);
                    V[xRegIndex] -= V[yRegIndex];
                    break;

                case 0x0006: //Store the value of register VY shifted right one bit in register VX
                    snprintf(instructionDescription, sizeof(instructionDescription), "V[%d] >>= 1", xRegIndex);

                    V[0xF] = V[xRegIndex] & 0x1;
                    V[xRegIndex] = V[yRegIndex];
                    V[xRegIndex] >>= 1;


                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[0xF]: %04X\n", V[0xF]);
                    break;

                case 0x0007: //Set register VX to the value of VY minus VX
                    snprintf(instructionDescription, sizeof(instructionDescription), "V[%d] - V[%d]", yRegIndex, xRegIndex);

                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", yRegIndex, V[yRegIndex]);
                    if(V[xRegIndex] > V[yRegIndex]){
                        V[0xF] = 0;
                    }else{
                        V[0xF] = 1;
                    }
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[0xF]: %04X\n", V[0xF]);

                    V[xRegIndex] = V[yRegIndex] - V[xRegIndex];
                    break;

                case 0x000E: //store the value of register vy shifted left one bit in register vx
                    snprintf(instructionDescription, sizeof(instructionDescription), "V[%d] <<= 1", xRegIndex);

                    V[0xF] = V[xRegIndex] >> 7; 
                    V[xRegIndex] = V[yRegIndex];
                    V[xRegIndex] <<=  1;

                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[0xF]: %04X\n", V[0xF]);
                    break;
                default:
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unkonwn opcode: 0x%04X\n", opcode);
                    break;
            }
            break;
        case 0x9000://Skip the following instruction if the value of register VX is not equal to the value of register VY
            snprintf(instructionDescription, sizeof(instructionDescription), "SKP V[%d] != V[%d]", xRegIndex, yRegIndex);

            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", xRegIndex, V[xRegIndex]);
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n", yRegIndex, V[yRegIndex]);
            if(V[xRegIndex] != V[yRegIndex]){
                pc += 2;
            }

            break;

        case 0xA000: //Store memory address NNN in register I
            snprintf(instructionDescription, sizeof(instructionDescription), "STR %04X -> I", xRegIndex);
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of I: %04X\n", I);
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of NNN: %04X\n", nnn);

            I = nnn; 
            break;

        case 0xB000: //Jump to address NNN + V0
            snprintf(instructionDescription, sizeof(instructionDescription), "JMP %04X + V[0]", xRegIndex);
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[0]: %04X\n", V[0]);
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of NNN: %04X\n", nnn);

            pc = nnn + V[0];
            break;

        case 0xC000://Set VX Random number with a mask of NN
            snprintf(instructionDescription, sizeof(instructionDescription), "SET V[%d]", xRegIndex);

            V[xRegIndex] = generateRandomNN(nn);
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n",xRegIndex, V[xRegIndex]);
            break;

        case 0xD000://Draw a sprite at position VX, VY with N bytes of sprite data starting at the address stored in I 
            
            {
                snprintf(instructionDescription, sizeof(instructionDescription), "DRW V[%d], V[%d]", xRegIndex, yRegIndex);

                uint16_t x = V[xRegIndex] & 63;
                uint16_t y = V[yRegIndex] & 31;
                uint16_t height = n;


                V[0xF] = 0;
                for (int yline = 0; yline < height; yline++)
                {
                    uint16_t pixel = memory[I + yline];
                    for(int xline = 0; xline < 8; xline++)
                    {
                        if((pixel & (0x80 >> xline)) != 0)
                        {
                            int X = (x + xline) % 64;
                            int Y = (y + yline) % 32;
                            int idx = X  + (Y * 64);

                            if(gpx[idx] == 1)
                                V[0xF] = 1;

                            gpx[idx] ^= 1;
                        }
                    }
                }

                drawFlag = true;
                break;
            }
        case 0xE000: 
            switch (opcode & 0x000F) {
                case 0x000E: //Skip the following instruction if the key corresponding to the hex value currently stored in register VX is pressed
                    snprintf(instructionDescription, sizeof(instructionDescription), "SKP V[%d]", xRegIndex);

                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n",xRegIndex, V[xRegIndex]);
                    if(keyPad[V[xRegIndex]] != 0){
                        pc += 2;
                    }                         

                    break;

                case 0x0001://Skip the following instruction if the key corresponding to the hex value currently stored in register VX is not pressed
                    snprintf(instructionDescription, sizeof(instructionDescription), "SKP V[%d]", xRegIndex);

                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n",xRegIndex, V[xRegIndex]);
                    if(keyPad[V[xRegIndex]] == 0){
                        pc += 2;
                    }                         

                    break;

                default:
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unkonwn opcode: 0x%04X\n", opcode);
                    break;
            }
            break;

        case 0xF000:
            switch (opcode & 0x00FF){
                case 0x0007://Store the current value of the  delay timer in register VX
                    snprintf(instructionDescription, sizeof(instructionDescription), "STR DLYTIM -> V[%d]", xRegIndex);

                    V[xRegIndex] = delay_timer; 
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n",xRegIndex, V[xRegIndex]);
                    break;

                case 0x000A://Wait for a keypress and store the result in register VX
                {
                    snprintf(instructionDescription, sizeof(instructionDescription), "WAT V[%d]", xRegIndex);

                    bool pressed = false;
                    for (int i = 0; i < 16; i++) {
                        if (keyPad[i] == 0x1){
                            V[xRegIndex] = i;      // Store key in Vx
                            pressed = true;
                            break;
                        }
                    }

                    if (!pressed) {
                        //pc stays the same, we stay on the same instruction
                        pc -= 2;
                    }else{
                        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n",xRegIndex, V[xRegIndex]);
                        return;
                    }
                    break;
                }

                case 0x0015://Set the delay timer to the value of register VX
                    snprintf(instructionDescription, sizeof(instructionDescription), "SET DLYTIM -> V[%d]", xRegIndex);

                    delay_timer = V[xRegIndex]; 
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n",xRegIndex, V[xRegIndex]);
                    break;

                case 0x0018://Set the sound timer to the value of register VX
                    snprintf(instructionDescription, sizeof(instructionDescription), "SET SNDTIM -> V[%d]", xRegIndex);

                    sound_timer = V[xRegIndex]; 
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n",xRegIndex, V[xRegIndex]);
                    break;

                case 0x001E: //Add the value stored in register VX to register I
                    snprintf(instructionDescription, sizeof(instructionDescription), "ADD I, V[%d]", xRegIndex);

                    if(I + V[xRegIndex] > 0xFFF){
                        V[0xF] = 1;
                    }
                    else{
                        V[0xF] = 0;
                    }
                    I += V[xRegIndex];
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n",xRegIndex, V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of I: %04X\n", I);
                    break;

                case 0x0029://Set I to the memory address of the sprite data corresponding to the hexadecimal digit stored in register VX
                    snprintf(instructionDescription, sizeof(instructionDescription), "SET I -> V[%d]", xRegIndex);

                    I = V[xRegIndex] * 0x5;
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n",xRegIndex, V[xRegIndex]);
                    break;

                case 0x0033://Store the binary-coded decimal equivalent of the value stored in register VX at addresses I, I + 1, and I + 2
                    snprintf(instructionDescription, sizeof(instructionDescription), "STR V[%d]", xRegIndex);

                    memory[I] = V[xRegIndex] / 100;
                    memory[I + 1] = (V[xRegIndex] / 10) % 10;
                    memory[I + 2] = V[xRegIndex]  % 10;
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of V[%d]: %04X\n",xRegIndex, V[xRegIndex]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of memory[I]: %04X\n", memory[I]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of memory[I + 1]: %04X\n", memory[I + 1]);
                    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Value of memory[I + 2]: %04X\n", memory[I + 2]);

                    break;

                    case 0x0055: //Store the values of registers V0 to VX inclusive in memory starting at address I. I is set to I + X + 1 after operation²
                    snprintf(instructionDescription, sizeof(instructionDescription), "STR V[0] - V[%d]", xRegIndex);

                    for(int i = 0; i <= xRegIndex; i++){
                        memory[I + i] = V[i];
                    }

                    I += xRegIndex + 1;
                    break;

                case 0x0065: //Fill registers V0 to VX inclusive with the values stored in memory starting at address I. I is set to I + X + 1 after operation²
                    snprintf(instructionDescription, sizeof(instructionDescription), "FLL V[0] - V[%d]", xRegIndex);

                    for(int i = 0; i <= xRegIndex; i++){
                        V[i] = memory[I + i];
                    }

                    I += xRegIndex + 1;
                    break;
                default:
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unkonwn opcode: 0x%04X\n", opcode);
                    break;
            }
            break;

        default:
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unkonwn opcode: 0x%04X\n", opcode);
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
                    keyPad[i] = 0x1;
                }
            }

            if(event.key.scancode == SDL_SCANCODE_ESCAPE)
                objects.start= false;

            if(event.key.scancode == SDL_SCANCODE_F1)
                objects.keepGoing = !objects.keepGoing;

            if(event.key.scancode == SDL_SCANCODE_F2){
                if(!objects.keepGoing)
                checkRegisters();
            }

            if(event.key.scancode == SDL_SCANCODE_F3){
                if(!objects.keepGoing)
                checkStack();
            }

            if(event.key.scancode == SDL_SCANCODE_F4){
                if(!objects.keepGoing)
                checkInternals();
            }

            if(event.key.scancode == SDL_SCANCODE_F5){
                objects.keepGoing = !objects.keepGoing;
                checkRegisters();
                checkStack();
                checkKeyPad();
                checkInternals();
            }

            if(event.key.scancode == SDL_SCANCODE_F6){
                objects.executeOnce = true;
            }

        }

        if(event.type == SDL_EVENT_KEY_UP){
            for(int i = 0; i < 16; i++){
                if(keyMap[i] == event.key.scancode){
                    keyPad[i] = 0x0;
                }
            }

        }
    }
    return 0;
}
//


void checkRegisters(){
    printf("CHIP-8 REGISTERS: \n");

    for(int i = 0; i < 16; i++){
        printf("V[%d]: %04X\n", i, V[i]);
    }
    printf("I: %04X\n", I);
    printf("-------------\n");
}

void checkStack(){
    printf("CHIP-8 STACK: \n");
    for(int i = 0; i < 16; i++){
        printf("stack[%d]: %04X\n", i, stack[i]);
    }
    printf("-------------\n");
}


void checkKeyPad(){
    printf("CHIP-8 KEYPAD: \n");
    for(int i = 0; i < 16; i++){
        printf("keypad[%d]: %04X\n", i, keyPad[i]);
    }
    printf("-------------\n");
}

void checkInternals(){
        
    printf("CHIP-8 INTERNALS: \n");
    printf("PC: %04X\n", pc);
    printf("delay_timer: %04X\n", delay_timer);
    printf("sound_timer: %04X\n", sound_timer);
    printf("opcode: %04X\n", opcode);
    printf("-------------\n");
}

int renderFrame(){
    SDL_RenderClear(objects.renderer);
    SDL_Texture *instructionTexture = createInstructionTexture();
    SDL_FRect instructionPanel = {0, 0, (SCREEN_WIDTH * globalConfig->scalingFactor) / 4.0, (SCREEN_HEIGHT * globalConfig->scalingFactor)};
    SDL_RenderTexture(objects.renderer, instructionTexture, NULL, &instructionPanel);
    SDL_DestroyTexture(instructionTexture);

    //Border for the instruction Panel
    SDL_SetRenderDrawColor(objects.renderer, 255, 255, 255, 255); // white border
    SDL_RenderRect(objects.renderer, &instructionPanel);
    SDL_SetRenderDrawColor(objects.renderer, 0, 0, 0, 255); // white border

    uint32_t pixels[2048];

    // Draw if needed
    if (drawFlag) {
        for (int i = 0; i < 2048; i++) {
            pixels[i] = gpx[i] ? 0xFFFFFFFF : 0xFF000000;
        }

        SDL_UpdateTexture(objects.mainScreenTexture, NULL, pixels, 64 * sizeof(Uint32));
        drawFlag = false;
    }

    SDL_FRect mainWindowRect = {instructionPanel.w, 0, (SCREEN_WIDTH * globalConfig->scalingFactor) / 2.0, (SCREEN_HEIGHT * globalConfig->scalingFactor) / 2.0};

    SDL_RenderTexture(objects.renderer, objects.mainScreenTexture, NULL, &mainWindowRect);
    
    //Border for the chip8 screen
    SDL_SetRenderDrawColor(objects.renderer, 255, 255, 255, 255); // white border
    SDL_RenderRect(objects.renderer, &mainWindowRect);
    SDL_SetRenderDrawColor(objects.renderer, 0, 0, 0, 255); 

    //Border of the title
    SDL_SetRenderTarget(objects.renderer, instructionTexture);

    SDL_SetRenderDrawColor(objects.renderer, 255, 255, 255, 255); // white border
    SDL_RenderRect(objects.renderer, &objects.titleRect);
    SDL_SetRenderDrawColor(objects.renderer, 0, 0, 0, 255); 

    SDL_SetRenderTarget(objects.renderer, NULL);



    SDL_RenderPresent(objects.renderer);
    return 0;
}

SDL_Texture *createInstructionTexture(){
    SDL_Texture *targetTexture = SDL_CreateTexture(objects.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, (SCREEN_WIDTH * globalConfig->scalingFactor) / 2, (SCREEN_HEIGHT * globalConfig->scalingFactor));

    SDL_SetRenderTarget(objects.renderer, targetTexture);
    SDL_SetRenderDrawColor(objects.renderer, 0, 0, 0, 255);
    SDL_RenderClear(objects.renderer);

    SDL_Color color = {255, 255, 255, 255};

    //TITLE OF THE TEXTURE
    SDL_RenderTexture(objects.renderer, objects.instructionPanelTitle, NULL, &objects.titleRect);


    //THE ACTUAL INSTRUCTION BEING EXECUTED
    char instruction[10];
    snprintf(instruction, sizeof(instruction),"> %04X", opcode);

    SDL_Surface *instructionSurface = TTF_RenderText_Solid(objects.font, instruction, strlen(instruction), color);
    SDL_Texture *instructionTexture = SDL_CreateTextureFromSurface(objects.renderer, instructionSurface);
    SDL_DestroySurface(instructionSurface);

    SDL_FRect instructionRect = {0, objects.titleRect.h + 5, 300, 50};
    SDL_RenderTexture(objects.renderer, instructionTexture, NULL, &instructionRect);
    SDL_DestroyTexture(instructionTexture);

    SDL_Surface *descriptionSurface = TTF_RenderText_Solid(objects.font, instructionDescription, strlen(instructionDescription), color);
    SDL_Texture *descriptionTexture = SDL_CreateTextureFromSurface(objects.renderer, descriptionSurface);
    SDL_DestroySurface(descriptionSurface);

    SDL_FRect descriptionRect = {20, instructionRect.y + 50, 500, 50};
    SDL_RenderTexture(objects.renderer, descriptionTexture, NULL, &descriptionRect);
    SDL_DestroyTexture(descriptionTexture);

    SDL_SetRenderTarget(objects.renderer, NULL);
    return targetTexture;
}
