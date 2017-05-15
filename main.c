#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <SDL/SDL.h>
#include "z80.c"

void Update();

int main(){
    Update();

    return 0;
}

void Update(){

    int loopCounter = 0;

    while(loopCounter < MAXLOOP){
        int cycles = ExecuteNextOpcode();
        loopCounter+=cycles;
        UpdateTimers(cycles);
        UpdateGraphics(cycles);
        DoInterupts();
    }
   // RenderScreen();
}


