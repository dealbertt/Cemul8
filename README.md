My own chip-8 emulator written in C  
Right now to compile just use:  

```cmd
gcc src/main.c -o chip src/chip8.c -Wall -Werror -lSDL3
```
and then to run the emulator:  

```cmd
./chip ROM.ch8 or ./chip ROM.c8
```
