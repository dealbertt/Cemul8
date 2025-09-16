#include <SDL3/SDL_scancode.h>
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
#include "../include/functions.h"
#include "../include/config.h"


//CHIP-8 specifications
unsigned short opcode; //the operation code of the instruction
unsigned char memory[MEMORY]; //the total memory of the chip-8
unsigned char V[16]; //all the general purpose registers

unsigned short I; //special register I for memory addresses
unsigned short pc; //Program counter


unsigned char gpx[SCREEN_WIDTH * SCREEN_HEIGHT]; //pixels of the screen. Total pixels in the array: 2048
                                                //Keep in mind that this are the screens of the CHIP-8, but not of the actual screen that is showed, that is because the original resolution is way too small

unsigned char delay_timer;
unsigned char sound_timer;

//STACK
unsigned short stack[16];
unsigned short sp; //stack pointer

unsigned char keyPad[16];
//I have no idea how to implement the pad, typeshit

//Flag for indicating when the screen needs to be updated
bool drawFlag = false;

unsigned char chip8_fontset[80] =
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
    for (int i = 0; i < 2048; ++i) {
        gpx[i] = 0;
    }

    // Clear the stack, keypad, and V registers
    for (int i = 0; i < 16; ++i) {
        stack[i] = 0;
        keyPad[i] = 0;
        V[i] = 0;
    }

    // Clear memory
    for (int i = 0; i < 4096; ++i) {
        memory[i] = 0;
    }

    // Load font set into memory
    for (int i = 0; i < 80; ++i) {
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

    const double cpuCycleMs = 2.0;         // CPU cycle every 2 ms
    const double timerIntervalMs = 1000.0 / 60.0; // 60 Hz timer -> ~16.67 ms


    uint32_t pixels[2048];
    objects.start= true;
    objects.keepGoing = true;

    while(objects.start){

        if(objects.keepGoing){
            Uint64 now = SDL_GetPerformanceCounter();
            double elapsedCycle = ((double)(now - lastCycleTime) / (double)frequency) * 1000;
            double elapsedTimer = ((double)(now - lastTimerTick) / (double)frequency) * 1000.0;

            if(elapsedCycle >= cpuCycleMs){
                emulateCycle();
                handleRealKeyboard();
                lastCycleTime = now;
            }else{
            }

            if(elapsedTimer >= timerIntervalMs) {
                if(delay_timer > 0) delay_timer--;
                if(sound_timer > 0) {
                    sound_timer--;
                    if(sound_timer == 0) {
                        printf("Beep!\n"); // Or trigger actual sound
                    }
                }
                lastTimerTick = now;
            }

            if(drawFlag){
                drawFlag = false;
                for (int i = 0; i < 2048; ++i) {
                    uint8_t pixel = gpx[i];
                    pixels[i] = (0x00FFFFFF * pixel) | 0xFF000000;
                }
                // Update SDL texture
                SDL_UpdateTexture(objects.texture, NULL, pixels, 64 * sizeof(Uint32));
                // Clear screen and render
                SDL_RenderClear(objects.renderer);
                SDL_RenderTexture(objects.renderer, objects.texture, NULL, NULL);
                SDL_RenderPresent(objects.renderer);
            }

        }else{ //the execution is stopped
            handleRealKeyboard();
            if(objects.executeOnce){

                emulateCycle();
                if(drawFlag){
                    drawFlag = false;
                    for (int i = 0; i < 2048; ++i) {
                        uint8_t pixel = gpx[i];
                        pixels[i] = (0x00FFFFFF * pixel) | 0xFF000000;
                    }
                    // Update SDL texture
                    SDL_UpdateTexture(objects.texture, NULL, pixels, 64 * sizeof(Uint32));
                    // Clear screen and render
                    SDL_RenderClear(objects.renderer);
                    SDL_RenderTexture(objects.renderer, objects.texture, NULL, NULL);
                    SDL_RenderPresent(objects.renderer);
                }
                objects.executeOnce = false;
            }

        }

    }

}



void emulateCycle(){
    opcode = memory[pc] << 8 | memory[pc + 1];

    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: Opcode: %04X\n", opcode);

    //DECODE
    switch(opcode & 0xF000){ //You only want to look at the first digit because is the one that tells you the opcode, therefore the AND operation with the 0xF000 
        case 0x0000:
            switch(opcode & 0x0FFF){
                case 0x00E0:
                    for(int i = 0; i < 2048; i++){
                        gpx[i] = 0x0;
                    }

                    drawFlag = true;
                    pc += 2;
                    break;
                
                case 0x00EE:
                    if(sp > 0){
                        sp--; //Because we increased when pushing onto the stack to point to the next free slot, we now decrease to retreive the value that was pushed
                        pc = stack[sp];
                    }else{
                        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: The stack pointer is less than 0!\nCurrent value of sp: %d\n", sp);
                    }
                    pc += 2;
                    break;
            }
            break;
        case 0x1000: //jump to the address NNN
            pc = opcode & 0x0FFF;
            break;

        case 0x2000: //calls subroutine at address NNN
            if(sp < 16){
                stack[sp] = pc;
                sp++; //avoid overwriting the current stack
                pc = opcode & 0x0FFF;
            }else{
                SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG]: The stack pointer is greater than 15!\nCurrent value of sp: %d\n", sp);
            }
            break;

        case 0x3000:
            if(V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)){
                pc += 4;
            }else{
                pc += 2;
            }
            break;

        case 0x4000:
            if(V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)){
                pc += 4;
            }else{
                pc += 2;
            }
            break;
        case 0x5000:
            if(V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4]){
                pc += 4;
            }else{
                pc += 2;
            }
            break;

        case 0x6000:
            V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            pc += 2;
            break;

        case 0x7000:
            V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            pc += 2;
            break;

        case 0x8000:
            switch(opcode & 0x000F){
                case 0x0000: //store the value of VY on VX
                    V[(opcode & 0X0F00) >> 8] = V[(opcode & 0X00F0) >> 4];
                    pc += 2;
                    break;

                case 0x0001: //Set VX to VX OR VY
                    V[(opcode & 0X0F00) >> 8] |= V[(opcode & 0X00F0) >> 4];
                    pc += 2;
                    break;

                case 0x0002: //Set VX to VX AND VY
                    V[(opcode & 0X0F00) >> 8] &= V[(opcode & 0X00F0) >> 4];
                    pc += 2;
                    break;

                case 0x0003: //Set VX to VX XOR VY
                    V[(opcode & 0X0F00) >> 8] ^= V[(opcode & 0X00F0) >> 4];
                    pc += 2;
                    break;


                case 0x0004: //Add the value of register VY to register VX
                    V[(opcode & 0X0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
                    if(V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8]))
                        V[0xF] = 1; //carry
                    else
                        V[0xF] = 0;
                    pc += 2;
                    break;

                case 0x0005: //Subtract the value of register VY from register VX
                        if(V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8])
                            V[0xF] = 0; // there is a borrow
                        else
                            V[0xF] = 1;
                        V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
                        pc += 2;
                        break;

                case 0x0006: //Store the value of register VY shifted right one bit in register VX
                        V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
                        V[(opcode & 0x0F00) >> 8] >>= 1;
                        pc += 2;
                        break;

                case 0x0007: //Set register VX to the value of VY minus VX
                        if(V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])	// VY-VX
                            V[0xF] = 0; // there is a borrow
                        else
                            V[0xF] = 1;
                        V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
                        pc += 2;
                    break;

                case 0x000E: //Store the value of register VY shifted left one bit in register VX
                    V[(opcode & 0X0F00) >> 8] = (V[(opcode & 0X00F0) >> 4] << 1);
                    V[15] = (V[(opcode & 0X00F0) >> 4] & 0x80) >> 7; //Shift it to the bit 0 so it stores a 01 or 00, not 10000000...
                    pc += 2;
                    break;
            }
        case 0x9000:
            if(V[(opcode & 0X0F00) >> 8] != V[(opcode & 0X00F0) >> 4]){
                pc += 4;
            }else{
                pc += 2;
            }
            break;

        case 0xA000:
            I = opcode & 0x0FFF; 
            pc += 2;
            break;

        case 0xB000: //Jump to address NNN + V0
            pc = (opcode & 0X0FFF) + V[0];
            break;

        case 0xC000://Set VX Random number with a mask of NN
            V[(opcode & 0x0F00) >> 8] = generateRandomNN(opcode & 0x00FF);
            pc += 2;
            break;

        case 0xD000://Draw a sprite at position VX, VY with N bytes of sprite data starting at the address stored in I 
            {
                unsigned short x = V[(opcode & 0x0F00) >> 8];
                unsigned short y = V[(opcode & 0x00F0) >> 4];
                unsigned short height = opcode & 0x000F;
                unsigned short pixel;

                
                V[0xF] = 0;
                for (int yline = 0; yline < height; yline++)
                {
                    pixel = memory[I + yline];
                    for(int xline = 0; xline < 8; xline++)
                    {
                        if((pixel & (0x80 >> xline)) != 0)
                        {
                            if(gpx[(x + xline + ((y + yline) * 64))] == 1)
                                V[0xF] = 1;                                 

                            gpx[x + xline + ((y + yline) * 64)] ^= 1;
                        }
                    }
                }
                drawFlag = true;
                pc += 2;
            }
            break;
        case 0xE000: 
            switch (opcode & 0x000F) {
                case 0x000E: //Skip the following instruction if the key corresponding to the hex value currently stored in register VX is pressed
                    if(keyPad[V[(opcode & 0x0F00) >> 8]] == 0x1){
                        pc += 4;
                    }else{
                        pc += 2;
                    }                         
                    break;

                case 0x0001://Skip the following instruction if the key corresponding to the hex value currently stored in register VX is not pressed
                    if(keyPad[V[(opcode & 0x0F00) >> 8]] == 0x0){
                        pc += 4;
                    }else{
                        pc += 2;
                    }                         
                    break;
            }
            break;

        case 0xF000:
            switch (opcode & 0x00FF){
                case 0x0007://Store the current value of the  delay timer in register VX
                    V[(opcode & 0x0F00) >> 8] = delay_timer; 
                    pc+= 2;
                    break;

                case 0x000A://Wait for a keypress and store the result in register VX
                {
                    bool pressed = false;
                    for (int i = 0; i < 16; i++) {
                        if (keyPad[i] == 0x1){
                            V[(opcode & 0x0F00) >> 8] = i;      // Store key in Vx
                            pressed = true;
                            break;
                        }
                    }

                    if (!pressed) {
                        //pc stays the same, we stay on the same instruction
                        return;
                    }else{
                        pc += 2; // we go to the next
                    }
                }
                break;

                case 0x0015://Set the delay timer to the value of register VX
                    delay_timer = V[(opcode & 0X0F00) >> 8]; 
                    pc += 2;
                    break;

                case 0x0018://Set the sound timer to the value of register VX
                    sound_timer = V[(opcode & 0X0F00) >> 8]; 
                    pc += 2;
                    break;

                case 0x001E:
                    if(I + V[(opcode & 0x0F00) >> 8] > 0xFFF){
                        V[0xF] = 1;
                    }
                    else{
                        V[0xF] = 0;
                    }
                    I += V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;

                case 0x0029://Set I to the memory address of the sprite data corresponding to the hexadecimal digit stored in register VX
                    I = V[(opcode & 0x0F00) >> 8] * 0x5;
                    pc+= 2;
                    break;

                case 0x0033://Store the binary-coded decimal equivalent of the value stored in register VX at addresses I, I + 1, and I + 2
                    memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
                    memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
                    memory[I + 2] = V[(opcode & 0x0F00) >> 8]  % 10;
                    pc += 2;
                    break;

                    case 0x0055: //Store the values of registers V0 to VX inclusive in memory starting at address I. I is set to I + X + 1 after operation²
                    for(int i = 0; i < (opcode & 0x0F00); i++){
                        memory[I + i] = V[i];
                    }
                    I += ((opcode & 0x0F00) >> 8) + 1;
                    pc+= 2;
                    break;

                case 0x0065: //Fill registers V0 to VX inclusive with the values stored in memory starting at address I. I is set to I + X + 1 after operation²
                    for(int i = 0; i <= ((opcode & 0x0F00) >> 8); i++){
                        V[i] = memory[I + i];
                    }
                    I += ((opcode & 0x0F00) >> 8) + 1;
                    pc+= 2;
                    break;
            }
            break;
        default:
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unkonwn opcode: 0x%04X\n", opcode);
            pc += 2;
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
